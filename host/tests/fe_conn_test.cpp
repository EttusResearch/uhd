//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/fe_connection.hpp>
#include <uhd/exception.hpp>
#include <boost/test/unit_test.hpp>

using namespace uhd::usrp;

BOOST_AUTO_TEST_CASE(test_quardrature){
    fe_connection_t IQ("IQ"), QI("QI"), IbQ("IbQ"), QbI("QbI"), QbIb("QbIb");
    BOOST_CHECK(IQ.get_sampling_mode()==fe_connection_t::QUADRATURE);
    BOOST_CHECK(QI.get_sampling_mode()==fe_connection_t::QUADRATURE);
    BOOST_CHECK(IbQ.get_sampling_mode()==fe_connection_t::QUADRATURE);
    BOOST_CHECK(QbI.get_sampling_mode()==fe_connection_t::QUADRATURE);
    BOOST_CHECK(QbIb.get_sampling_mode()==fe_connection_t::QUADRATURE);

    BOOST_CHECK(not IQ.is_iq_swapped());
    BOOST_CHECK(QI.is_iq_swapped());
    BOOST_CHECK(not IbQ.is_iq_swapped());
    BOOST_CHECK(QbI.is_iq_swapped());
    BOOST_CHECK(QbIb.is_iq_swapped());

    BOOST_CHECK(not IQ.is_i_inverted());
    BOOST_CHECK(not QI.is_i_inverted());
    BOOST_CHECK(IbQ.is_i_inverted());
    BOOST_CHECK(not QbI.is_i_inverted());
    BOOST_CHECK(QbIb.is_i_inverted());

    BOOST_CHECK(not IQ.is_q_inverted());
    BOOST_CHECK(not QI.is_q_inverted());
    BOOST_CHECK(not IbQ.is_q_inverted());
    BOOST_CHECK(QbI.is_q_inverted());
    BOOST_CHECK(QbIb.is_q_inverted());
}

BOOST_AUTO_TEST_CASE(test_heterodyne){
    fe_connection_t II("II"), QQ("QQ"), IbIb("IbIb"), QbQb("QbQb");
    BOOST_CHECK(II.get_sampling_mode()==fe_connection_t::HETERODYNE);
    BOOST_CHECK(QQ.get_sampling_mode()==fe_connection_t::HETERODYNE);
    BOOST_CHECK(IbIb.get_sampling_mode()==fe_connection_t::HETERODYNE);
    BOOST_CHECK(QbQb.get_sampling_mode()==fe_connection_t::HETERODYNE);

    BOOST_CHECK(not II.is_iq_swapped());
    BOOST_CHECK(QQ.is_iq_swapped());
    BOOST_CHECK(not IbIb.is_iq_swapped());
    BOOST_CHECK(QbQb.is_iq_swapped());

    BOOST_CHECK(not II.is_i_inverted());
    BOOST_CHECK(not QQ.is_i_inverted());
    BOOST_CHECK(IbIb.is_i_inverted());
    BOOST_CHECK(QbQb.is_i_inverted());

    BOOST_CHECK(not II.is_q_inverted());
    BOOST_CHECK(not QQ.is_q_inverted());
    BOOST_CHECK(IbIb.is_q_inverted());
    BOOST_CHECK(QbQb.is_q_inverted());

    BOOST_CHECK_THROW(fe_connection_t dummy("IIb"), uhd::value_error);
    BOOST_CHECK_THROW(fe_connection_t dummy("IbI"), uhd::value_error);
    BOOST_CHECK_THROW(fe_connection_t dummy("QQb"), uhd::value_error);
    BOOST_CHECK_THROW(fe_connection_t dummy("QbQ"), uhd::value_error);
}

BOOST_AUTO_TEST_CASE(test_real){
    fe_connection_t I("I"), Q("Q"), Ib("Ib"), Qb("Qb");
    BOOST_CHECK(I.get_sampling_mode()==fe_connection_t::REAL);
    BOOST_CHECK(Q.get_sampling_mode()==fe_connection_t::REAL);
    BOOST_CHECK(Ib.get_sampling_mode()==fe_connection_t::REAL);
    BOOST_CHECK(Qb.get_sampling_mode()==fe_connection_t::REAL);

    BOOST_CHECK(not I.is_iq_swapped());
    BOOST_CHECK(Q.is_iq_swapped());
    BOOST_CHECK(not Ib.is_iq_swapped());
    BOOST_CHECK(Qb.is_iq_swapped());

    BOOST_CHECK(not I.is_i_inverted());
    BOOST_CHECK(not Q.is_i_inverted());
    BOOST_CHECK(Ib.is_i_inverted());
    BOOST_CHECK(Qb.is_i_inverted());

    BOOST_CHECK(not I.is_q_inverted());
    BOOST_CHECK(not Q.is_q_inverted());
    BOOST_CHECK(not Ib.is_q_inverted());
    BOOST_CHECK(not Qb.is_q_inverted());
}

BOOST_AUTO_TEST_CASE(test_invalid){
    BOOST_CHECK_THROW(fe_connection_t dummy("blah"), uhd::value_error);
    BOOST_CHECK_THROW(fe_connection_t dummy("123456"), uhd::value_error);
    BOOST_CHECK_THROW(fe_connection_t dummy("ii"), uhd::value_error);
    BOOST_CHECK_THROW(fe_connection_t dummy("qb"), uhd::value_error);
    BOOST_CHECK_THROW(fe_connection_t dummy("IIIQ"), uhd::value_error);
}
