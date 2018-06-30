//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/tasks.hpp>
#include <uhd/utils/msg_task.hpp>
#include <uhd/utils/thread.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <exception>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>

using namespace uhd;

class task_impl : public task{
public:

    task_impl(const task_fcn_type &task_fcn, const std::string &name):
        _exit(false)
    {
        _task = std::thread([this, task_fcn](){ this->task_loop(task_fcn); });
        if (not name.empty()) {
#ifdef HAVE_PTHREAD_SETNAME
            pthread_setname_np(_task->native_handle(), name.substr(0,16).c_str());
#endif /* HAVE_PTHREAD_SETNAME */
        }
    }

    ~task_impl(void){
        _exit = true;
        if (_task.joinable()) {
            _task.join();
        }
    }

private:
    void task_loop(const task_fcn_type &task_fcn){
        try{
            while (!_exit){
                task_fcn();
            }
        }
        catch(const std::exception &e){
            do_error_msg(e.what());
        }
        catch(...){
            UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void do_error_msg(const std::string &msg){
        UHD_LOGGER_ERROR("UHD")
            << "An unexpected exception was caught in a task loop."
            << "The task loop will now exit, things may not work."
            << msg
        ;
    }

    std::atomic<bool> _exit;
    std::thread _task;
};

task::sptr task::make(const task_fcn_type &task_fcn, const std::string &name){
    return task::sptr(new task_impl(task_fcn, name));
}

msg_task::~msg_task(void){
    /* NOP */
}

/*
 * During shutdown pointers to queues for radio_ctrl_core might not be available anymore.
 * msg_task_impl provides a dump_queue for such messages.
 * ctrl_cores can check this queue for stranded messages.
 */

class msg_task_impl : public msg_task{
public:

    msg_task_impl(const task_fcn_type &task_fcn):
        _spawn_barrier(2)
    {
        (void)_thread_group.create_thread(boost::bind(&msg_task_impl::task_loop, this, task_fcn));
        _spawn_barrier.wait();
    }

    ~msg_task_impl(void){
        _running = false;
        _thread_group.interrupt_all();
        _thread_group.join_all();
    }

    /*
     * Returns the first message for the given SID.
     * This way a radio_ctrl_core doesn't have to die in timeout but can check for stranded messages here.
     * This might happen during shutdown when dtors are called.
     * See also: comments in b200_io_impl->handle_async_task
     */
    msg_payload_t get_msg_from_dump_queue(uint32_t sid)
    {
        boost::mutex::scoped_lock lock(_mutex);
        msg_payload_t b;
        for (size_t i = 0; i < _dump_queue.size(); i++) {
            if (sid == _dump_queue[i].first) {
                b = _dump_queue[i].second;
                _dump_queue.erase(_dump_queue.begin() + i);
                break;
            }
        }
        return b;
    }

private:

    void task_loop(const task_fcn_type &task_fcn){
        _running = true;
        _spawn_barrier.wait();

        try{
            while (_running){
            	boost::optional<msg_type_t> buff = task_fcn();
            	if(buff != boost::none){
            	    /*
            	     * If a message gets stranded it is returned by task_fcn and then pushed to the dump_queue.
            	     * This way ctrl_cores can check dump_queue for missing messages.
            	     */
            	    boost::mutex::scoped_lock lock(_mutex);
            	    _dump_queue.push_back(buff.get() );
            	}
            }
        }
        catch(const boost::thread_interrupted &){
            //this is an ok way to exit the task loop
        }
        catch(const std::exception &e){
            do_error_msg(e.what());
        }
        catch(...){
            //FIXME
            //Unfortunately, this is also an ok way to end a task,
            //because on some systems boost throws uncatchables.
        }
    }

    void do_error_msg(const std::string &msg){
        UHD_LOGGER_ERROR("UHD")
            << "An unexpected exception was caught in a task loop." 
            << "The task loop will now exit, things may not work." 
            << msg 
        ;
    }

    boost::mutex _mutex;
    boost::thread_group _thread_group;
    boost::barrier _spawn_barrier;
    bool _running;

    /*
     * This queue holds stranded messages until a radio_ctrl_core grabs them via 'get_msg_from_dump_queue'.
     */
    std::vector <msg_type_t> _dump_queue;
};

msg_task::sptr msg_task::make(const task_fcn_type &task_fcn){
    return msg_task::sptr(new msg_task_impl(task_fcn));
}
