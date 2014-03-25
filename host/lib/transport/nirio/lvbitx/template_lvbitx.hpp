{autogen_msg}

#ifndef INCLUDED_{lvbitx_classname_u}_LVBITX_HPP
#define INCLUDED_{lvbitx_classname_u}_LVBITX_HPP

#include <uhd/transport/nirio/nifpga_lvbitx.h>

namespace uhd {{ namespace niusrprio {{

class {lvbitx_classname}_lvbitx : public nifpga_lvbitx {{
public:
    {lvbitx_classname}_lvbitx(const std::string& option);

    virtual ~{lvbitx_classname}_lvbitx() {{}};

    virtual const char* get_bitfile_path();
    virtual const char* get_signature();
    virtual const char* get_bitstream_checksum();

    virtual size_t get_input_fifo_count();
    virtual const char** get_input_fifo_names();

    virtual size_t get_output_fifo_count();
    virtual const char** get_output_fifo_names();

    virtual size_t get_control_count();
    virtual const char** get_control_names();

    virtual size_t get_indicator_count();
    virtual const char** get_indicator_names();

    virtual void init_register_info(nirio_register_info_vtr& vtr);
    virtual void init_fifo_info(nirio_fifo_info_vtr& vtr);

    static const char* CONTROLS[];
    static const char* INDICATORS[];
    static const char* OUTPUT_FIFOS[];
    static const char* INPUT_FIFOS[];

private:
    std::string _fpga_file_name;
    std::string _bitstream_checksum;
}};

}}}}

#endif /* INCLUDED_{lvbitx_classname_u}_LVBITX_HPP */

