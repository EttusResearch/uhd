//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_DEVICE3_HPP
#define INCLUDED_UHD_DEVICE3_HPP

#include <uhd/device.hpp>
#include <uhd/rfnoc/graph.hpp>
#include <uhd/rfnoc/block_ctrl_base.hpp>
#include <boost/units/detail/utility.hpp>
#include <boost/thread/mutex.hpp>
#include <vector>

namespace uhd {

/*!
 * \brief Extends uhd::device for third-generation USRP devices.
 *
 * Generation-3 devices are characterized by the following traits:
 * - They support RFNoC (RF Network-on-Chip).
 * - Data transport uses the compressed VITA (CVITA/CHDR) data format.
 */
class UHD_API device3 : public uhd::device {

  public:
    typedef boost::shared_ptr<device3> sptr;

    //! Same as uhd::device::make(), but will fail if not actually a device3
    static sptr make(const device_addr_t &hint, const size_t which = 0);

    virtual rfnoc::graph::sptr create_graph(const std::string &name="") = 0;

    /*! Reset blocks after a stream.
     *
     * TODO write docs
     */
    void clear();

    /*! \brief Checks if an RFNoC block exists on the device.
     *
     * \param block_id Canonical block name (e.g. "0/FFT_1").
     * \return true if a block with the specified id exists
     * \note this access is not thread safe if peformed during block enumeration
     */
    bool has_block(const rfnoc::block_id_t &block_id) const;

    /*! Same as has_block(), but with a type check.
     *
     * \return true if a block of type T with the specified id exists
     * \note this access is not thread safe if peformed during block enumeration
     */
    template <typename T>
    bool has_block(const rfnoc::block_id_t &block_id) const
    {
        if (has_block(block_id)) {
            return bool(boost::dynamic_pointer_cast<T>(get_block_ctrl(block_id)));
        } else {
            return false;
        }
    }

    /*! \brief Returns a block controller class for an RFNoC block.
     *
     * If the given block ID is not valid (i.e. such a block does not exist
     * on this device), it will throw a uhd::lookup_error.
     *
     * \param block_id Canonical block name (e.g. "0/FFT_1").
     * \note this access is not thread safe if peformed during block enumeration
     */
    rfnoc::block_ctrl_base::sptr get_block_ctrl(const rfnoc::block_id_t &block_id) const;

    /*! Same as get_block_ctrl(), but with a type cast.
     *
     * If you have a block controller class that is derived from block_ctrl_base,
     * use this function to access its specific methods.
     * If the given block ID is not valid (i.e. such a block does not exist
     * on this device) or if the type does not match, it will throw a uhd::lookup_error.
     *
     * \code{.cpp}
     * // Assume DEV is a device3::sptr
     * uhd::rfnoc::my_block_ctrl::sptr block_controller = get_block_ctrl<my_block_ctrl>("0/MyBlock_0");
     * block_controller->my_own_block_method();
     * \endcode
     * \note this access is not thread safe if peformed during block enumeration
     */
    template <typename T>
    boost::shared_ptr<T> get_block_ctrl(const rfnoc::block_id_t &block_id) const
    {
        boost::shared_ptr<T> blk = boost::dynamic_pointer_cast<T>(get_block_ctrl(block_id));
        if (blk) {
            return blk;
        } else {
            throw uhd::lookup_error(str(boost::format("This device does not have a block of type %s with ID: %s")
                % boost::units::detail::demangle(typeid(T).name())
                % block_id.to_string()));
        }
    }

    /*! Returns the block ids of all blocks that match the specified hint
     * Uses block_ctrl_base::match() internally.
     * If no matching block is found, it returns an empty vector.
     *
     * To access specialized block controller classes (i.e. derived from block_ctrl_base),
     * use the templated version of this function, e.g.
     * \code{.cpp}
     * // Assume DEV is a device3::sptr
     * null_block_ctrl::sptr null_block = DEV->find_blocks<null_block_ctrl>("NullSrcSink");
     * \endcode
     * \note this access is not thread safe if peformed during block enumeration
     */
    std::vector<rfnoc::block_id_t> find_blocks(const std::string &block_id_hint) const;

    /*! Type-cast version of find_blocks().
     */
    template <typename T>
    std::vector<rfnoc::block_id_t> find_blocks(const std::string &block_id_hint) const
    {
        std::vector<rfnoc::block_id_t> all_block_ids = find_blocks(block_id_hint);
        std::vector<rfnoc::block_id_t> filt_block_ids;
        for (size_t i = 0; i < all_block_ids.size(); i++) {
            if (has_block<T>(all_block_ids[i])) {
                filt_block_ids.push_back(all_block_ids[i]);
            }
        }
        return filt_block_ids;
    }

  protected:
    //! List of *all* RFNoC blocks available on this device.
    //  It is the responsibility of the deriving class to make
    //  sure this gets correctly populated.
    std::vector< rfnoc::block_ctrl_base::sptr > _rfnoc_block_ctrl;
    //! Mutex to protect access to members
    boost::mutex                                _block_ctrl_mutex;
};

} //namespace uhd

#endif /* INCLUDED_UHD_DEVICE3_HPP */
// vim: sw=4 et:
