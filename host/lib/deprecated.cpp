//----------------------------------------------------------------------
//-- deprecated interfaces below, to be removed when the API is changed
//----------------------------------------------------------------------

#include <uhd/types/otw_type.hpp>
#include <uhd/types/io_type.hpp>
#include <stdint.h>
#include <stdexcept>
#include <complex>
#include <vector>

using namespace uhd;

/***********************************************************************
 * otw type
 **********************************************************************/
size_t otw_type_t::get_sample_size(void) const{
    return (this->width * 2) / 8;
}

otw_type_t::otw_type_t(void):
    width(0),
    shift(0),
    byteorder(BO_NATIVE)
{
    /* NOP */
}

/***********************************************************************
 * io type
 **********************************************************************/
static std::vector<size_t> get_tid_size_table(void){
    std::vector<size_t> table(128, 0);
    table[size_t(io_type_t::COMPLEX_FLOAT64)] = sizeof(std::complex<double>);
    table[size_t(io_type_t::COMPLEX_FLOAT32)] = sizeof(std::complex<float>);
    table[size_t(io_type_t::COMPLEX_INT16)]   = sizeof(std::complex<int16_t>);
    table[size_t(io_type_t::COMPLEX_INT8)]    = sizeof(std::complex<int8_t>);
    return table;
}

static const std::vector<size_t> tid_size_table(get_tid_size_table());

io_type_t::io_type_t(tid_t tid):
    size(tid_size_table[size_t(tid) & 0x7f]), tid(tid)
{
    /* NOP */
}

io_type_t::io_type_t(size_t size):
    size(size), tid(CUSTOM_TYPE)
{
    /* NOP */
}

#include <uhd/types/clock_config.hpp>

using namespace uhd;

clock_config_t clock_config_t::external(void){
    clock_config_t clock_config;
    clock_config.ref_source = clock_config_t::REF_SMA;
    clock_config.pps_source = clock_config_t::PPS_SMA;
    clock_config.pps_polarity = clock_config_t::PPS_POS;
    return clock_config;
}

clock_config_t clock_config_t::internal(void){
    clock_config_t clock_config;
    clock_config.ref_source = clock_config_t::REF_INT;
    clock_config.pps_source = clock_config_t::PPS_SMA;
    clock_config.pps_polarity = clock_config_t::PPS_POS;
    return clock_config;
}

clock_config_t::clock_config_t(void):
    ref_source(REF_INT),
    pps_source(PPS_SMA),
    pps_polarity(PPS_POS)
{
    /* NOP */
}
