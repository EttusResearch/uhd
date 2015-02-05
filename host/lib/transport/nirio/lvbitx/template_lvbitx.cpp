{autogen_msg}

#include "{lvbitx_classname}_lvbitx.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <uhd/utils/paths.hpp>

namespace uhd {{ namespace niusrprio {{

#define SEARCH_PATHS "{lvbitx_search_paths}"

const char* {lvbitx_classname}_lvbitx::CONTROLS[] = {{{control_list}
}};

const char* {lvbitx_classname}_lvbitx::INDICATORS[] = {{{indicator_list}
}};

const char* {lvbitx_classname}_lvbitx::OUTPUT_FIFOS[] = {{{out_fifo_list}
}};

const char* {lvbitx_classname}_lvbitx::INPUT_FIFOS[] = {{{in_fifo_list}
}};

{lvbitx_classname}_lvbitx::{lvbitx_classname}_lvbitx(const std::string& option)
{{
    std::string fpga_file = "usrp_{lvbitx_classname}_fpga_" + option + ".lvbitx";
    boost::filesystem::path fpga_path(uhd::find_image_path(fpga_file, SEARCH_PATHS));

    _fpga_file_name = fpga_path.string();
    _bitstream_checksum = _get_bitstream_checksum(_fpga_file_name);
}}

const char* {lvbitx_classname}_lvbitx::get_bitfile_path() {{
    return _fpga_file_name.c_str();
}}

const char* {lvbitx_classname}_lvbitx::get_signature() {{
    return "{lvbitx_signature}";
}}

const char* {lvbitx_classname}_lvbitx::get_bitstream_checksum() {{
    return _bitstream_checksum.c_str();
}}

size_t {lvbitx_classname}_lvbitx::get_input_fifo_count() {{
    return sizeof(INPUT_FIFOS)/sizeof(*INPUT_FIFOS);
}}

const char** {lvbitx_classname}_lvbitx::get_input_fifo_names() {{
    return INPUT_FIFOS;
}}

size_t {lvbitx_classname}_lvbitx::get_output_fifo_count() {{
    return sizeof(OUTPUT_FIFOS)/sizeof(*OUTPUT_FIFOS);
}}

const char** {lvbitx_classname}_lvbitx::get_output_fifo_names() {{
    return OUTPUT_FIFOS;
}}

size_t {lvbitx_classname}_lvbitx::get_control_count() {{
    return sizeof(CONTROLS)/sizeof(*CONTROLS);
}}

const char** {lvbitx_classname}_lvbitx::get_control_names() {{
    return CONTROLS;
}}

size_t {lvbitx_classname}_lvbitx::get_indicator_count() {{
    return sizeof(INDICATORS)/sizeof(*INDICATORS);
}}

const char** {lvbitx_classname}_lvbitx::get_indicator_names() {{
    return INDICATORS;
}}

void {lvbitx_classname}_lvbitx::init_register_info(nirio_register_info_vtr& vtr) {{ {register_init}
}}

void {lvbitx_classname}_lvbitx::init_fifo_info(nirio_fifo_info_vtr& vtr) {{ {fifo_init}
}}

}}}}
