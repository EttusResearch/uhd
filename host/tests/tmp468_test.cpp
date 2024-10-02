//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/serial.hpp>
#include <uhdlib/usrp/common/tmp468.hpp>
#include <boost/test/unit_test.hpp>
#include <vector>

// Mock i2c interface to mimic the behavior of the tmp468 sensor.
class mock_tmp468_i2c : public uhd::i2c_iface
{
public:
    using sptr = std::shared_ptr<mock_tmp468_i2c>;
    mock_tmp468_i2c()
    {
        // Example temperature readings from TMP468 datasheet and some addtional values
        // Expectations for temperature given the data is (in degC)
        _mock_temp_readings = {
            {0xE0, 0x00}, // -64
            {0xF3, 0x80}, // -25
            {0xFF, 0xF0}, // -0.125
            {0x00, 0x00}, // 0
            {0x00, 0x08}, // 0.0625
            {0x05, 0x68}, // 10.8125
            {0x0C, 0x80}, // 25
            {0x57, 0x80}, // 175
            {0x5F, 0x80}, // 191
        };
    }
    ~mock_tmp468_i2c() override = default;

    void write_i2c(uint16_t /*addr*/, const uhd::byte_vector_t& buf) override
    {
        _sensor_addr = buf[0];
        // For sensors with a higher address than writing the pointer register, store the
        // value written to the register in a local register space.
        if (buf[0] > 0x20) {
            _reg_settings[buf[0]] = std::make_pair(buf[1], buf[2]);
        }
    }

    uhd::byte_vector_t read_i2c(uint16_t /*addr*/, size_t /*num_bytes*/) override
    {
        return _mock_temp_readings[_sensor_addr];
    }

    std::pair<uint8_t, uint8_t> get_reg_setting(int8_t reg)
    {
        return _reg_settings[reg];
    }

private:
    std::vector<uhd::byte_vector_t> _mock_temp_readings;
    std::map<int8_t, std::pair<uint8_t, uint8_t>> _reg_settings;
    uint8_t _sensor_addr;
};

// The tmp468 sensor read_temperature function takes in the address of the sensor to write
// and the function writes that sensor to the device, reads back two bytes, and then
// converts those bytes to a temperature reading in degC. The mock i2c interface above is
// used to produce values that are outlined in the datasheet and those values are then
// compared to the expected temperature conversions to ensure the conversion logic is
// correct.
BOOST_AUTO_TEST_CASE(tmp468_temperature_conversion_test)
{
    uhd::i2c_iface::sptr tmp_i2c  = std::make_shared<mock_tmp468_i2c>();
    tmp468_iface::sptr tmp_sensor = tmp468_iface::make(tmp_i2c, 0x00);

    BOOST_CHECK_EQUAL(tmp_sensor->read_temperature(tmp468_iface::LOCAL_SENSOR), -64);
    BOOST_CHECK_EQUAL(tmp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR1), -25);
    BOOST_CHECK_EQUAL(tmp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR2), -0.125);
    BOOST_CHECK_EQUAL(tmp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR3), 0);
    BOOST_CHECK_EQUAL(tmp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR4), 0.0625);
    BOOST_CHECK_EQUAL(
        tmp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR5), 10.8125);
    BOOST_CHECK_EQUAL(tmp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR6), 25);
    BOOST_CHECK_EQUAL(tmp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR7), 175);
    BOOST_CHECK_EQUAL(tmp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR8), 191);
}

// The tmp468 sensor set_ideality_factor function takes in the address of the sensor as
// well as the ideality factor of the diode or transistor being used for the remote
// temperature sensing. The mock tmp468 interface stores a mock register space to read
// back the value set by the set_ideality_factor function. The expected values are using
// examples from the datasheet as well as a few additional values.
BOOST_AUTO_TEST_CASE(tmp468_ideality_factor_test)
{
    mock_tmp468_i2c::sptr tmp_i2c = std::make_shared<mock_tmp468_i2c>();
    tmp468_iface::sptr tmp_sensor = tmp468_iface::make(tmp_i2c, 0x00);

    tmp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR1, 1.008);
    BOOST_CHECK_EQUAL(tmp_i2c->get_reg_setting(0x41).first, 0x00);
    tmp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR2, 0.950205);
    BOOST_CHECK_EQUAL(tmp_i2c->get_reg_setting(0x49).first, 0x7F);
    tmp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR3, 1.073829);
    BOOST_CHECK_EQUAL(tmp_i2c->get_reg_setting(0x51).first, 0x80);
    tmp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR4, 1.005112);
    BOOST_CHECK_EQUAL(tmp_i2c->get_reg_setting(0x59).first, 0x06);
    tmp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR5, 1.009935);
    BOOST_CHECK_EQUAL(tmp_i2c->get_reg_setting(0x61).first, 0xFC);
    tmp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR6, 1.003195);
    BOOST_CHECK_EQUAL(tmp_i2c->get_reg_setting(0x69).first, 0x0A);
    tmp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR7, 1.002);
    BOOST_CHECK_EQUAL(tmp_i2c->get_reg_setting(0x71).first, 0x0D);
    tmp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR8, 1.062);
    BOOST_CHECK_EQUAL(tmp_i2c->get_reg_setting(0x79).first, 0x96);

    // Test the min and max values for the ideality factor
    tmp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR1, 0.5);
    BOOST_CHECK_EQUAL(tmp_i2c->get_reg_setting(0x41).first, 0x7F);
    tmp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR1, 2.0);
    BOOST_CHECK_EQUAL(tmp_i2c->get_reg_setting(0x41).first, 0x80);
}
