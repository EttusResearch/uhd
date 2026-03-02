//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include "hbx_constants.hpp"
#include <uhdlib/usrp/dboard/hbx/hbx_cpld_ctrl.hpp>
#include <map>

namespace uhd { namespace usrp { namespace hbx {

/*! HBX LO Power Detector class
 *
 * This class implements the SPI transactions and calibration
 * for the LO power detectors used on HBX daughterboards.
 * It uses a combination of ADS8862 ADC and the LTC5582 power detector.
 * Calculations depend on the schematic and are thus HBX-specific.
 */
class hbx_lo_pd final : public hbx_cpld_ctrl::spi_transactor
{
public:
    // Pass in our power detector address and poke/peek functions
    hbx_lo_pd(size_t start_address,
        hbx_cpld_ctrl::poke_fn_type&& poke_fn,
        hbx_cpld_ctrl::peek_fn_type&& peek_fn);

    /*! Read raw ADC data
     *
     * \return Raw ADC data as uint16_t
     */
    uint16_t read_data_raw();

    /*! Read ADC data and convert to dBm
     *
     * \param frequency Frequency in Hz for calibration lookup
     * \return Converted power value in dBm
     */
    double read_data_dbm(const double frequency);

private:
    const std::string _log_id = "HBX_LO_PD";

    /*********************************************
     * Attributes
     *********************************************/
    // ADC and voltage conversion constants
    // ADC resolution from ADS8862
    static constexpr uint32_t ADC_RESOLUTION = 1 << 16; // 16-bit ADC resolution
    // Values depend on HBX schematic and components used
    static constexpr double VOLTAGE_CORRECTION_FACTOR =
        1.01; // Voltage divider correction factor
    static constexpr double REFERENCE_VOLTAGE = 2.5; // ADC reference voltage in volts

    // Slope data: frequency (GHz) -> slope (mV/dB) from Plot Digitizer
    static constexpr std::array<double, 49> x_slope_freq = {{4.436975055152143,
        5.014005426399135,
        5.518207089388992,
        5.798319124383357,
        4.056022056297707,
        3.6302515526855728,
        3.2268908535557843,
        0.4628313753395006,
        0.5619479415682759,
        0.712777498872934,
        0.7903469854867582,
        0.8851541357925438,
        1.0101271975592605,
        1.1264814274799968,
        1.2299074096317624,
        1.3247145599375474,
        1.419521710243332,
        1.5358759401640683,
        1.630683090469854,
        1.7211808248526483,
        1.8159879751584334,
        1.9323422050791697,
        2.022839939461965,
        2.1521224171516717,
        2.2641672311494183,
        2.3632837973781937,
        2.4753286113759385,
        2.6003016731426563,
        2.725274734909372,
        2.824391301138147,
        2.9235078673669226,
        3.022624433595699,
        3.1088127520555036,
        3.302736468590064,
        3.4018530348188385,
        3.4966601851246244,
        3.7940098838109497,
        3.8888170341167347,
        4.194785564649042,
        4.306830378646787,
        4.5179917588733085,
        4.642964820640025,
        4.724843723176841,
        4.854126200866547,
        4.9403145193263525,
        5.1514758995528735,
        5.25921129762763,
        5.3497090320104235,
        5.681534058080671}};

    static constexpr std::array<double, 49> y_slope = {{30.495327124948087,
        30.685981243208413,
        30.85140182302482,
        30.93831768073437,
        30.369158774549263,
        30.220560860845772,
        30.05514014939352,
        29.441480955913182,
        29.39403307877155,
        29.335801593188634,
        29.320704541370844,
        29.331488149812124,
        29.361682253447707,
        29.39834652214806,
        29.42638390409539,
        29.467361616172254,
        29.499712441496094,
        29.54069015357296,
        29.575197700585054,
        29.614018690973666,
        29.65068295967402,
        29.685190506686112,
        29.7153846103217,
        29.756362322398562,
        29.78655642603415,
        29.79734003447543,
        29.80165347785194,
        29.81243708629322,
        29.84478791161706,
        29.88360890200567,
        29.92027317070602,
        29.95909416109463,
        29.99791515148324,
        30.086340740701736,
        30.125161731090348,
        30.1618259997907,
        30.278288970956524,
        30.31279651796862,
        30.416319159004914,
        30.452983427705263,
        30.524155243417713,
        30.562976233806324,
        30.588856894065394,
        30.63414804951877,
        30.6621854314661,
        30.731200525490294,
        30.76570807250239,
        30.79805889782623,
        30.905894982239033}};

