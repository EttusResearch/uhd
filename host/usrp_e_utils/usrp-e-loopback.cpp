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

#include "common.hpp"
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <boost/cstdint.hpp>
#include <sys/mman.h> //mmap
#include <unistd.h> //getpagesize
#include <poll.h> //poll

static const size_t bytes_per_frame = 2048;

static const int poll_timeout_ms = 100;

struct loopback_pkt_hdr_type{
    boost::uint32_t words32;
    boost::uint32_t checksum;
    boost::uint64_t seq_num;
};

struct loopback_pkt_type{
    loopback_pkt_hdr_type hdr;
    boost::uint32_t data[(bytes_per_frame-sizeof(loopback_pkt_hdr_type))/sizeof(boost::uint32_t)];
};

static ring_buffer_info (*recv_info)[];
static ring_buffer_info (*send_info)[];
static loopback_pkt_type (*recv_buff)[];
static loopback_pkt_type (*send_buff)[];

static struct usrp_e_ring_buffer_size_t rb_size;

static bool running = true;

static boost::uint64_t seq_errors = 0;
static boost::uint64_t checksum_errors = 0;
static boost::uint64_t sent_words32 = 0;
static boost::uint64_t recvd_words32 = 0;

static inline void print_pkt(const loopback_pkt_type &pkt){
    std::cout << std::endl;
    std::cout << "pkt.hdr.words32 " << pkt.hdr.words32 << std::endl;
    std::cout << "pkt.hdr.checksum " << pkt.hdr.checksum << std::endl;
    std::cout << "pkt.hdr.seq_num " << pkt.hdr.seq_num << std::endl;
}

boost::uint32_t my_checksum(void *buff, size_t size32){
    boost::uint32_t x = 0;
    for (size_t i = 0; i < size32; i++){
        x += reinterpret_cast<boost::uint32_t *>(buff)[i];
        x ^= reinterpret_cast<boost::uint32_t *>(buff)[i];
    }
    return x;
}

/***********************************************************************
 * Read thread - recv frames and verify checksum
 **********************************************************************/
static void read_thread(void){
    std::cout << "start read thread... " << std::endl;

    boost::uint64_t seq_num = 0;
    size_t index = 0;

    while (running){

        loopback_pkt_type &pkt = (*recv_buff)[index];
        ring_buffer_info &info = (*recv_info)[index];

        //wait for frame available
        if (not (info.flags & RB_USER)){
            pollfd pfd;
            pfd.fd = fp;
            pfd.events = POLLIN;
            if (poll(&pfd, 1, poll_timeout_ms) <= 0){
                std::cout << "Read poll timeout, exiting read thread!" << std::endl;
                running = false;
                return;
            }
        }
        info.flags = RB_USER_PROCESS;

        //print_pkt(pkt);

        //handle checksum
        const boost::uint32_t expected_checksum = pkt.hdr.checksum;
        pkt.hdr.checksum = 0; //set to zero for calculation
        const boost::uint32_t actual_checksum = my_checksum(&pkt, pkt.hdr.words32);
        if (expected_checksum != actual_checksum){
            checksum_errors++;
            std::cerr << "C";
        }
        else{
            recvd_words32 += pkt.hdr.words32;
        }

        //handle sequence
        if (seq_num != pkt.hdr.seq_num){
            seq_errors++;
            std::cerr << "S";
        }
        seq_num = pkt.hdr.seq_num+1;

        //release the packet
        info.flags = RB_KERNEL;

        //increment index and wrap around to zero
        index++;
        if (index == size_t(rb_size.num_rx_frames)) index = 0;
    }

}

/***********************************************************************
 * Write thread - send frames and calculate checksum
 **********************************************************************/
static void write_thread(const size_t num_words32){
    std::cout << "start write thread... " << std::endl;

    srandom(std::time(NULL));

    boost::uint64_t seq_num = 0;
    size_t index = 0;

    //write into tmp and memcopy into pkt to avoid cache issues
    loopback_pkt_type pkt_tmp;

    while (running){

        ring_buffer_info &info = (*send_info)[index];

        //wait for frame available
        if (not (info.flags & RB_KERNEL)){
            pollfd pfd;
            pfd.fd = fp;
            pfd.events = POLLOUT;
            if (poll(&pfd, 1, poll_timeout_ms) <= 0){
                std::cout << "Write poll timeout, exiting write thread!" << std::endl;
                running = false;
                return;
            }
        }

        //fill packet header and body
        const boost::uint32_t seed = random();
        pkt_tmp.hdr.words32 = sizeof(pkt_tmp.hdr)/sizeof(boost::uint32_t) + num_words32;
        pkt_tmp.hdr.checksum = 0; //set to zero for checksum()
        pkt_tmp.hdr.seq_num = seq_num++;
        for (size_t i = 0; i < num_words32; i++) pkt_tmp.data[i] = seed + i;
        pkt_tmp.hdr.checksum = my_checksum(&pkt_tmp, pkt_tmp.hdr.words32);
        sent_words32 += pkt_tmp.hdr.words32;

        loopback_pkt_type &pkt = (*send_buff)[index];
        std::memcpy(&pkt, &pkt_tmp, pkt_tmp.hdr.words32*sizeof(boost::uint32_t));

        //print_pkt(pkt);

        //commit the packet
        info.len = pkt_tmp.hdr.words32*sizeof(boost::uint32_t);
        info.flags = RB_USER;
        ::write(fp, NULL, 0);

        //increment index and wrap around to zero
        index++;
        if (index == size_t(rb_size.num_tx_frames)) index = 0;
    }
}

