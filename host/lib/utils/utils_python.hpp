//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/utils/chdr/chdr_packet.hpp>
#include <uhd/utils/pybind_adaptors.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace uhd::utils::chdr;
namespace chdr_rfnoc = uhd::rfnoc::chdr;

template <typename payload_t>
void export_utils_with_payload(
    pybind11::class_<chdr_packet>& packet_class, std::string packet_name)
{
    std::string get_payload_name("get_payload_");
    get_payload_name.append(packet_name);

    std::string to_string_with_payload_name("to_string_with_payload_");
    to_string_with_payload_name.append(packet_name);

    packet_class
        .def(py::init([](uhd::rfnoc::chdr_w_t chdr_w,
                          chdr_rfnoc::chdr_header header,
                          payload_t payload,
                          boost::optional<uint64_t> timestamp,
                          std::vector<uint64_t> metadata) {
            return chdr_packet(chdr_w, header, payload, timestamp, metadata);
        }),
            py::arg("chdr_w"),
            py::arg("header"),
            py::arg("payload"),
            py::arg("timestamp") = boost::optional<uint64_t>(),
            py::arg("metadata")  = std::vector<uint64_t>())
        .def(get_payload_name.c_str(),
            &chdr_packet::get_payload<payload_t>,
            py::arg("endianness") = uhd::ENDIANNESS_LITTLE)
        .def("set_payload",
            &chdr_packet::set_payload<payload_t>,
            py::arg("payload"),
            py::arg("endianness") = uhd::ENDIANNESS_LITTLE)
        .def(to_string_with_payload_name.c_str(),
            &chdr_packet::to_string_with_payload<payload_t>,
            py::arg("endianness") = uhd::ENDIANNESS_LITTLE);
}

