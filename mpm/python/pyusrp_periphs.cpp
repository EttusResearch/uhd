#include "n310_periphs.hpp"
// #include "tests_periphs.hpp"
#include "lib_periphs.hpp"
#include <mpm/print_foo.hpp>
#include <boost/python.hpp>
#include <boost/noncopyable.hpp>

namespace bp = boost::python;

BOOST_PYTHON_MODULE(libpyusrp_periphs)
{
    bp::object package = bp::scope();
    package.attr("__path__") = "libpyusrp_periphs";
    bp::def("print_foo", &mpm::print_foo);
    export_spi();
    // export_tests();
    export_lmk();
    export_n3xx();
}
