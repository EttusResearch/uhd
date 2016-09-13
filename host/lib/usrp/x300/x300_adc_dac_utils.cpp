//
// Copyright 2015 Ettus Research LLC
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

#include "x300_impl.hpp"
#include <boost/date_time/posix_time/posix_time_io.hpp>

using namespace uhd::usrp::x300;

/***********************************************************************
 * DAC: Reset and synchronization operations
 **********************************************************************/

void x300_impl::synchronize_dacs(const std::vector<radio_perifs_t*>& radios)
{
    if (radios.size() < 2) return;  //Nothing to synchronize

    //**PRECONDITION**
    //This function assumes that all the VITA times in "radios" are synchronized
    //to a common reference. Currently, this function is called in get_tx_stream
    //which also has the same precondition.

    //Reinitialize and resync all DACs
    for (size_t i = 0; i < radios.size(); i++) {
        radios[i]->dac->reset();
    }

    //Get a rough estimate of the cumulative command latency
    boost::posix_time::ptime t_start = boost::posix_time::microsec_clock::local_time();
    for (size_t i = 0; i < radios.size(); i++) {
        radios[i]->ctrl->peek64(uhd::usrp::radio::RB64_TIME_NOW); //Discard value. We are just timing the call
    }
    boost::posix_time::time_duration t_elapsed =
        boost::posix_time::microsec_clock::local_time() - t_start;

    //Add 100% of headroom + uncertaintly to the command time
    boost::uint64_t t_sync_us = (t_elapsed.total_microseconds() * 2) + 13000 /*Scheduler latency*/;

    //Pick radios[0] as the time reference.
    uhd::time_spec_t sync_time =
        radios[0]->time64->get_time_now() + uhd::time_spec_t(((double)t_sync_us)/1e6);

    //Send the sync command
    for (size_t i = 0; i < radios.size(); i++) {
        radios[i]->ctrl->set_time(sync_time);
        radios[i]->ctrl->poke32(uhd::usrp::radio::sr_addr(uhd::usrp::radio::DACSYNC), 0x1);    //Arm FRAMEP/N sync pulse
        radios[i]->ctrl->set_time(uhd::time_spec_t(0.0));   //Clear command time
    }

    //Wait and check status
    boost::this_thread::sleep(boost::posix_time::microseconds(t_sync_us));
    for (size_t i = 0; i < radios.size(); i++) {
        radios[i]->dac->verify_sync();
    }
}

/***********************************************************************
 * ADC: Self-test operations
 **********************************************************************/

static void check_adc(uhd::wb_iface::sptr iface, const boost::uint32_t val, const boost::uint32_t i)
{
    boost::uint32_t adc_rb = iface->peek32(uhd::usrp::radio::RB32_RX);
    adc_rb ^= 0xfffc0000; //adapt for I inversion in FPGA
    if (val != adc_rb) {
        throw uhd::runtime_error(
            (boost::format("ADC self-test failed for Radio%d. (Exp=0x%x, Got=0x%x)")%i%val%adc_rb).str());
    }
}

void x300_impl::self_test_adcs(mboard_members_t& mb, boost::uint32_t ramp_time_ms) {
    for (size_t r = 0; r < mboard_members_t::NUM_RADIOS; r++) {
        radio_perifs_t &perif = mb.radio_perifs[r];

        //First test basic patterns
        perif.adc->set_test_word("ones", "ones"); check_adc(perif.ctrl, 0xfffcfffc,r);
        perif.adc->set_test_word("zeros", "zeros"); check_adc(perif.ctrl, 0x00000000,r);
        perif.adc->set_test_word("ones", "zeros"); check_adc(perif.ctrl, 0xfffc0000,r);
        perif.adc->set_test_word("zeros", "ones"); check_adc(perif.ctrl, 0x0000fffc,r);
        for (size_t k = 0; k < 14; k++)
        {
            perif.adc->set_test_word("zeros", "custom", 1 << k);
            check_adc(perif.ctrl, 1 << (k+2),r);
        }
        for (size_t k = 0; k < 14; k++)
        {
            perif.adc->set_test_word("custom", "zeros", 1 << k);
            check_adc(perif.ctrl, 1 << (k+18),r);
        }

        //Turn on ramp pattern test
        perif.adc->set_test_word("ramp", "ramp");
        perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
        perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
    }
    boost::this_thread::sleep(boost::posix_time::milliseconds(ramp_time_ms));

    bool passed = true;
    std::string status_str;
    for (size_t r = 0; r < mboard_members_t::NUM_RADIOS; r++) {
        radio_perifs_t &perif = mb.radio_perifs[r];
        perif.regmap->misc_ins_reg.refresh();

        std::string i_status, q_status;
        if (perif.regmap->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_LOCKED))
            if (perif.regmap->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_ERROR))
                i_status = "Bit Errors!";
            else
                i_status = "Good";
        else
            i_status = "Not Locked!";

        if (perif.regmap->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_LOCKED))
            if (perif.regmap->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_ERROR))
                q_status = "Bit Errors!";
            else
                q_status = "Good";
        else
            q_status = "Not Locked!";

        passed = passed && (i_status == "Good") && (q_status == "Good");
        status_str += (boost::format(", ADC%d_I=%s, ADC%d_Q=%s")%r%i_status%r%q_status).str();

        //Return to normal mode
        perif.adc->set_test_word("normal", "normal");
    }

    if (not passed) {
        throw uhd::runtime_error(
            (boost::format("ADC self-test failed! Ramp checker status: {%s}")%status_str.substr(2)).str());
    }
}

