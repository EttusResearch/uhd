//USB->GPIF->FPGA loopback test for USRP1P
//uses UHD libusb transport

#include <uhd/device.hpp>
#include <uhd/transport/usb_zero_copy.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/usb_control.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/shared_array.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <vector>
#include <iostream>
#include <iomanip>

//so the goal is to open a USB device to endpoints (2,6), submit a buffer, receive a reply, and compare them.
//use usb_zero_copy::make() to get a usb_zero_copy object and then start submitting.
//need to get a usb dev handle to pass to make
//use static std::vector<usb_device_handle::sptr> get_device_list(boost::uint16_t vid, boost::uint16_t pid) to get a device handle
//then get_send_buffer, send, etc.
using namespace uhd;
using namespace uhd::transport;

const boost::uint16_t data_xfer_size = 32;
const boost::uint16_t ctrl_xfer_size = 32;

int main(int argc, char *argv[]) {
    std::cout << "USRP1+ GPIF loopback test" << std::endl;
    //step 1: get a handle on it
    std::vector<usb_device_handle::sptr> handles = usb_device_handle::get_device_list(0xfffe, 0x0003);
    if(handles.size() == 0) {
        std::cout << "No USRP1+ found." << std::endl;
        return ~0;
    }
    
    bool verbose = false;
    if(argc > 1) if(std::string(argv[1]) == "-v") verbose = true;
    
    usb_device_handle::sptr handle = handles.front();

    usb_zero_copy::sptr data_transport;
    usb_control::sptr ctrl_transport = usb_control::make(handle); //just in case

    data_transport = usb_zero_copy::make(
                handle,        // identifier
                8,             // IN endpoint
                4,             // OUT endpoint
                uhd::device_addr_t("recv_frame_size=32, num_recv_frames=1, send_frame_size=32, num_send_frames=1") //args
    );
    
    if(verbose) std::cout << "Made." << std::endl;
    
    //ok now we're made. time to get a buffer and start sending data.
    
    boost::uint8_t localbuf[data_xfer_size];

    managed_send_buffer::sptr sbuf;
    managed_recv_buffer::sptr rbuf;
    size_t xfercount = 0;
    
    srand(time(0));
    while(1) {

        if(verbose) std::cout << "Getting send buffer." << std::endl;
        sbuf = data_transport->get_send_buff();
        if(sbuf == 0) {
            std::cout << "Failed to get a send buffer." << std::endl;
            return ~0;
        }
        for(int i = 0; i < data_xfer_size; i++) {
            boost::uint8_t x = rand();
            sbuf->cast<boost::uint8_t *>()[i] = x;
            localbuf[i] = x;
        }
        
        if(verbose) std::cout << "Buffer loaded" << std::endl;

        sbuf->commit(data_xfer_size);
        if(verbose) std::cout << "Committed." << std::endl;

        rbuf = data_transport->get_recv_buff(0.3); //timeout
        
        if(rbuf == 0) {
            std::cout << "Failed to get receive buffer (timeout?)" << std::endl;
            return ~0;
        }
        
        if(verbose) std::cout << "# " << xfercount << std::endl;
    
        if(!memcmp(rbuf->cast<const boost::uint8_t *>(), localbuf, data_xfer_size)) {
            std::cout << ".";
        } else {
            if(verbose) {
                int i = 0;
                for(int j = 0; j < 32; j++) {
                    std::cout << boost::format("%02X ") % int(rbuf->cast<const boost::uint8_t *>()[i*32+j]);
                }
                std::cout << std::endl;
            }    
            else std::cout << "x";

        }
        sbuf.reset();
        rbuf.reset();
        xfercount++;
        //if(verbose) std::cout << "sptrs reset" << std::endl;
    }
    
    return 0;
}
