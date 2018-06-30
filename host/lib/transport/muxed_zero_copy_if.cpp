//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/muxed_zero_copy_if.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/safe_call.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <map>
#include <chrono>
#include <thread>

using namespace uhd;
using namespace uhd::transport;

class muxed_zero_copy_if_impl : public muxed_zero_copy_if,
                                public boost::enable_shared_from_this<muxed_zero_copy_if_impl>
{
public:
    typedef boost::shared_ptr<muxed_zero_copy_if_impl> sptr;

    muxed_zero_copy_if_impl(
        zero_copy_if::sptr base_xport,
        stream_classifier_fn classify_fn,
        size_t max_streams
    ):
        _base_xport(base_xport), _classify(classify_fn),
        _max_num_streams(max_streams), _num_dropped_frames(0)
    {
        //Create the receive thread to poll the underlying transport
        //and classify packets into queues
        _recv_thread = boost::thread(
            boost::bind(&muxed_zero_copy_if_impl::_update_queues, this));
    }

    virtual ~muxed_zero_copy_if_impl()
    {
        UHD_SAFE_CALL(
            //Interrupt buffer updater loop
            _recv_thread.interrupt();
            //Wait for loop to finish
            //No timeout on join. The recv loop is guaranteed
            //to terminate in a reasonable amount of time because
            //there are no timed blocks on the underlying.
            _recv_thread.join();
            //Flush base transport
            while (_base_xport->get_recv_buff(0.0001)) /*NOP*/;
            //Release child streams
            //Note that this will not delete or flush the child streams
            //until the owners of the streams have released the respective
            //shared pointers. This ensures that packets are not dropped.
            _streams.clear();
        );
    }

    virtual zero_copy_if::sptr make_stream(const uint32_t stream_num)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        if (_streams.size() >= _max_num_streams) {
            throw uhd::runtime_error("muxed_zero_copy_if: stream capacity exceeded. cannot create more streams.");
        }
        // Only allocate a portion of the base transport's frames to each stream
        // to prevent all streams from attempting to use all the frames.
        stream_impl::sptr stream = boost::make_shared<stream_impl>(
            this->shared_from_this(), stream_num,
            _base_xport->get_num_send_frames() / _max_num_streams,
            _base_xport->get_num_recv_frames() / _max_num_streams);
        _streams[stream_num] = stream;
        return stream;
    }

    virtual size_t get_num_dropped_frames() const
    {
        return _num_dropped_frames;
    }

    void remove_stream(const uint32_t stream_num)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        _streams.erase(stream_num);
    }

private:
    /*
     * @class stream_mrb is used to copy the data and release the original
     * managed receive buffer back to the base transport.
     */
    class stream_mrb : public managed_recv_buffer
    {
    public:
        stream_mrb(size_t size) : _buff(new char[size]) {}

        ~stream_mrb() {
            delete[] _buff;
        }

        void release() {}

        UHD_INLINE sptr get_new(char *buff, size_t len)
        {
            memcpy(_buff, buff, len);
            return make(this, _buff, len);
        }

    private:
        char *_buff;
    };

    class stream_impl : public zero_copy_if
    {
    public:
        typedef boost::shared_ptr<stream_impl> sptr;
        typedef boost::weak_ptr<stream_impl> wptr;

        stream_impl(
            muxed_zero_copy_if_impl::sptr muxed_xport,
            const uint32_t stream_num,
            const size_t num_send_frames,
            const size_t num_recv_frames
            ) :
            _stream_num(stream_num), _muxed_xport(muxed_xport),
            _num_send_frames(num_send_frames),
            _send_frame_size(_muxed_xport->base_xport()->get_send_frame_size()),
            _num_recv_frames(num_recv_frames),
            _recv_frame_size(_muxed_xport->base_xport()->get_recv_frame_size()),
            _buff_queue(num_recv_frames),
            _buffers(num_recv_frames),
            _buffer_index(0)
        {
            for (size_t i = 0; i < num_recv_frames; i++) {
                _buffers[i] = boost::make_shared<stream_mrb>(_recv_frame_size);
            }
        }

        ~stream_impl(void)
        {
            //First remove the stream from muxed transport
            //so no more frames are pushed in
            _muxed_xport->remove_stream(_stream_num);
            //Flush the transport
            managed_recv_buffer::sptr buff;
            while (_buff_queue.pop_with_haste(buff)) {
                //NOP
            }
        }

        size_t get_num_recv_frames(void) const {
            return _num_recv_frames;
        }

        size_t get_recv_frame_size(void) const {
            return _recv_frame_size;
        }

        managed_recv_buffer::sptr get_recv_buff(double timeout) {
            managed_recv_buffer::sptr buff;
            if (_buff_queue.pop_with_timed_wait(buff, timeout)) {
                return buff;
            } else {
                return managed_recv_buffer::sptr();
            }
        }

        void push_recv_buff(managed_recv_buffer::sptr buff) {
            _buff_queue.push_with_wait(_buffers.at(_buffer_index++)->get_new(buff->cast<char*>(), buff->size()));
            _buffer_index %= _buffers.size();
        }

        size_t get_num_send_frames(void) const {
            return _num_send_frames;
        }

        size_t get_send_frame_size(void) const {
            return _send_frame_size;
        }

        managed_send_buffer::sptr get_send_buff(double timeout)
        {
            return _muxed_xport->base_xport()->get_send_buff(timeout);
        }

    private:
        const uint32_t                              _stream_num;
        muxed_zero_copy_if_impl::sptr               _muxed_xport;
        const size_t                                _num_send_frames;
        const size_t                                _send_frame_size;
        const size_t                                _num_recv_frames;
        const size_t                                _recv_frame_size;
        bounded_buffer<managed_recv_buffer::sptr>   _buff_queue;
        std::vector< boost::shared_ptr<stream_mrb> >    _buffers;
        size_t                                      _buffer_index;
    };

    inline zero_copy_if::sptr& base_xport() { return _base_xport; }

    void _update_queues()
    {
        //Run forever:
        // - Pull packets from the base transport
        // - Classify them
        // - Push them to the appropriate receive queue
        while (true) {
            {   //Uninterruptable block of code
                boost::this_thread::disable_interruption interrupt_disabler;
                if (not _process_next_buffer()) {
                    //Be a good citizen and yield if no packet is processed
                    static const size_t MIN_DUR = 1;
                    std::this_thread::sleep_for(std::chrono::nanoseconds(MIN_DUR));
                    //We call sleep(MIN_DUR) above instead of yield() to ensure that we
                    //relinquish the current scheduler time slot.
                    //yield() is a hint to the scheduler to end the time
                    //slice early and schedule in another thread that is ready to run.
                    //However in most situations, there will be no other thread and
                    //this thread will continue to run which will rail a CPU core.
                    //We call sleep(MIN_DUR=1) instead which will sleep for a minimum time.
                    //Ideally we would like to use boost::chrono::.*seconds::min() but that
                    //is bound to 0, which causes the sleep_for call to be a no-op and
                    //thus useless to actually force a sleep.

                    //****************************************************************
                    //NOTE: This behavior makes this transport a poor choice for
                    //      low latency communication.
                    //****************************************************************
                }
            }
            //Check if the master thread has requested a shutdown
            if (boost::this_thread::interruption_requested()) break;
        }
    }

    bool _process_next_buffer()
    {
        managed_recv_buffer::sptr buff = _base_xport->get_recv_buff(0.0);
        if (buff) {
            stream_impl::sptr stream;
            try {
                const uint32_t stream_num = _classify(buff->cast<void*>(), _base_xport->get_recv_frame_size());
                {
                    //Hold the stream mutex long enough to pull a bounded buffer
                    //and lock it (increment its ref count).
                    boost::lock_guard<boost::mutex> lock(_mutex);
                    stream_map_t::iterator str_iter = _streams.find(stream_num);
                    if (str_iter != _streams.end()) {
                        stream = (*str_iter).second.lock();
                    }
                }
            } catch (std::exception&) {
                //If _classify throws we simply drop the frame
            }
            //Once a bounded buffer is acquired, we can rely on its
            //thread safety to serialize with the consumer.
            if (stream.get()) {
                stream->push_recv_buff(buff);
            } else {
                boost::lock_guard<boost::mutex> lock(_mutex);
                _num_dropped_frames++;
            }
            //We processed a packet, and there could be more coming
            //Don't yield in the next iteration.
            return true;
        } else {
            //The base transport is idle. Return false to let the
            //thread yield.
            return false;
        }
    }

    typedef std::map<uint32_t, stream_impl::wptr> stream_map_t;

    zero_copy_if::sptr      _base_xport;
    stream_classifier_fn    _classify;
    stream_map_t            _streams;
    const size_t            _max_num_streams;
    size_t                  _num_dropped_frames;
    boost::thread           _recv_thread;
    boost::mutex            _mutex;
};

muxed_zero_copy_if::sptr muxed_zero_copy_if::make(
    zero_copy_if::sptr base_xport,
    muxed_zero_copy_if::stream_classifier_fn classify_fn,
    size_t max_streams
) {
    return boost::make_shared<muxed_zero_copy_if_impl>(base_xport, classify_fn, max_streams);
}