void x300_impl::extended_adc_test(mboard_members_t& mb, double duration_s)
{
    static const size_t SECS_PER_ITER = 5;
    UHD_MSG(status) << boost::format("Running Extended ADC Self-Test (Duration=%.0fs, %ds/iteration)...\n")
        % duration_s % SECS_PER_ITER;

    size_t num_iters = static_cast<size_t>(ceil(duration_s/SECS_PER_ITER));
    size_t num_failures = 0;
    for (size_t iter = 0; iter < num_iters; iter++) {
        //Print date and time
        boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%d-%b-%Y %H:%M:%S");
        std::ostringstream time_strm;
        time_strm.imbue(std::locale(std::locale::classic(), facet));
        time_strm << boost::posix_time::second_clock::local_time();
        //Run self-test
        UHD_MSG(status) << boost::format("-- [%s] Iteration %06d... ") % time_strm.str() % (iter+1);
        try {
            self_test_adcs(mb, SECS_PER_ITER*1000);
            UHD_MSG(status) << "passed" << std::endl;
        } catch(std::exception &e) {
            num_failures++;
            UHD_MSG(status) << e.what() << std::endl;
        }

    }
    if (num_failures == 0) {
        UHD_MSG(status) << "Extended ADC Self-Test PASSED\n";
    } else {
        throw uhd::runtime_error(
                (boost::format("Extended ADC Self-Test FAILED!!! (%d/%d failures)\n") % num_failures % num_iters).str());
    }
}

/***********************************************************************
 * ADC: Self-calibration operations
 **********************************************************************/