    // Log Intercept data: frequency (GHz) -> intercept (dBm) from Plot Digitizer
    static constexpr std::array<double, 55> x_log_intercept_freq = {{0.4559270516717324,
        0.5790273556230999,
        0.7112462006079023,
        0.8343465045592701,
        0.9528875379939205,
        1.0623100303951367,
        1.1945288753799386,
        1.2948328267477203,
        1.4224924012158053,
        1.522796352583586,
        1.6550151975683884,
        1.7507598784194527,
        1.8738601823708205,
        1.9924012158054705,
        2.097264437689969,
        2.1930091185410325,
        2.325227963525835,
        2.4300911854103338,
        2.5531914893617014,
        2.648936170212765,
        2.7629179331306983,
        2.8541033434650447,
        2.9452887537993924,
        3.0364741641337387,
        3.159574468085107,
        3.2553191489361692,
        3.3419452887537986,
        3.428571428571429,
        3.5243161094224917,
        3.6200607902735564,
        3.7158054711246193,
        3.797872340425531,
        3.9164133738601823,
        4.066869300911853,
        4.144376899696049,
        4.240121580547111,
        4.322188449848023,
        4.417933130699088,
        4.5,
        4.5866261398176285,
        4.66869300911854,
        4.759878419452887,
        4.846504559270516,
        4.928571428571427,
        5.006079027355623,
        5.092705167173251,
        5.170212765957446,
        5.2750759878419435,
        5.348024316109421,
        5.452887537993921,
        5.530395136778115,
        5.630699088145897,
        5.703647416413372,
        5.785714285714285}};

    static constexpr std::array<double, 55> y_log_intercept = {{-86.21699544764795,
        -86.3535660091047,
        -86.47647951441579,
        -86.47647951441579,
        -86.39453717754174,
        -86.21699544764795,
        -86.09408194233687,
        -85.9165402124431,
        -85.77996965098635,
        -85.6433990895296,
        -85.43854324734446,
        -85.27465857359635,
        -85.12443095599393,
        -84.94688922610015,
        -84.8103186646434,
        -84.64643399089529,
        -84.45523520485584,
        -84.30500758725341,
        -84.12746585735964,
        -83.92261001517451,
        -83.69044006069802,
        -83.47192716236722,
        -83.28072837632777,
        -83.02124430955993,
        -82.7617602427921,
        -82.55690440060698,
        -82.28376327769348,
        -82.05159332321699,
        -81.79210925644917,
        -81.51896813353565,
        -81.25948406676783,
        -81.02731411229135,
        -80.64491654021245,
        -80.20789074355083,
        -79.93474962063733,
        -79.70257966616084,
        -79.42943854324734,
        -79.14264036418817,
        -78.896813353566,
        -78.58270106221548,
        -78.350531107739,
        -78.05007587253414,
        -77.77693474962064,
        -77.51745068285281,
        -77.2443095599393,
        -76.95751138088012,
        -76.71168437025797,
        -76.37025796661608,
        -76.15174506828528,
        -75.79666160849773,
        -75.59180576631259,
        -75.26403641881639,
        -75.04552352048559,
        -74.74506828528072}};


    /*********************************************
     * Private Methods
     *********************************************/
    std::map<double, double> slope_lut;
    std::map<double, double> intercept_lut;

    // Returns the calibration data depending on the LO frequency
    std::pair<double, double> get_calibration(double freq)
    {
        // Only allow LO frequencies within LMX2572 range
        if (freq < LMX2572_MIN_FREQ || freq > LMX2572_MAX_FREQ) {
            throw uhd::runtime_error("LO frequency " + std::to_string(freq)
                                     + " Hz is out of range for HBX LO PD");
        }
        auto find_closest = [](const std::map<double, double>& lut, double target) {
            if (lut.empty()) {
                throw uhd::runtime_error("Calibration LUT is empty");
            }

            // Find first element >= target
            auto it = lut.lower_bound(target);

            // If we're at the beginning, first element is closest
            if (it == lut.begin()) {
                return it;
            }

            // If we're at the end, last element is closest
            if (it == lut.end()) {
                return std::prev(it);
            }

            // Compare with previous element to find which is closer
            auto prev_it     = std::prev(it);
            double diff_cur  = std::abs(it->first - target);
            double diff_prev = std::abs(prev_it->first - target);

            return (diff_prev < diff_cur) ? prev_it : it;
        };

        double closest_slope     = find_closest(slope_lut, freq)->second;
        double closest_intercept = find_closest(intercept_lut, freq)->second;

        return {closest_slope, closest_intercept};
    }

    /*
     * This is based on the Slope vs. Frequency and Logarithmic Intercept vs. Frequency
     * graphs provided in
     * https://www.analog.com/media/en/technical-documentation/data-sheets/LTC5582.pdf
     * (page 7). The plot for 25°C was digitized, so smaller deviations may exist for
     * other temperatures.
     */
    double adc_to_dbm(const uint16_t adc, std::pair<double, double> calibration)
    {
        double adc_value = static_cast<double>(adc);
        double voltage =
            (adc_value / ADC_RESOLUTION) * VOLTAGE_CORRECTION_FACTOR * REFERENCE_VOLTAGE;
        // `calibration.first` is slope in V/dB
        // `calibration.second` is log_intercept in dBm
        double power_dbm = (voltage / calibration.first) + calibration.second;
        return power_dbm;
    }
};
}}} // namespace uhd::usrp::hbx