void export_utils(py::module& m)
{
    py::enum_<uhd::endianness_t>(m, "Endianness")
        .value("LITTLE", uhd::ENDIANNESS_LITTLE)
        .value("BIG", uhd::ENDIANNESS_BIG);

    py::enum_<uhd::rfnoc::chdr_w_t>(m, "ChdrWidth")
        .value("W64", uhd::rfnoc::CHDR_W_64)
        .value("W128", uhd::rfnoc::CHDR_W_128)
        .value("W256", uhd::rfnoc::CHDR_W_256)
        .value("W512", uhd::rfnoc::CHDR_W_512);

    pybind11::class_<chdr_packet> packet_class =
        py::class_<chdr_packet>(m, "ChdrPacket")
            .def(py::init([](uhd::rfnoc::chdr_w_t chdr_w,
                              chdr_rfnoc::chdr_header header,
                              py::bytes& payload,
                              boost::optional<uint64_t> timestamp,
                              py::bytes& metadata) {
                return chdr_packet(chdr_w,
                    header,
                    pybytes_to_vector(payload),
                    timestamp,
                    pybytes_to_u64_vector(metadata));
            }),
                py::arg("chdr_w"),
                py::arg("header"),
                py::arg("payload"),
                py::arg("timestamp") = boost::optional<uint64_t>(),
                py::arg("metadata")  = py::bytes())
            // Methods
            .def("__str__", &chdr_packet::to_string)
            .def("__repr__", &chdr_packet::to_string)
            .def("get_header", &chdr_packet::get_header)
            .def("set_header", &chdr_packet::set_header)
            .def("get_timestamp", &chdr_packet::get_timestamp)
            .def("set_timestamp", &chdr_packet::set_timestamp)
            .def("get_metadata", &chdr_packet::get_metadata)
            .def("serialize",
                &chdr_packet::serialize_to_byte_vector,
                py::arg("endianness") = uhd::ENDIANNESS_LITTLE)
            .def_static(
                "deserialize",
                [](uhd::rfnoc::chdr_w_t chdr_w,
                    std::vector<uint8_t> bytes,
                    uhd::endianness_t endianness) {
                    return chdr_packet::deserialize(
                        chdr_w, bytes.begin(), bytes.end(), endianness);
                },
                py::arg("chdr_w"),
                py::arg("bytes"),
                py::arg("endianness") = uhd::ENDIANNESS_LITTLE)
            .def_static(
                "deserialize",
                [](uhd::rfnoc::chdr_w_t chdr_w,
                    py::bytes bytes,
                    uhd::endianness_t endianness) {
                    std::vector<uint8_t> vector = pybytes_to_vector(bytes);
                    return chdr_packet::deserialize(
                        chdr_w, vector.begin(), vector.end(), endianness);
                },
                py::arg("chdr_w"),
                py::arg("bytes"),
                py::arg("endianness") = uhd::ENDIANNESS_LITTLE)
            .def("get_packet_len", &chdr_packet::get_packet_len)
            .def("get_payload_bytes", &chdr_packet::get_payload_bytes)
            .def("set_payload_bytes", &chdr_packet::set_payload_bytes)
            .def("set_payload_bytes", [](chdr_packet& self, py::bytes bytes) {
                auto bytes_vector = pybytes_to_vector(bytes);
                self.set_payload_bytes(bytes_vector);
            });

    export_utils_with_payload<chdr_rfnoc::ctrl_payload>(packet_class, "ctrl");
    export_utils_with_payload<chdr_rfnoc::mgmt_payload>(packet_class, "mgmt");
    export_utils_with_payload<chdr_rfnoc::strs_payload>(packet_class, "strs");
    export_utils_with_payload<chdr_rfnoc::strc_payload>(packet_class, "strc");

    py::class_<chdr_rfnoc::chdr_header>(m, "ChdrHeader")
        // Constructor
        .def(py::init<>())

        // Methods
        .def("__str__", &chdr_rfnoc::chdr_header::to_string)
        .def("__repr__", &chdr_rfnoc::chdr_header::to_string)

        // Properties
        .def_property(
            "vc", &chdr_rfnoc::chdr_header::get_vc, &chdr_rfnoc::chdr_header::set_vc)
        .def_property(
            "eob", &chdr_rfnoc::chdr_header::get_eob, &chdr_rfnoc::chdr_header::set_eob)
        .def_property(
            "eov", &chdr_rfnoc::chdr_header::get_eov, &chdr_rfnoc::chdr_header::set_eov)
        .def_property("pkt_type",
            &chdr_rfnoc::chdr_header::get_pkt_type,
            &chdr_rfnoc::chdr_header::set_pkt_type)
        .def_property("seq_num",
            &chdr_rfnoc::chdr_header::get_seq_num,
            &chdr_rfnoc::chdr_header::set_seq_num)
        .def_property("length",
            &chdr_rfnoc::chdr_header::get_length,
            &chdr_rfnoc::chdr_header::set_length)
        .def_property("dst_epid",
            &chdr_rfnoc::chdr_header::get_dst_epid,
            &chdr_rfnoc::chdr_header::set_dst_epid)
        .def_property("num_mdata",
            &chdr_rfnoc::chdr_header::get_num_mdata,
            &chdr_rfnoc::chdr_header::set_num_mdata)
        .def("pack", &chdr_rfnoc::chdr_header::pack);

    py::enum_<chdr_rfnoc::packet_type_t>(m, "PacketType")
        .value("MGMT", chdr_rfnoc::PKT_TYPE_MGMT)
        .value("STRS", chdr_rfnoc::PKT_TYPE_STRS)
        .value("STRC", chdr_rfnoc::PKT_TYPE_STRC)
        .value("CTRL", chdr_rfnoc::PKT_TYPE_CTRL)
        .value("DATA_NO_TS", chdr_rfnoc::PKT_TYPE_DATA_NO_TS)
        .value("DATA_WITH_TS", chdr_rfnoc::PKT_TYPE_DATA_WITH_TS);

    py::class_<chdr_rfnoc::ctrl_payload>(m, "CtrlPayload")
        // Constructor
        .def(py::init<>())

        // Methods
        .def("has_timestamp", &chdr_rfnoc::ctrl_payload::has_timestamp)
        .def_readwrite("dst_port", &chdr_rfnoc::ctrl_payload::dst_port)
        .def_readwrite("src_port", &chdr_rfnoc::ctrl_payload::src_port)
        .def_readwrite("seq_num", &chdr_rfnoc::ctrl_payload::seq_num)
        .def_readwrite("timestamp", &chdr_rfnoc::ctrl_payload::timestamp)
        .def_readwrite("is_ack", &chdr_rfnoc::ctrl_payload::is_ack)
        .def_readwrite("src_epid", &chdr_rfnoc::ctrl_payload::src_epid)
        .def_readwrite("address", &chdr_rfnoc::ctrl_payload::address)
        .def_readwrite("byte_enable", &chdr_rfnoc::ctrl_payload::byte_enable)
        .def_readwrite("op_code", &chdr_rfnoc::ctrl_payload::op_code)
        .def_readwrite("status", &chdr_rfnoc::ctrl_payload::status)
        .def("get_data",
            [](chdr_rfnoc::ctrl_payload& self) -> std::vector<uint32_t> {
                return self.data_vtr;
            })
        .def("set_data",
            [](chdr_rfnoc::ctrl_payload& self, std::vector<uint32_t> data) {
                self.data_vtr = data;
            })
        .def("__str__", &chdr_rfnoc::ctrl_payload::to_string)
        .def("__repr__", &chdr_rfnoc::ctrl_payload::to_string);

    py::enum_<chdr_rfnoc::ctrl_status_t>(m, "CtrlStatus")
        .value("OKAY", chdr_rfnoc::CMD_OKAY)
        .value("CMDERR", chdr_rfnoc::CMD_CMDERR)
        .value("TSERR", chdr_rfnoc::CMD_TSERR)
        .value("WARNING", chdr_rfnoc::CMD_WARNING);

    py::enum_<chdr_rfnoc::ctrl_opcode_t>(m, "CtrlOpCode")
        .value("SLEEP", chdr_rfnoc::OP_SLEEP)
        .value("WRITE", chdr_rfnoc::OP_WRITE)
        .value("READ", chdr_rfnoc::OP_READ)
        .value("READ_WRITE", chdr_rfnoc::OP_READ_WRITE)
        .value("BLOCK_WRITE", chdr_rfnoc::OP_BLOCK_WRITE)
        .value("BLOCK_READ", chdr_rfnoc::OP_BLOCK_READ)
        .value("POLL", chdr_rfnoc::OP_POLL)
        .value("USER1", chdr_rfnoc::OP_USER1)
        .value("USER2", chdr_rfnoc::OP_USER2)
        .value("USER3", chdr_rfnoc::OP_USER3)
        .value("USER4", chdr_rfnoc::OP_USER4)
        .value("USER5", chdr_rfnoc::OP_USER5)
        .value("USER6", chdr_rfnoc::OP_USER6);

    py::class_<chdr_rfnoc::mgmt_payload>(m, "MgmtPayload")
        // Constructor
        .def(py::init<>())

        // Methods
        .def("set_header",
            &chdr_rfnoc::mgmt_payload::set_header,
            py::arg("src_epid"),
            py::arg("proto_ver"),
            py::arg("chdr_w"))
        .def("add_hop", &chdr_rfnoc::mgmt_payload::add_hop)
        .def("get_num_hops", &chdr_rfnoc::mgmt_payload::get_num_hops)
        .def("get_hop",
            &chdr_rfnoc::mgmt_payload::get_hop,
            py::return_value_policy::reference_internal)
        .def("pop_hop", &chdr_rfnoc::mgmt_payload::pop_hop)
        .def_property("src_epid",
            &chdr_rfnoc::mgmt_payload::get_src_epid,
            &chdr_rfnoc::mgmt_payload::set_src_epid)
        .def_property("chdr_w",
            &chdr_rfnoc::mgmt_payload::get_chdr_w,
            &chdr_rfnoc::mgmt_payload::set_chdr_w)
        .def_property("proto_ver",
            &chdr_rfnoc::mgmt_payload::get_proto_ver,
            &chdr_rfnoc::mgmt_payload::set_proto_ver)

        .def("__str__", &chdr_rfnoc::mgmt_payload::to_string)
        .def("__repr__", &chdr_rfnoc::mgmt_payload::to_string)
        .def("hops_to_string", &chdr_rfnoc::mgmt_payload::hops_to_string);

    py::class_<chdr_rfnoc::mgmt_hop_t>(m, "MgmtHop")
        // Constructor
        .def(py::init<>())

        // Methods
        .def("add_op", &chdr_rfnoc::mgmt_hop_t::add_op)
        .def("get_num_ops", &chdr_rfnoc::mgmt_hop_t::get_num_ops)
        .def("get_op",
            &chdr_rfnoc::mgmt_hop_t::get_op,
            py::return_value_policy::reference_internal)
        .def("__str__", &chdr_rfnoc::mgmt_hop_t::to_string)
        .def("__repr__", &chdr_rfnoc::mgmt_hop_t::to_string);

    py::class_<chdr_rfnoc::mgmt_op_t>(m, "MgmtOp")
        // Constructor
        .def(py::init<chdr_rfnoc::mgmt_op_t::op_code_t, uint64_t>(),
            py::arg("op_code"),
            py::arg("op_payload") = 0)
        .def(py::init<chdr_rfnoc::mgmt_op_t::op_code_t,
                 chdr_rfnoc::mgmt_op_t::sel_dest_payload>(),
            py::arg("op_code"),
            py::arg("op_payload"))
        .def(py::init<chdr_rfnoc::mgmt_op_t::op_code_t,
                 chdr_rfnoc::mgmt_op_t::cfg_payload>(),
            py::arg("op_code"),
            py::arg("op_payload"))
        .def(py::init<chdr_rfnoc::mgmt_op_t::op_code_t,
                 chdr_rfnoc::mgmt_op_t::node_info_payload>(),
            py::arg("op_code"),
            py::arg("op_payload"))

        // Methods
        .def_property_readonly("op_code", &chdr_rfnoc::mgmt_op_t::get_op_code)
        .def("get_op_payload", &chdr_rfnoc::mgmt_op_t::get_op_payload)
        .def("__str__", &chdr_rfnoc::mgmt_op_t::to_string)
        .def("__repr__", &chdr_rfnoc::mgmt_op_t::to_string);

    py::enum_<chdr_rfnoc::mgmt_op_t::op_code_t>(m, "MgmtOpCode")
        .value("NOP", chdr_rfnoc::mgmt_op_t::MGMT_OP_NOP)
        .value("ADVERTISE", chdr_rfnoc::mgmt_op_t::MGMT_OP_ADVERTISE)
        .value("SEL_DEST", chdr_rfnoc::mgmt_op_t::MGMT_OP_SEL_DEST)
        .value("RETURN", chdr_rfnoc::mgmt_op_t::MGMT_OP_RETURN)
        .value("INFO_REQ", chdr_rfnoc::mgmt_op_t::MGMT_OP_INFO_REQ)
        .value("INFO_RESP", chdr_rfnoc::mgmt_op_t::MGMT_OP_INFO_RESP)
        .value("CFG_WR_REQ", chdr_rfnoc::mgmt_op_t::MGMT_OP_CFG_WR_REQ)
        .value("CFG_RD_REQ", chdr_rfnoc::mgmt_op_t::MGMT_OP_CFG_RD_REQ)
        .value("CFG_RD_RESP", chdr_rfnoc::mgmt_op_t::MGMT_OP_CFG_RD_RESP);

    py::class_<chdr_rfnoc::mgmt_op_t::sel_dest_payload>(m, "MgmtOpSelDest")
        .def(py::init<uint16_t>())
        .def_readonly("dest", &chdr_rfnoc::mgmt_op_t::sel_dest_payload::dest)
        .def_static(
            "parse", [](uint64_t value) -> chdr_rfnoc::mgmt_op_t::sel_dest_payload {
                return value;
            });

    py::class_<chdr_rfnoc::mgmt_op_t::cfg_payload>(m, "MgmtOpCfg")
        .def(py::init<uint16_t, uint32_t>(), py::arg("addr"), py::arg("data"))
        .def_readonly("addr", &chdr_rfnoc::mgmt_op_t::cfg_payload::addr)
        .def_readonly("data", &chdr_rfnoc::mgmt_op_t::cfg_payload::data)
        .def_static("parse",
            [](uint64_t value) -> chdr_rfnoc::mgmt_op_t::cfg_payload { return value; });

    py::class_<chdr_rfnoc::mgmt_op_t::node_info_payload>(m, "MgmtOpNodeInfo")
        .def(py::init<uint16_t, uint8_t, uint16_t, uint32_t>(),
            py::arg("device_id"),
            py::arg("node_type"),
            py::arg("node_inst"),
            py::arg("ext_info"))
        .def_readonly("device_id", &chdr_rfnoc::mgmt_op_t::node_info_payload::device_id)
        .def_readonly("node_type", &chdr_rfnoc::mgmt_op_t::node_info_payload::node_type)
        .def_readonly("node_inst", &chdr_rfnoc::mgmt_op_t::node_info_payload::node_inst)
        .def_readonly("ext_info", &chdr_rfnoc::mgmt_op_t::node_info_payload::ext_info)
        .def_static(
            "parse", [](uint64_t value) -> chdr_rfnoc::mgmt_op_t::node_info_payload {
                return value;
            });

    py::class_<chdr_rfnoc::strs_payload>(m, "StrsPayload")
        // Constructor
        .def(py::init<>())

        // Methods
        .def_readwrite("src_epid", &chdr_rfnoc::strs_payload::src_epid)
        .def_readwrite("status", &chdr_rfnoc::strs_payload::status)
        .def_readwrite("capacity_bytes", &chdr_rfnoc::strs_payload::capacity_bytes)
        .def_readwrite("capacity_pkts", &chdr_rfnoc::strs_payload::capacity_pkts)
        .def_readwrite("xfer_count_bytes", &chdr_rfnoc::strs_payload::xfer_count_bytes)
        .def_readwrite("xfer_count_pkts", &chdr_rfnoc::strs_payload::xfer_count_pkts)
        .def_readwrite("buff_info", &chdr_rfnoc::strs_payload::buff_info)
        .def_readwrite("status_info", &chdr_rfnoc::strs_payload::status_info)

        .def("__str__", &chdr_rfnoc::strs_payload::to_string)
        .def("__repr__", &chdr_rfnoc::strs_payload::to_string);

    py::enum_<chdr_rfnoc::strs_status_t>(m, "StrsStatus")
        .value("OKAY", chdr_rfnoc::STRS_OKAY)
        .value("CMDERR", chdr_rfnoc::STRS_CMDERR)
        .value("SEQERR", chdr_rfnoc::STRS_SEQERR)
        .value("DATAERR", chdr_rfnoc::STRS_DATAERR)
        .value("RTERR", chdr_rfnoc::STRS_RTERR);

    py::class_<chdr_rfnoc::strc_payload>(m, "StrcPayload")
        // Constructor
        .def(py::init<>())

        // Methods
        .def_readwrite("src_epid", &chdr_rfnoc::strc_payload::src_epid)
        .def_readwrite("op_code", &chdr_rfnoc::strc_payload::op_code)
        .def_readwrite("op_data", &chdr_rfnoc::strc_payload::op_data)
        .def_readwrite("num_pkts", &chdr_rfnoc::strc_payload::num_pkts)
        .def_readwrite("num_bytes", &chdr_rfnoc::strc_payload::num_bytes)

        .def("__str__", &chdr_rfnoc::strc_payload::to_string)
        .def("__repr__", &chdr_rfnoc::strc_payload::to_string);

    py::enum_<chdr_rfnoc::strc_op_code_t>(m, "StrcOpCode")
        .value("INIT", chdr_rfnoc::STRC_INIT)
        .value("PING", chdr_rfnoc::STRC_PING)
        .value("RESYNC", chdr_rfnoc::STRC_RESYNC);
}
