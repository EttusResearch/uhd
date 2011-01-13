//
// Copyright 2011 Ettus Research LLC
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
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/any.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace pt = boost::posix_time;
namespace lt = boost::local_time;

/***********************************************************************
 * Utility helper functions
 **********************************************************************/

//TODO put these in time_spec_t (maybe useful)

time_spec_t time_dur_to_time_spec(const pt::time_duration &time_dur){
    return time_spec_t(
        time_dur.total_seconds(),
        time_dur.fractional_seconds(),
        pt::time_duration::ticks_per_second()
    );
}

pt::time_duration time_spec_to_time_dur(const time_spec_t &time_spec){
    return pt::time_duration(
        0, 0, time_spec.get_full_secs(),
        time_spec.get_tick_count(pt::time_duration::ticks_per_second())
    );
}

/***********************************************************************
 * Soft time control implementation
 **********************************************************************/
class soft_time_ctrl_impl : public soft_time_ctrl{
public:
    soft_time_ctrl_impl(const cb_fcn_type &stream_on_off):
        _nsamps_remaining(0),
        _stream_mode(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS),
        _cmd_queue(bounded_buffer<boost::any>::make(2)),
        _stream_on_off(stream_on_off)
    {
        //synchronously spawn a new thread
        _update_mutex.lock(); //lock mutex before spawned
        _thread_group.create_thread(boost::bind(&soft_time_ctrl_impl::recv_cmd_dispatcher, this));
        _update_mutex.lock(); //lock blocks until spawned
        _update_mutex.unlock(); //unlock mutex before done
    }

    ~soft_time_ctrl_impl(void){
        _thread_running = false;
        _thread_group.join_all();
    }

    void set_time(const time_spec_t &time){
        _time_offset = pt::microsec_clock::universal_time() - time_spec_to_time_dur(time);
    }

    time_spec_t get_time(void){
        return time_dur_to_time_spec(pt::microsec_clock::universal_time() - _time_offset);
    }

    void recv_post(rx_metadata_t &md, size_t &nsamps){
        //load the metadata with the current time
        md.has_time_spec = true;
        md.time_spec = get_time();

        //lock the mutex here before changing state
        boost::mutex::scoped_lock lock(_update_mutex);

        //When to stop streaming:
        //The samples have been received and the stream mode is non-continuous.
        //Rewrite the sample count to clip to the requested number of samples.
        if (_nsamps_remaining <= nsamps and
            _stream_mode != stream_cmd_t::STREAM_MODE_START_CONTINUOUS
        ){
            nsamps = _nsamps_remaining; //set nsamps, then stop
            stream_on_off(false);
            return;
        }

        //update the consumed samples
        _nsamps_remaining -= nsamps;
    }

    void send_pre(const tx_metadata_t &md, double /*TODO timeout*/){
        if (not md.has_time_spec) return;
        sleep_until_time(md.time_spec); //TODO late?
    }

    void issue_stream_cmd(const stream_cmd_t &cmd){
        _cmd_queue->push_with_wait(cmd);
    }

private:

    void sleep_until_time(const time_spec_t &time){
        boost::this_thread::sleep(_time_offset + time_spec_to_time_dur(time));
    }

    void recv_cmd_handle_cmd(const stream_cmd_t &cmd){
        //handle the stream at time by sleeping
        if (not cmd.stream_now) sleep_until_time(cmd.time_spec); //TODO late?

        //lock the mutex here before changing state
        boost::mutex::scoped_lock lock(_update_mutex);

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

    void recv_cmd_dispatcher(void){
        _thread_running = true;
        _update_mutex.unlock();

        boost::any cmd;
        while (_thread_running){
            if (_cmd_queue->pop_with_timed_wait(cmd, 1.0)){
                recv_cmd_handle_cmd(boost::any_cast<stream_cmd_t>(cmd));
            }
        }
    }

    void stream_on_off(bool stream){
        _stream_on_off(stream);
        _nsamps_remaining = 0;
    }

    boost::mutex _update_mutex;
    size_t _nsamps_remaining;
    stream_cmd_t::stream_mode_t _stream_mode;

    pt::ptime _time_offset;
    bounded_buffer<boost::any>::sptr _cmd_queue;
    const cb_fcn_type _stream_on_off;
    boost::thread_group _thread_group;
    bool _thread_running;
};

/***********************************************************************
 * Soft time control factor
 **********************************************************************/
soft_time_ctrl::sptr soft_time_ctrl::make(const cb_fcn_type &stream_on_off){
    return sptr(new soft_time_ctrl_impl(stream_on_off));
}
