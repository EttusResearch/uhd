//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/features/discoverable_feature_getter_iface.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>

namespace uhd { namespace rfnoc {

/*! A default block controller for blocks that can't be found in the registry
 */
class UHD_API mb_controller
    : public uhd::noncopyable,
      public virtual ::uhd::features::discoverable_feature_getter_iface
{
public:
    using sptr          = std::shared_ptr<mb_controller>;
    using sync_source_t = device_addr_t;

    /*! Callback function for changing sync sources
     *
     * When a sync source is changed, the sync source callback function is called
     * to notify any registrants of the update
     */
    using sync_source_updater_t = std::function<void(const sync_source_t& sync_source)>;

    ~mb_controller() override = default;

    /**************************************************************************
     * Timebase API
     *************************************************************************/
    /*! Interface to interact with timekeepers
     *
     * A timekeeper is an entity within a USRPs FPGA to track the time. For
     * example, the execution of timed commands requires the existence of a
     * timekeeper.
     *
     * Timekeepers are objects separate from RFNoC blocks, but RFNoC blocks can
     * have access to the time provided by timekeepers in order to execute
     * commands at a certain time (e.g., the radio blocks use this to be able to
     * assign a timestamp to samples). Note that most other RFNoC blocks do not
     * require access to a timekeeper to execute timed commands, as they will
     * execute commands relative to the incoming data.
     *
     */
    class UHD_API timekeeper
    {
    public:
        using sptr              = std::shared_ptr<timekeeper>;
        using write_period_fn_t = std::function<void(uint64_t)>;

        timekeeper();

        virtual ~timekeeper() {}

        /*! Return the current time as a time spec
         *
         * Note that there is no control over when this command gets executed,
         * it will read the time "as soon as possible", and then return that
         * value. Calling this on two synchronized clocks sequentially will
         * definitely return two different values.
         *
         * When using the RFNoC API, radio blocks also provide API calls
         * (uhd::rfnoc::radio_control::get_time_now() and
         * uhd::rfnoc::radio_control::get_ticks_now()) which directly returns the
         * current time from the radio block itself. This has the advantage that
         * it is not necessary to know which timekeeper is providing the time to
         * which radio block when reading the current time, and generally has a
         * lower latency than executing this call.
         *
         * \returns the current time
         */
        uhd::time_spec_t get_time_now(void);

        /*! Return the current time as a tick count
         *
         * When using the RFNoC API, radio blocks also provide API calls
         * (uhd::rfnoc::radio_control::get_time_now() and
         * uhd::rfnoc::radio_control::get_ticks_now()) which directly returns the
         * current time from the radio block itself. This has the advantage that
         * it is not necessary to know which timekeeper is providing the time to
         * which radio block when reading the current time, and generally has a
         * lower latency than executing this call.
         *
         * See also get_time_now().
         *
         * \returns the current time
         */
        virtual uint64_t get_ticks_now() = 0;

        /*! Return the time from the last PPS as a time spec
         *
         * Note that there is no control over when this command gets executed,
         * it will read the time "as soon as possible", and then return that
         * value. Calling this on two synchronized clocks sequentially will
         * definitely return two different values.
         */
        uhd::time_spec_t get_time_last_pps(void);

        /*! Return the time from the last PPS as a tick count
         *
         * See also get_time_last_pps()
         */
        virtual uint64_t get_ticks_last_pps() = 0;

        /*! Set the time "now" from a time spec
         *
         * This will convert \p time into a tick count value and call
         * set_ticks_now().
         */
        void set_time_now(const uhd::time_spec_t& time);

        /*! Set the ticks "now"
         *
         * This will set the tick count on the remote device's timekeeper as
         * soon as possible. Note that there is some amount of lag between
         * executing this call and when the device's time will be updated, e.g.,
         * due to network latency.
         */
        virtual void set_ticks_now(const uint64_t ticks) = 0;

        /*! Set the time at next PPS from a time spec
         *
         * \b Note: When changing clock sources, a previously set time will
         * most likely be lost. It is recommended to set the time after
         * changing the clock source. Otherwise, an unexpected time may line
         * up with future PPS edges.
         *
         * This will convert \p time into a tick count value and use that to
         * call set_ticks_next_pps().
         */
        void set_time_next_pps(const uhd::time_spec_t& time);

        /*! Set the ticks at next PPS
         *
         * This will instruct the remote device to set its tick count when the
         * next PPS edge is detected. Use this to set multiple devices to the
         * same time (assuming they are synchronized in time and frequency).
         *
         * To guarantee that devices are synchronized in time it is recommended
         * to wait for a PPS edge before calling this command. Otherwise, it
         * could happen that due to network latency or other reasons, this
         * command reaches different devices on different sides of the same PPS
         * edge, causing devices to be unsynchronized in time by exactly one
         * second.
         */
        virtual void set_ticks_next_pps(const uint64_t ticks) = 0;

        /*! Return the current tick rate
         */
        double get_tick_rate()
        {
            return _tick_rate;
        }

    protected:
        /*! Set the tick rate
         *
         * This doesn't change the input clock to the timekeeper, but does two
         * things:
         * - Update the local value of the tick rate, so the time-spec based API
         *   calls work
         * - Convert the tick rate to a period and call set_period()
         */
        void set_tick_rate(const double rate);

        /*! Set the time period as a 64-bit Q32 value
         *
         * \param period_ns The period as nanoseconds per tick, in Q32 format
         */
        virtual void set_period(const uint64_t period_ns) = 0;

    private:
        //! Ticks/Second
        double _tick_rate = 1.0;
    };

    //! Returns the number of timekeepers, which equals the number of timebases
    // on this device.
    //
    // Most USRPs have one timekeeper. Refer to the manual for your device
    // family for potential exceptions to this behavior. Custom FPGA images
    // may also implement multiple timekeepers for various purposes.
    size_t get_num_timekeepers() const;

    //! Return a reference to the \p tk_idx-th timekeeper on this motherboard
    //
    // For most USRPs, timekeeper index 0 is used to access the main timekeeper.
    // When using a USRP with multiple timekeepers, refer to the relevant device
    // family manual for more information on timekeeper mapping and enumeration.
    //
    // Custom FPGA images can implement multiple timekeepers.
    //
    // To make sure that \p tk_idx is a valid value, get_num_timekeepers() can
    // be used to query the number of timekeepers available.
    //
    // \throws uhd::index_error if \p tk_idx is not valid
    timekeeper::sptr get_timekeeper(const size_t tk_idx) const;

    /**************************************************************************
     * Motherboard Control
     *************************************************************************/
    /*! Run initializations of this motherboard that have to occur post-block init
     */
    virtual void init() {}

    /*! Get canonical name for this USRP motherboard
     *
     * \return a string representing the name
     */
    virtual std::string get_mboard_name() const = 0;

    /*!  Set the time source for the USRP device
     *
     * This sets the method of time synchronization, typically a pulse per
     * second signal. In order to time-align multiple USRPs, it is necessary to
     * connect all of them to a common reference and provide them with the same
     * time source.
     * Typical values for \p source are 'internal', 'external'. Refer to the
     * specific device manual for a full list of options.
     *
     * If the value for for \p source is not available for this device, it will
     * throw an exception. Calling get_time_sources() will return a valid list
     * of options for this method.
     *
     * Side effects: Some devices only support certain combinations of time and
     * clock source. It is possible that the underlying device implementation
     * will change the clock source when the time source changes and vice versa.
     * Reading back the current values of clock and time source using
     * get_clock_source() and get_time_source() is the only certain way of
     * knowing which clock and time source are currently selected.
     *
     * This function does not force a re-initialization of the underlying
     * hardware when the value does not change. Consider the following snippet:
     * ~~~{.cpp}
     * auto usrp = uhd::usrp::multi_usrp::make(device_args);
     * // This may or may not cause the hardware to reconfigure, depending on
     * // the default state of the device
     * usrp->set_time_source("internal");
     * // Now, the time source is definitely set to "internal"!
     * // The next call probably won't do anything but will return immediately,
     * // because the time source was already set to "internal"
     * usrp->set_time_source("internal");
     * // The time source is still guaranteed to be "internal" at this point
     * ~~~
     *
     * See also:
     * - set_clock_source()
     * - set_sync_source()
     *
     * \param source a string representing the time source
     * \throws uhd::value_error if \p source is an invalid option
     */
    virtual void set_time_source(const std::string& source) = 0;

    /*! Get the currently set time source
     *
     * \return the string representing the time source
     */
    virtual std::string get_time_source() const = 0;

    /*!
     * Get a list of possible time sources.
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_time_sources() const = 0;

    /*!  Set the clock source for the USRP device
     *
     * This sets the source of the frequency reference, typically a 10 MHz
     * signal. In order to frequency-align multiple USRPs, it is necessary to
     * connect all of them to a common reference and provide them with the same
     * clock source.
     * Typical values for \p source are 'internal', 'external'. Refer to the
     * specific device manual for a full list of options.
     *
     * If the value for for \p source is not available for this device, it will
     * throw an exception. Calling get_clock_sources() will return a valid list
     * of options for this method.
     *
     * Side effects: Some devices only support certain combinations of time and
     * clock source. It is possible that the underlying device implementation
     * will change the time source when the clock source changes and vice versa.
     * Reading back the current values of clock and time source using
     * get_clock_source() and get_time_source() is the only certain way of
     * knowing which clock and time source are currently selected.
     *
     * This function does not force a re-initialization of the underlying
     * hardware when the value does not change. Consider the following snippet:
     * ~~~{.cpp}
     * auto usrp = uhd::usrp::multi_usrp::make(device_args);
     * // This may or may not cause the hardware to reconfigure, depending on
     * // the default state of the device
     * usrp->set_clock_source("internal");
     * // Now, the clock source is definitely set to "internal"!
     * // The next call probably won't do anything but will return immediately,
     * // because the clock source was already set to "internal"
     * usrp->set_clock_source("internal");
     * // The clock source is still guaranteed to be "internal" at this point
     * ~~~
     *
     * See also:
     * - set_time_source()
     * - set_sync_source()
     *
     * \param source a string representing the time source
     * \throws uhd::value_error if \p source is an invalid option
     */
    virtual void set_clock_source(const std::string& source) = 0;

    /*! Get the currently set clock source
     *
     * \return the string representing the clock source
     */
    virtual std::string get_clock_source() const = 0;

    /*! Get a list of possible clock sources
     *
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_clock_sources() const = 0;

    /*! Set the reference/synchronization sources for the USRP device
     *
     * This is a shorthand for calling
     * `set_sync_source(device_addr_t("clock_source=$CLOCK_SOURCE,time_source=$TIME_SOURCE"))`
     *
     * \param clock_source A string representing the clock source
     * \param time_source A string representing the time source
     * \throws uhd::value_error if the sources don't actually exist
     */
    virtual void set_sync_source(
        const std::string& clock_source, const std::string& time_source) = 0;

    /*! Set the reference/synchronization sources for the USRP device
     *
     * Typically, this will set both clock and time source in a single call. For
     * some USRPs, this may be significantly faster than calling
     * set_time_source() and set_clock_source() individually.
     *
     * Example:
     * ~~~{.cpp}
     * auto graph = uhd::rfnoc::rfnoc_graph::make("");
     * graph->get_mb_controller(0)->set_sync_source(
     *     device_addr_t("clock_source=external,time_source=external"));
     * ~~~
     *
     * This function does not force a re-initialization of the underlying
     * hardware when the value does not change. See also set_time_source() and
     * set_clock_source() for more details.
     *
     * \b Note: Reconfiguring the sync source may affect the clocking
     * within the FPGAs of USRPs, and affect timekeeping as well as proper
     * functioning of blocks that depend on these clocks. It is therefore
     * strongly recommended to configure clock and time source before doing
     * anything else. In particular, setting the device time should be done
     * after calling this, and there should be no ongoing streaming operation
     * while reconfiguring the sync source.
     *
     * \param sync_source A dictionary representing the various source settings.
     * \throws uhd::value_error if the sources don't actually exist or if the
     *         combination of clock and time source is invalid.
     */
    virtual void set_sync_source(const uhd::device_addr_t& sync_source) = 0;

    /*! Get the currently set sync source
     *
     * \return the dictionary representing the sync source settings
     */
    virtual uhd::device_addr_t get_sync_source() const = 0;

    /*! Get a list of available sync sources
     *
     * \return the dictionary representing the sync source settings
     */
    virtual std::vector<uhd::device_addr_t> get_sync_sources() = 0;

    /*! Send the clock source to an output connector
     *
     * This call is only applicable on devices with reference outputs.
     * By default, the reference output will be enabled for ease of use.
     * This call may be used to enable or disable the output.
     * \param enb true to output the clock source.
     */
    virtual void set_clock_source_out(const bool enb) = 0;

    /*! Send the time source to an output connector
     *
     * This call is only applicable on devices with PPS outputs.
     * By default, the PPS output will be enabled for ease of use.
     * This call may be used to enable or disable the output.
     * \param enb true to output the time source.
     */
    virtual void set_time_source_out(const bool enb) = 0;

    /*! Get a motherboard sensor value
     *
     * \param name the name of the sensor
     * \return a sensor value object
     */
    virtual uhd::sensor_value_t get_sensor(const std::string& name) = 0;

    /*! Get a list of possible motherboard sensor names
     *
     * \return a vector of sensor names
     */
    virtual std::vector<std::string> get_sensor_names() = 0;

    /*! Return the motherboard EEPROM data
     */
    virtual uhd::usrp::mboard_eeprom_t get_eeprom() = 0;

    /*! Synchronize a list of motherboards in time and frequency
     *
     * Note: When a uhd::rfnoc_graph is instantiated, this method is called on
     * all motherboards.
     *
     * The exact steps taken when calling this method are hardware-specific, but
     * primarily can be split in three steps in a specific order:
     * 1. Pre-timekeeper sync tasks (things here could affect timekeepers)
     * 2. Timekeeper sync (timekeepers are perfectly aligned after this step)
     * 3. Post-timekeeper sync ( anything here should not affect timekeepers)
     * This ensures that timekeepers will not lose synchronization.
     * In all cases these will ensure that:
     * - Any hardware settings that need to be applied to synchronize will be
     *   configured. For example, the X3x0 DAC (AD9146) requires synchronization
     *   triggers for all the DACs to run synchronously, and the X4x0 RFSoC
     *   requires programming an identical tile latency.
     * - Timekeepers are synchronized. That means that timekeepers with the
     *   same clock rate increment are in unison, and at all times have the same
     *   time. This allows sending timed commands to the motherboards and
     *   expect them to be executed at the same time.
     *
     * \param mb_controllers A list of motherboard controllers to synchronize.
     *                       Any motherboard controllers that could not be
     *                       synchronized because they're incompatible with this
     *                       motherboard controller are removed from the list.
     *                       On return, the list should be (ideally) identical
     *                       to its value at call time.
     * \param time_spec Time specification to synchronize \p mb_controllers to
     * \param quiet If true, don't print any errors or warnings if
     *              synchronization fails. During initialization of uhd::rfnoc_graph,
     *              UHD will call synchronize() on all motherboards with \p quiet
     *              set to true.
     * \returns true if all motherboards that were listed in \p mb_controllers
     *          could be synchronized.
     */
    virtual bool synchronize(std::vector<mb_controller::sptr>& mb_controllers,
        const uhd::time_spec_t& time_spec = uhd::time_spec_t(0.0),
        const bool quiet                  = false);

    /*! Return the list of GPIO banks that are controlled by this MB controller
     *
     * Note that this list may be empty. Only if the MB controller has any
     * control over GPIOs, do the get listed here.
     */
    virtual std::vector<std::string> get_gpio_banks() const;

    /*! Return a list of possible sources to drive GPIOs
     *
     * Sources can be "PS", for when an embedded device can drive the pins from
     * software, "Radio#0", if a radio block can drive them, and so on.
     */
    virtual std::vector<std::string> get_gpio_srcs(const std::string& bank) const;

    /*! Return the current sources for a given GPIO bank
     */
    virtual std::vector<std::string> get_gpio_src(const std::string& bank);

    /*! Set the source for GPIO pins on a given bank.
     *
     * \throws uhd::key_error if the bank does not exist
     * \throws uhd::value_error if the source does not exist
     * \throws uhd::not_implemented_error if the current motherboard does not
     *         support this feature
     */
    virtual void set_gpio_src(
        const std::string& bank, const std::vector<std::string>& src);

    /*! Register a callback function to update sync sources
     *
     * This callback alerts those registered that a change has
     * occurred to a sync source, whether that be the clock_source,
     * time_source, or both via sync_source.
     *
     * \param callback_f The function to call when a sync source is updated
     */
    virtual void register_sync_source_updater(sync_source_updater_t callback_f);

protected:
    /*! Stash away a timekeeper. This needs to be called by the implementer of
     * mb_controller.
     */
    void register_timekeeper(const size_t idx, timekeeper::sptr tk);

private:
    /**************************************************************************
     * Attributes
     *************************************************************************/
    std::unordered_map<size_t, timekeeper::sptr> _timekeepers;
};

}} // namespace uhd::rfnoc
