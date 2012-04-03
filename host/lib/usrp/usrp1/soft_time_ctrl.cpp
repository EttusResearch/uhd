//
// Copyright 2011-2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "soft_time_ctrl.hpp"
#include <uhd/utils/tasks.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace pt = boost::posix_time;

static const time_spec_t TWIDDLE(0.0011);

/***********************************************************************
 * Soft time control implementation
 **********************************************************************/
class soft_time_ctrl_impl : public soft_time_ctrl{
public:

    soft_time_ctrl_impl(const cb_fcn_type &stream_on_off):
        _nsamps_remaining(0),
        _stream_mode(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS),
        _cmd_queue(2),
        _async_msg_queue(1000),
        _inline_msg_queue(1000),
        _stream_on_off(stream_on_off)
    {
        //synchronously spawn a new thread
        _recv_cmd_task = task::make(boost::bind(&soft_time_ctrl_impl::recv_cmd_task, this));

        //initialize the time to something
        this->set_time(time_spec_t(0.0));
    }

    /*******************************************************************
     * Time control
     ******************************************************************/
    void set_time(const time_spec_t &time){
        boost::mutex::scoped_lock lock(_update_mutex);
        _time_offset = time_spec_t::get_system_time() - time;
    }

    time_spec_t get_time(void){
        boost::mutex::scoped_lock lock(_update_mutex);
        return time_now();
    }

    UHD_INLINE time_spec_t time_now(void){
        //internal get time without scoped lock
        return time_spec_t::get_system_time() - _time_offset;
    }

    UHD_INLINE void sleep_until_time(
        boost::mutex::scoped_lock &lock, const time_spec_t &time
    ){
        boost::condition_variable cond;
        //use a condition variable to unlock, sleep, lock
        double seconds_to_sleep = (time - time_now()).get_real_secs();
        cond.timed_wait(lock, pt::microseconds(long(seconds_to_sleep*1e6)));
    }

    /*******************************************************************
     * Receive control
     ******************************************************************/
    size_t recv_post(rx_metadata_t &md, const size_t nsamps){
        boost::mutex::scoped_lock lock(_update_mutex);

        //Since it timed out on the receive, check for inline messages...
        //Must do a post check because recv() will not wake up for a message.
        if (md.error_code == rx_metadata_t::ERROR_CODE_TIMEOUT){
            if (_inline_msg_queue.pop_with_haste(md)) return 0;
        }

        //load the metadata with the expected time
        md.has_time_spec = true;
        md.time_spec = time_now();

        //none of the stuff below matters in continuous streaming mode
        if (_stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS) return nsamps;

        //When to stop streaming:
        //The samples have been received and the stream mode is non-continuous.
        //Rewrite the sample count to clip to the requested number of samples.
        if (_nsamps_remaining <= nsamps) switch(_stream_mode){
        case stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE:{
            rx_metadata_t metadata;
            metadata.has_time_spec = true;
            metadata.time_spec = this->time_now();
            metadata.error_code = rx_metadata_t::ERROR_CODE_BROKEN_CHAIN;
            _inline_msg_queue.push_with_pop_on_full(metadata);
        } //continue to next case...
        case stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE:
            md.end_of_burst = true;
            this->issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            return _nsamps_remaining;
        default: break;
        }

        //update the consumed samples
        _nsamps_remaining -= nsamps;
        return nsamps;
    }

    void issue_stream_cmd(const stream_cmd_t &cmd){
        _cmd_queue.push_with_wait(boost::make_shared<stream_cmd_t>(cmd));
    }

    void stream_on_off(bool enb){
        _stream_on_off(enb);
        _nsamps_remaining = 0;
    }

    /*******************************************************************
     * Transmit control
     ******************************************************************/
    void send_pre(const tx_metadata_t &md, double &timeout){
        if (not md.has_time_spec) return;

        boost::mutex::scoped_lock lock(_update_mutex);

        time_spec_t time_at(md.time_spec - TWIDDLE);

        //handle late packets
        if (time_at < time_now()){
            async_metadata_t metadata;
            metadata.channel = 0;
            metadata.has_time_spec = true;
            metadata.time_spec = this->time_now();
            metadata.event_code = async_metadata_t::EVENT_CODE_TIME_ERROR;
            _async_msg_queue.push_with_pop_on_full(metadata);
            return;
        }

        timeout -= (time_at - time_now()).get_real_secs();
        sleep_until_time(lock, time_at);
    }

    /*******************************************************************
     * Thread control
     ******************************************************************/
    void recv_cmd_handle_cmd(const stream_cmd_t &cmd){
        boost::mutex::scoped_lock lock(_update_mutex);

        //handle the stream at time by sleeping
        if (not cmd.stream_now){
            time_spec_t time_at(cmd.time_spec - TWIDDLE);
            if (time_at < time_now()){
                rx_metadata_t metadata;
                metadata.has_time_spec = true;
                metadata.time_spec = this->time_now();
                metadata.error_code = rx_metadata_t::ERROR_CODE_LATE_COMMAND;
                _inline_msg_queue.push_with_pop_on_full(metadata);
                this->issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
                return;
            }
            else{
                sleep_until_time(lock, time_at);
            }
        }

        //When to stop streaming:
        //Stop streaming when the command is a stop and streaming.
        if (cmd.stream_mode == stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS
           and _stream_mode != stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS
        ) stream_on_off(false);

        //When to start streaming:
        //Start streaming when the command is not a stop and not streaming.
        if (cmd.stream_mode != stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS
           and _stream_mode == stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS
        ) stream_on_off(true);

        //update the state
        _nsamps_remaining += cmd.num_samps;
        _stream_mode = cmd.stream_mode;
    }

    void recv_cmd_task(void){ //task is looped
        boost::shared_ptr<stream_cmd_t> cmd;
        _cmd_queue.pop_with_wait(cmd);
        recv_cmd_handle_cmd(*cmd);
    }

    bounded_buffer<async_metadata_t> &get_async_queue(void){
        return _async_msg_queue;
    }

    bounded_buffer<rx_metadata_t> &get_inline_queue(void){
        return _inline_msg_queue;
    }

    void stop(void){
        _recv_cmd_task.reset();
    }

private:
    boost::mutex _update_mutex;
    size_t _nsamps_remaining;
    stream_cmd_t::stream_mode_t _stream_mode;
    time_spec_t _time_offset;
    bounded_buffer<boost::shared_ptr<stream_cmd_t> > _cmd_queue;
    bounded_buffer<async_metadata_t> _async_msg_queue;
    bounded_buffer<rx_metadata_t> _inline_msg_queue;
    const cb_fcn_type _stream_on_off;
    task::sptr _recv_cmd_task;
};

/***********************************************************************
 * Soft time control factor
 **********************************************************************/
soft_time_ctrl::sptr soft_time_ctrl::make(const cb_fcn_type &stream_on_off){
    return sptr(new soft_time_ctrl_impl(stream_on_off));
}
