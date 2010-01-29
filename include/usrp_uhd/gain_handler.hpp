//
// Copyright 2010 Ettus Research LLC
//

#include <boost/shared_ptr.hpp>
#include <usrp_uhd/wax.hpp>
#include <usrp_uhd/props.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#ifndef INCLUDED_USRP_UHD_GAIN_HANDLER_HPP
#define INCLUDED_USRP_UHD_GAIN_HANDLER_HPP

namespace usrp_uhd{

class gain_handler{
public:
    typedef boost::shared_ptr<gain_handler> sptr;

    template <class T> gain_handler(
        wax::obj::ptr wax_obj_ptr, const T &gain_prop,
        const T &gain_min_prop, const T &gain_max_prop,
        const T &gain_step_prop, const T &gain_names_prop
    ){
        _wax_obj_ptr = wax_obj_ptr;
        _gain_prop = gain_prop;
        _gain_min_prop = gain_min_prop;
        _gain_max_prop = gain_max_prop;
        _gain_step_prop = gain_step_prop;
        _gain_names_prop = gain_names_prop;
        _is_equal = boost::bind(&gain_handler::is_equal<T>, _1, _2);
    }

    ~gain_handler(void);

    /*!
     * Intercept gets for overall gain, min, max, step.
     * Ensures that the gain name is valid.
     * \return true for handled, false to pass on
     */
    bool intercept_get(const wax::type &key, wax::type &val);

    /*!
     * Intercept sets for overall gain.
     * Ensures that the gain name is valid.
     * Ensures that the new gain is within range.
     * \return true for handled, false to pass on
     */
    bool intercept_set(const wax::type &key, const wax::type &val);

private:

    wax::obj::ptr  _wax_obj_ptr;
    wax::type      _gain_prop;
    wax::type      _gain_min_prop;
    wax::type      _gain_max_prop;
    wax::type      _gain_step_prop;
    wax::type      _gain_names_prop;

    /*!
     * Verify that the key is valid:
     * If its a named prop for gain, ensure that name is valid.
     * If the name if not valid, throw a std::invalid_argument.
     * The name can only be valid if its in the list of gain names.
     */
    void _check_key(const wax::type &key);

    /*
     * Private interface to test if two wax types are equal:
     * The constructor will bind an instance of this for a specific type.
     * This bound equals functions allows the intercept methods to be non-templated.
     */
    template <class T> static bool is_equal(const wax::type &a, const wax::type &b){
        try{
            return wax::cast<T>(a) == wax::cast<T>(b);
        }
        catch(const wax::bad_cast &){
            return false;
        }
    }
    boost::function<bool(const wax::type &, const wax::type &)> _is_equal;

};

} //namespace usrp_uhd

#endif /* INCLUDED_USRP_UHD_GAIN_HANDLER_HPP */

