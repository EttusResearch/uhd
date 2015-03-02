//
// Copyright 2015 Per Vices Corporation
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

#include "udp_common.hpp"
#include <uhd/transport/udp_stream.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

// C headers
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * UDP stream implementation: stream in(RX stream) and stream out(TX stream)
 **********************************************************************/
class udp_stream_impl : public udp_stream {
public:
    udp_stream_impl(
        const std::string &addr, const std::string &port, bool isRX, bool isTX
    ):_isConnected(false){
	_isConnected = false;
	_isTX = isTX;
	_isRX = isRX;
	_slen = sizeof(_si);

    	if (_isRX) {
        	UHD_LOG << boost::format("Creating RX udp stream for %s %s") % addr % port << std::endl;
	} else if (_isTX) {
	        UHD_LOG << boost::format("Creating TX udp stream for %s %s") % addr % port << std::endl;
	}

	// create the UDP socket
	_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (_sockfd < 0) {
		UHD_MSG(error) << boost::format("ERROR socket(): %s") % strerror(errno) << std::endl;
		return;
	}

	// setup is different for RX stream and TX stream
	if (_isRX) {
		// clear the sock structure
		memset((char *) &_si, 0, sizeof(_si));
		_si.sin_family = AF_INET;
		_si.sin_port = htons(boost::lexical_cast<int>(port));
		inet_pton(AF_INET, addr.c_str(), &(_si.sin_addr));

		// bind the settings to the socket
		if ( bind(_sockfd, (struct sockaddr *) &_si, sizeof(_si)) < 0) {
			UHD_MSG(error) << boost::format("ERROR bind(): %s:%i %s") % addr.c_str() % port % strerror(errno) << std::endl;
			return;
		}
		_isConnected = true;

	// setting up TX stream
	} else if (_isTX) {
		// clear the sock structure
		memset((char *) &_si, 0, sizeof(_si));
		_si.sin_family = AF_INET;
		_si.sin_port = htons(boost::lexical_cast<int>(port));
		inet_pton(AF_INET, addr.c_str(), &(_si.sin_addr));

		// don't need to bind for TX stream
		_isConnected = true;
	}
    }

    ~udp_stream_impl(void) {
       close(_sockfd);
    }

    size_t stream_out(const void* buff, size_t size){
    	if (!_isConnected) UHD_MSG(error) << "UDP stream not connected." << std::endl;
	if (!_isTX) UHD_MSG(error) << "UDP stream cannot stream in because not a TX stream." << std::endl;

	return sendto(_sockfd, buff, size, 0, (struct sockaddr*) &_si, sizeof(_si));
    }

    size_t stream_in(void* buff, size_t size, double timeout){
    	if (!_isConnected) UHD_MSG(error) << "UDP stream not connected." << std::endl;
	if (!_isRX) UHD_MSG(error) << "UDP stream cannot stream in because not an RX stream." << std::endl;

	int ret = 0;

	// set the socket with a timeout option
	struct timeval tout;
	tout.tv_sec  = (long int)timeout;
	tout.tv_usec = (long int)((timeout - (long int)timeout) * 1000000);
	ret = setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout));
	if (ret < 0 ) {
		UHD_MSG(error) << boost::format("UDP stream: incorrect timeout specified %lf.") %timeout << std::endl;
		return 0;
	}

	// receive from the socket
	ret = recvfrom(_sockfd, buff, size, 0, (struct sockaddr *) &_si, &_slen);
	if (ret < 0) {
		UHD_LOG << "UDP stream timedout." << std::endl;
		ret = 0;
	}

        return ret;
    }

private:
    bool _isConnected;
    bool _isTX;
    bool _isRX;
    int _sockfd;
    socklen_t _slen;
    struct sockaddr_in _si;
};

udp_stream::~udp_stream(void){
    /* NOP */
}

/***********************************************************************
 * UDP public make functions
 **********************************************************************/
udp_stream::sptr udp_stream::make_rx_stream(
    const std::string &addr, const std::string &port
){
    return sptr(new udp_stream_impl(addr, port, true, false));
}

udp_stream::sptr udp_stream::make_tx_stream(
    const std::string &addr, const std::string &port
){
    return sptr(new udp_stream_impl(addr, port, false, true));
}