void x300_impl::self_cal_adc_capture_delay(mboard_members_t& mb, const size_t radio_i, bool print_status)
{
    radio_perifs_t& perif = mb.radio_perifs[radio_i];
    if (print_status) UHD_MSG(status) << "Running ADC capture delay self-cal..." << std::flush;

    static const boost::uint32_t NUM_DELAY_STEPS = 32;   //The IDELAYE2 element has 32 steps
    static const boost::uint32_t NUM_RETRIES     = 2;    //Retry self-cal if it fails in warmup situations
    static const boost::int32_t  MIN_WINDOW_LEN  = 4;

    boost::int32_t win_start = -1, win_stop = -1;
    boost::uint32_t iter = 0;
    while (iter++ < NUM_RETRIES) {
        for (boost::uint32_t dly_tap = 0; dly_tap < NUM_DELAY_STEPS; dly_tap++) {
            //Apply delay
            perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_VAL, dly_tap);
            perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 1);
            perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 0);

            boost::uint32_t err_code = 0;

            // -- Test I Channel --
            //Put ADC in ramp test mode. Tie the other channel to all ones.
            perif.adc->set_test_word("ramp", "ones");
            //Turn on the pattern checker in the FPGA. It will lock when it sees a zero
            //and count deviations from the expected value
            perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
            perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
            //10ms @ 200MHz = 2 million samples
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
            if (perif.regmap->misc_ins_reg.read(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_I_LOCKED)) {
                err_code += perif.regmap->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_I_ERROR);
            } else {
                err_code += 100;    //Increment error code by 100 to indicate no lock
            }

            // -- Test Q Channel --
            //Put ADC in ramp test mode. Tie the other channel to all ones.
            perif.adc->set_test_word("ones", "ramp");
            //Turn on the pattern checker in the FPGA. It will lock when it sees a zero
            //and count deviations from the expected value
            perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
            perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
            //10ms @ 200MHz = 2 million samples
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
            if (perif.regmap->misc_ins_reg.read(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_Q_LOCKED)) {
                err_code += perif.regmap->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_Q_ERROR);
            } else {
                err_code += 100;    //Increment error code by 100 to indicate no lock
            }

            if (err_code == 0) {
                if (win_start == -1) {      //This is the first window
                    win_start = dly_tap;
                    win_stop = dly_tap;
                } else {                    //We are extending the window
                    win_stop = dly_tap;
                }
            } else {
                if (win_start != -1) {      //A valid window turned invalid
                    if (win_stop - win_start >= MIN_WINDOW_LEN) {
                        break;              //Valid window found
                    } else {
                        win_start = -1;     //Reset window
                    }
                }
            }
            //UHD_MSG(status) << (boost::format("CapTap=%d, Error=%d\n") % dly_tap % err_code);
        }

        //Retry the self-cal if it fails
        if ((win_start == -1 || (win_stop - win_start) < MIN_WINDOW_LEN) && iter < NUM_RETRIES /*not last iteration*/) {
            win_start = -1;
            win_stop = -1;
            boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
        } else {
            break;
        }
    }
    perif.adc->set_test_word("normal", "normal");
    perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);

    if (win_start == -1) {
        throw uhd::runtime_error("self_cal_adc_capture_delay: Self calibration failed. Convergence error.");
    }

    if (win_stop-win_start < MIN_WINDOW_LEN) {
        throw uhd::runtime_error("self_cal_adc_capture_delay: Self calibration failed. Valid window too narrow.");
    }

    boost::uint32_t ideal_tap = (win_stop + win_start) / 2;
    perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_VAL, ideal_tap);
    perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 1);
    perif.regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 0);

    if (print_status) {
        double tap_delay = (1.0e12 / mb.clock->get_master_clock_rate()) / (2*32); //in ps
        UHD_MSG(status) << boost::format(" done (Tap=%d, Window=%d, TapDelay=%.3fps, Iter=%d)\n") % ideal_tap % (win_stop-win_start) % tap_delay % iter;
    }
}

