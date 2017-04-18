#include "../../host/lib/usrp/common/lmk04828.hpp"

#ifdef LIBMPM_PYTHON
void export_(){
    LIBMPM_BOOST_PREAMBLE("lmk04828")
    bp::class_<lmk04828_iface>("lmk04828", bp::init<lmk04828_iface::write_fn_t, lmk04828_iface::read_fn_t>())
        .def("verify_chip_id", &lmk04828_iface::verify_chip_id)
        .def("get_chip_id", &lmk04828_iface::get_chip_id)
        .def("init", &lmk04828_iface::init)
        .def("send_sysref_pulse", &lmk04828_iface::send_sysref_pulse)
    ;
}
#endif