/***********************************************************************
 * Setup memory mapped ring buffer
 **********************************************************************/
static void setup_ring(void){
    std::cout << "setup memory mapped ring buffer... " << std::flush;

    //calculate various sizes
    const size_t page_size = getpagesize();
    ioctl(fp, USRP_E_GET_RB_INFO, &rb_size);
    const size_t map_size = (rb_size.num_pages_rx_flags + rb_size.num_pages_tx_flags) * page_size +
        (rb_size.num_rx_frames + rb_size.num_tx_frames) * (page_size >> 1);

    //call into memory map
    void *mem = ::mmap(0, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fp, 0);
    if (mem == MAP_FAILED) {
        std::cerr << "mmap failed" << std::endl;
        std::exit(-1);
    }

    //calculate the memory offsets for info and buffers
    const size_t recv_info_off = 0;
    const size_t recv_buff_off = recv_info_off + (rb_size.num_pages_rx_flags * page_size);
    const size_t send_info_off = recv_buff_off + (rb_size.num_rx_frames * page_size/2);
    const size_t send_buff_off = send_info_off + (rb_size.num_pages_tx_flags * page_size);

    //set the internal pointers for info and buffers
    typedef ring_buffer_info (*rbi_pta)[];
    typedef loopback_pkt_type (*pkt_pta)[];
    char *rb_ptr = reinterpret_cast<char *>(mem);
    recv_info = reinterpret_cast<rbi_pta>(rb_ptr + recv_info_off);
    recv_buff = reinterpret_cast<pkt_pta>(rb_ptr + recv_buff_off);
    send_info = reinterpret_cast<rbi_pta>(rb_ptr + send_info_off);
    send_buff = reinterpret_cast<pkt_pta>(rb_ptr + send_buff_off);

    std::cout << "done" << std::endl;
}

/***********************************************************************
 * Main
 **********************************************************************/
#include <boost/program_options.hpp>

int main(int argc, char *argv[]){

    //variables to be set by po
    double duration;
    size_t nwords;

    //setup the program options
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("duration", po::value<double>(&duration)->default_value(10), "number of seconds to run loopback")
        ("nwords", po::value<size_t>(&nwords)->default_value(400), "number of words32 to send per packet")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD USRP-E-Loopback %s") % desc << std::endl;
        return ~0;
    }

    if ((fp = ::open("/dev/usrp_e0", O_RDWR)) < 0){
        std::cerr << "Open failed" << std::endl;
        return -1;
    }

    //set the mode to loopback
    poke16(E100_REG_MISC_XFER_RATE, (1<<8) | (1<<9));

    //clear FIFO state in FPGA and kernel
    poke32(E100_REG_CLEAR_RX, 0);
    poke32(E100_REG_CLEAR_TX, 0);
    ::close(fp);
    if ((fp = ::open("/dev/usrp_e0", O_RDWR)) < 0){
        std::cerr << "Open failed" << std::endl;
        return -1;
    }

    //setup the ring buffer
    setup_ring();

    //spawn threads
    boost::thread_group tg;
    tg.create_thread(boost::bind(&read_thread));
    tg.create_thread(boost::bind(&write_thread, nwords));

    const boost::system_time start_time = boost::get_system_time();
    const boost::system_time finish_time = start_time + boost::posix_time::milliseconds(long(duration*1000));
    while (boost::get_system_time() < finish_time){
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        std::cerr << ".";
    }
    running = false;
    tg.join_all();

    std::cout << std::endl;
    std::cout << "seq_errors          " << seq_errors << std::endl;
    std::cout << "checksum_errors     " << checksum_errors << std::endl;
    std::cout << "sent_words32        " << sent_words32 << std::endl;
    std::cout << "recvd_words32       " << recvd_words32 << std::endl;
    std::cout << "approx send rate    " << (sent_words32/duration)/1e6 << "Msps" << std::endl;
    std::cout << "approx recv rate    " << (recvd_words32/duration)/1e6 << "Msps" << std::endl;

    ::close(fp);
    return seq_errors + checksum_errors;
}
