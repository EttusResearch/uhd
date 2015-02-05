//
// Copyright 2013-2014 Ettus Research LLC
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

#include <uhd/transport/nirio/nifpga_lvbitx.h>
#include <cstdlib>
#include <string>
#include <fstream>
#include <streambuf>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace uhd { namespace niusrprio {

std::string nifpga_lvbitx::_get_bitstream_checksum(const std::string& file_path)
{
    std::string checksum;
    std::ifstream lvbitx_stream(file_path.c_str());
    if (lvbitx_stream.is_open()) {
        std::string lvbitx_contents;
        lvbitx_stream.seekg(0, std::ios::end);
        lvbitx_contents.reserve(static_cast<size_t>(lvbitx_stream.tellg()));
        lvbitx_stream.seekg(0, std::ios::beg);
        lvbitx_contents.assign((std::istreambuf_iterator<char>(lvbitx_stream)), std::istreambuf_iterator<char>());
        try {
            boost::smatch md5_match;
            if (boost::regex_search(lvbitx_contents, md5_match, boost::regex("<BitstreamMD5>([a-zA-Z0-9]{32})<\\/BitstreamMD5>", boost::regex::icase))) {
                checksum = std::string(md5_match[1].first, md5_match[1].second);
            }
        } catch (boost::exception&) {
            checksum = "";
        }
    }
    boost::to_upper(checksum);
    return checksum;
}

}}
