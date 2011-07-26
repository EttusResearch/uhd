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

#include <uhd/utils/tasks.hpp>
#include <uhd/utils/msg.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <exception>
#include <iostream>

using namespace uhd;

class task_impl : public task{
public:

    task_impl(const task_fcn_type &task_fcn):
        _spawn_barrier(2)
    {
        _thread_group.create_thread(boost::bind(&task_impl::task_loop, this, task_fcn));
        _spawn_barrier.wait();
    }

    ~task_impl(void){
        _running = false;
        _thread_group.interrupt_all();
        _thread_group.join_all();
    }

private:

    void task_loop(const task_fcn_type &task_fcn){
        _running = true;
        _spawn_barrier.wait();

        try{
            while (_running){
                task_fcn();
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
        UHD_MSG(error)
            << "An unexpected exception was caught in a task loop." << std::endl
            << "The task loop will now exit, things may not work." << std::endl
            << msg << std::endl
        ;
    }

    boost::thread_group _thread_group;
    boost::barrier _spawn_barrier;
    bool _running;
};

task::sptr task::make(const task_fcn_type &task_fcn){
    return task::sptr(new task_impl(task_fcn));
}