double x300_impl::self_cal_adc_xfer_delay(mboard_members_t& mb, bool apply_delay)
{
    UHD_MSG(status) << "Running ADC transfer delay self-cal: " << std::flush;

    //Effective resolution of the self-cal.
    static const size_t NUM_DELAY_STEPS = 100;

    double master_clk_period = (1.0e9 / mb.clock->get_master_clock_rate()); //in ns
    double delay_start = 0.0;
    double delay_range = 2 * master_clk_period;
    double delay_incr = delay_range / NUM_DELAY_STEPS;

    UHD_MSG(status) << "Measuring..." << std::flush;
    double cached_clk_delay = mb.clock->get_clock_delay(X300_CLOCK_WHICH_ADC0);
    double fpga_clk_delay = mb.clock->get_clock_delay(X300_CLOCK_WHICH_FPGA);

    //Iterate through several values of delays and measure ADC data integrity
    std::vector< std::pair<double,bool> > results;
    for (size_t i = 0; i < NUM_DELAY_STEPS; i++) {
        //Delay the ADC clock (will set both Ch0 and Ch1 delays)
        double delay = mb.clock->set_clock_delay(X300_CLOCK_WHICH_ADC0, delay_incr*i + delay_start);
        wait_for_clk_locked(mb, fw_regmap_t::clk_status_reg_t::LMK_LOCK, 0.1);

        boost::uint32_t err_code = 0;
        for (size_t r = 0; r < mboard_members_t::NUM_RADIOS; r++) {
            //Test each channel (I and Q) individually so as to not accidentally trigger
            //on the data from the other channel if there is a swap

            // -- Test I Channel --
            //Put ADC in ramp test mode. Tie the other channel to all ones.
            mb.radio_perifs[r].adc->set_test_word("ramp", "ones");
            //Turn on the pattern checker in the FPGA. It will lock when it sees a zero
            //and count deviations from the expected value
            mb.radio_perifs[r].regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
            mb.radio_perifs[r].regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
            //50ms @ 200MHz = 10 million samples
            boost::this_thread::sleep(boost::posix_time::milliseconds(50));
            if (mb.radio_perifs[r].regmap->misc_ins_reg.read(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_LOCKED)) {
                err_code += mb.radio_perifs[r].regmap->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_ERROR);
            } else {
                err_code += 100;    //Increment error code by 100 to indicate no lock
            }

            // -- Test Q Channel --
            //Put ADC in ramp test mode. Tie the other channel to all ones.
            mb.radio_perifs[r].adc->set_test_word("ones", "ramp");
            //Turn on the pattern checker in the FPGA. It will lock when it sees a zero
            //and count deviations from the expected value
            mb.radio_perifs[r].regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
            mb.radio_perifs[r].regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
            //50ms @ 200MHz = 10 million samples
            boost::this_thread::sleep(boost::posix_time::milliseconds(50));
            if (mb.radio_perifs[r].regmap->misc_ins_reg.read(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_LOCKED)) {
                err_code += mb.radio_perifs[r].regmap->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_ERROR);
            } else {
                err_code += 100;    //Increment error code by 100 to indicate no lock
            }
        }
        //UHD_MSG(status) << (boost::format("XferDelay=%fns, Error=%d\n") % delay % err_code);
        results.push_back(std::pair<double,bool>(delay, err_code==0));
    }

    //Calculate the valid window
    int win_start_idx = -1, win_stop_idx = -1, cur_start_idx = -1, cur_stop_idx = -1;
    for (size_t i = 0; i < results.size(); i++) {
        std::pair<double,bool>& item = results[i];
        if (item.second) {  //If data is stable
            if (cur_start_idx == -1) {  //This is the first window
                cur_start_idx = i;
                cur_stop_idx = i;
            } else {                    //We are extending the window
                cur_stop_idx = i;
            }
        } else {
            if (cur_start_idx == -1) {  //We haven't yet seen valid data
                //Do nothing
            } else if (win_start_idx == -1) {   //We passed the first valid window
                win_start_idx = cur_start_idx;
                win_stop_idx = cur_stop_idx;
            } else {                    //Update cached window if current window is larger
                double cur_win_len = results[cur_stop_idx].first - results[cur_start_idx].first;
                double cached_win_len = results[win_stop_idx].first - results[win_start_idx].first;
                if (cur_win_len > cached_win_len) {
                    win_start_idx = cur_start_idx;
                    win_stop_idx = cur_stop_idx;
                }
            }
            //Reset current window
            cur_start_idx = -1;
            cur_stop_idx = -1;
        }
    }
    if (win_start_idx == -1) {
        throw uhd::runtime_error("self_cal_adc_xfer_delay: Self calibration failed. Convergence error.");
    }

    double win_center = (results[win_stop_idx].first + results[win_start_idx].first) / 2.0;
    double win_length = results[win_stop_idx].first - results[win_start_idx].first;
    if (win_length < master_clk_period/4) {
        throw uhd::runtime_error("self_cal_adc_xfer_delay: Self calibration failed. Valid window too narrow.");
    }

    //Cycle slip the relative delay by a clock cycle to prevent sample misalignment
    //fpga_clk_delay > 0 and 0 < win_center < 2*(1/MCR) so one cycle slip is all we need
    bool cycle_slip = (win_center-fpga_clk_delay >= master_clk_period);
    if (cycle_slip) {
        win_center -= master_clk_period;
    }

    if (apply_delay) {
        UHD_MSG(status) << "Validating..." << std::flush;
        //Apply delay
        win_center = mb.clock->set_clock_delay(X300_CLOCK_WHICH_ADC0, win_center);  //Sets ADC0 and ADC1
        wait_for_clk_locked(mb, fw_regmap_t::clk_status_reg_t::LMK_LOCK, 0.1);
        //Validate
        self_test_adcs(mb, 2000);
    } else {
        //Restore delay
        mb.clock->set_clock_delay(X300_CLOCK_WHICH_ADC0, cached_clk_delay);  //Sets ADC0 and ADC1
    }

    //Teardown
    for (size_t r = 0; r < mboard_members_t::NUM_RADIOS; r++) {
        mb.radio_perifs[r].adc->set_test_word("normal", "normal");
        mb.radio_perifs[r].regmap->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
    }
    UHD_MSG(status) << (boost::format(" done (FPGA->ADC=%.3fns%s, Window=%.3fns)\n") %
        (win_center-fpga_clk_delay) % (cycle_slip?" +cyc":"") % win_length);

    return win_center;
}
