//
// Copyright 2010-2011 Ettus Research LLC
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

#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <csignal>
#include <vector>

namespace po = boost::program_options;
namespace asio = boost::asio;

typedef boost::shared_ptr<asio::ip::udp::socket> socket_type;

static const size_t insane_mtu = 9000;

boost::mutex spawn_mutex;

/***********************************************************************
 * Signal handlers
 **********************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

static bool wait_for_recv_ready(int sock_fd){
    //setup timeval for timeout
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; //100ms

    //setup rset for timeout
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(sock_fd, &rset);

    //call select with timeout on receive socket
    return ::select(sock_fd+1, &rset, NULL, NULL, &tv) > 0;
}

/***********************************************************************
 * Relay class
 **********************************************************************/
class udp_relay_type{
public:
    udp_relay_type(
        const std::string &server_addr,
        const std::string &client_addr,
        const std::string &port
    ):_port(port){
        {
            asio::ip::udp::resolver resolver(_io_service);
            asio::ip::udp::resolver::query query(asio::ip::udp::v4(), server_addr, port);
            asio::ip::udp::endpoint endpoint = *resolver.resolve(query);

            _server_socket = boost::shared_ptr<asio::ip::udp::socket>(new asio::ip::udp::socket(_io_service, endpoint));
        }
        {
            asio::ip::udp::resolver resolver(_io_service);
            asio::ip::udp::resolver::query query(asio::ip::udp::v4(), client_addr, port);
            asio::ip::udp::endpoint endpoint = *resolver.resolve(query);

            _client_socket = boost::shared_ptr<asio::ip::udp::socket>(new asio::ip::udp::socket(_io_service));
            _client_socket->open(asio::ip::udp::v4());
            _client_socket->connect(endpoint);
        }

        std::cout << "spawning relay threads..." << std::endl;
        _thread_group.create_thread(boost::bind(&udp_relay_type::server_thread, this));
        spawn_mutex.lock();
        spawn_mutex.lock();
        spawn_mutex.unlock();
        _thread_group.create_thread(boost::bind(&udp_relay_type::client_thread, this));
        spawn_mutex.lock();
        spawn_mutex.lock();
        spawn_mutex.unlock();
        std::cout << "   done" << std::endl;
    }

    ~udp_relay_type(void){
        std::cout << "killing relay threads..." << std::endl;
        _thread_group.interrupt_all();
        _thread_group.join_all();
        std::cout << "   done" << std::endl;
    }

private:

    void server_thread(void){
        std::cout << "entering server_thread... " << _port << std::endl;
        spawn_mutex.unlock();
        std::vector<char> buff(insane_mtu);
        while (not boost::this_thread::interruption_requested()){
            if (wait_for_recv_ready(_server_socket->native())){
                boost::mutex::scoped_lock lock(_endpoint_mutex);
                const size_t len = _server_socket->receive_from(asio::buffer(&buff.front(), buff.size()), _endpoint);
                lock.unlock();
                //std::cout << " len " << len << std::endl;
                _client_socket->send(asio::buffer(&buff.front(), len));
            }
        }
        std::cout << "exiting server_thread... " << _port << std::endl;
    }

    void client_thread(void){
        std::cout << "entering client_thread... " << _port << std::endl;
        spawn_mutex.unlock();
        std::vector<char> buff(insane_mtu);
        while (not boost::this_thread::interruption_requested()){
            if (wait_for_recv_ready(_client_socket->native())){
                const size_t len = _client_socket->receive(asio::buffer(&buff.front(), buff.size()));
                //std::cout << " len " << len << std::endl;
                boost::mutex::scoped_lock lock(_endpoint_mutex);
                _server_socket->send_to(asio::buffer(&buff.front(), len), _endpoint);
            }
        }
        std::cout << "exiting client_thread... " << _port << std::endl;
    }

    const std::string _port;
    boost::thread_group _thread_group;
    asio::io_service _io_service;
    asio::ip::udp::endpoint _endpoint;
    boost::mutex _endpoint_mutex;
    socket_type _server_socket, _client_socket;
};


/***********************************************************************
 * Main
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string addr;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("addr", po::value<std::string>(&addr)->default_value(""), "the resolvable address of the usrp")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout
            << boost::format("UHD Network Relay %s") % desc << std::endl
            << "Runs a network relay between UHD on one computer and a USRP on the network.\n"
            << "This example is basically for test purposes. Use at your own convenience.\n"
            << std::endl;
        return ~0;
    }

    {
        boost::shared_ptr<udp_relay_type> ctrl(new udp_relay_type("0.0.0.0", addr, "49152"));
        boost::shared_ptr<udp_relay_type> rxdsp0(new udp_relay_type("0.0.0.0", addr, "49156"));
        boost::shared_ptr<udp_relay_type> txdsp0(new udp_relay_type("0.0.0.0", addr, "49157"));
        boost::shared_ptr<udp_relay_type> rxdsp1(new udp_relay_type("0.0.0.0", addr, "49158"));
        boost::shared_ptr<udp_relay_type> gps(new udp_relay_type("0.0.0.0", addr, "49172"));

        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;

        while (not stop_signal_called){
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }
    }

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}
