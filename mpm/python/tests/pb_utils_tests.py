#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Unit tests for usrp_mpm.pb_utils.

pb_utils provides the protobuf<->Python glue used by rpc_server.py. The pure
Python helpers (snake_to_camelcase, _sanitize_map_nones) run anywhere. The
remaining helpers inspect real protobuf descriptors/containers, so those tests
require the generated ``mpm_server_pb2`` module and are skipped when it is not
importable (i.e. run them on the MPM device where gRPC/Protobuf are installed).

Running on the MPM device
-------------------------
The generated ``mpm_server_pb2`` and the ``usrp_mpm`` package are installed on
the device, so these tests run there without any extra setup. Copy this file
into the on-device tests directory (or use one already deployed) and run:

    # From the MPM Python tests directory on the device
    # (typically /usr/lib/usrp_mpm/tests or the build's
    #  python/tests directory):

    # 1. Run only this test module, verbose:
    python3 -m unittest pb_utils_tests -v

    # 2. Run a single test case or method:
    python3 -m unittest pb_utils_tests.TestSnakeToCamelcase -v
    python3 -m unittest pb_utils_tests.TestBuildResponseFromReturnValue.test_no_fields

    # 3. Run as part of the full MPM unit-test suite (also picks these up
    #    because they are registered in run_unit_tests.py):
    python3 run_unit_tests.py x4xx

If ``mpm_server_pb2`` cannot be imported (e.g. on a developer host without a
built/installed MPM), the protobuf-dependent test classes are skipped and only
the pure-Python tests run.
"""

import unittest

from base_tests import TestBase
from usrp_mpm import pb_utils

# The protobuf stubs are generated at build time from mpm_server.proto and are
# only present on a built MPM device. Guard the import so the pure-Python tests
# still run (and the rest skip gracefully) on a developer machine.
try:
    from usrp_mpm import mpm_server_pb2 as pb2

    HAVE_PB2 = True
except ImportError:
    pb2 = None
    HAVE_PB2 = False

_PB2_REASON = "requires generated mpm_server_pb2 (run on an MPM device)"


class FakeLog:
    """
    Minimal logger stub that records the messages passed to each level so
    tests can assert on warning/trace side effects without a real logger.
    """

    def __init__(self):
        self.warnings = []
        self.traces = []
        self.debugs = []
        self.errors = []
        self.infos = []

    def warning(self, msg, *args, **kwargs):
        self.warnings.append(msg)

    def trace(self, msg, *args, **kwargs):
        self.traces.append(msg)

    def debug(self, msg, *args, **kwargs):
        self.debugs.append(msg)

    def error(self, msg, *args, **kwargs):
        self.errors.append(msg)

    def info(self, msg, *args, **kwargs):
        self.infos.append(msg)


class FakeFieldDescriptor:
    """
    Minimal field descriptor for scalar types not present in mpm_server.proto.
    """

    TYPE_DOUBLE = 1
    TYPE_FLOAT = 2
    TYPE_INT64 = 3
    TYPE_UINT64 = 4
    TYPE_INT32 = 5
    TYPE_BOOL = 8
    TYPE_STRING = 9
    TYPE_MESSAGE = 11
    TYPE_BYTES = 12
    TYPE_UINT32 = 13
    TYPE_ENUM = 14

    def __init__(self, field_type):
        self.type = field_type



class TestSnakeToCamelcase(TestBase):
    """
    Tests for pb_utils.snake_to_camelcase (pure Python, no protobuf needed).
    """

    def test_basic_conversion(self):
        """A simple snake_case identifier becomes CamelCase."""
        self.assertEqual(pb_utils.snake_to_camelcase("get_device_info"), "GetDeviceInfo")

    def test_single_word(self):
        """A single lowercase word is capitalized."""
        self.assertEqual(pb_utils.snake_to_camelcase("ping"), "Ping")

    def test_preserves_existing_uppercase(self):
        """Existing uppercase letters within a segment are preserved."""
        self.assertEqual(
            pb_utils.snake_to_camelcase("get_rf_channelA_temp_sensor"),
            "GetRfChannelATempSensor",
        )

    def test_already_capitalized_first_segment(self):
        """A segment that is already capitalized stays capitalized."""
        self.assertEqual(pb_utils.snake_to_camelcase("Get_info"), "GetInfo")

    def test_single_char_segments(self):
        """Single-character segments are each upper-cased and concatenated."""
        self.assertEqual(pb_utils.snake_to_camelcase("a_b_c"), "ABC")

    def test_double_underscore_yields_underscore(self):
        """An empty segment from a double underscore becomes a literal '_'."""
        self.assertEqual(pb_utils.snake_to_camelcase("foo__bar"), "Foo_Bar")

    def test_leading_underscore_yields_underscore(self):
        """A leading underscore produces a leading '_' in the output."""
        self.assertEqual(pb_utils.snake_to_camelcase("_private"), "_Private")

    def test_trailing_underscore_yields_underscore(self):
        """A trailing underscore produces a trailing '_' in the output."""
        self.assertEqual(pb_utils.snake_to_camelcase("foo_"), "Foo_")

    def test_empty_string(self):
        """An empty input splits into one empty segment and yields '_'."""
        self.assertEqual(pb_utils.snake_to_camelcase(""), "_")


class TestSanitizeMapNones(TestBase):
    """
    Tests for pb_utils._sanitize_map_nones (pure Python, no protobuf needed).
    """

    def test_replaces_none_with_na(self):
        """None values are replaced with the string 'n/a'."""
        log = FakeLog()
        result = pb_utils._sanitize_map_nones({"a": "1", "b": None}, "field", log)
        self.assertEqual(result, {"a": "1", "b": "n/a"})

    def test_logs_warning_per_none(self):
        """A warning is logged for every None value that is coerced."""
        log = FakeLog()
        pb_utils._sanitize_map_nones({"a": None, "b": None, "c": "ok"}, "field", log)
        self.assertEqual(len(log.warnings), 2)

    def test_no_none_no_warning(self):
        """When there are no None values, nothing is logged and dict is copied."""
        log = FakeLog()
        src = {"a": "1", "b": "2"}
        result = pb_utils._sanitize_map_nones(src, "field", log)
        self.assertEqual(result, src)
        self.assertEqual(len(log.warnings), 0)

    def test_returns_new_dict(self):
        """The original dictionary is not mutated."""
        log = FakeLog()
        src = {"a": None}
        result = pb_utils._sanitize_map_nones(src, "field", log)
        self.assertIsNone(src["a"])
        self.assertEqual(result["a"], "n/a")

    def test_empty_dict(self):
        """An empty dict returns an empty dict with no warnings."""
        log = FakeLog()
        self.assertEqual(pb_utils._sanitize_map_nones({}, "field", log), {})
        self.assertEqual(len(log.warnings), 0)


@unittest.skipUnless(HAVE_PB2, _PB2_REASON)
class TestProtobufToPython(TestBase):
    """
    Tests for pb_utils.protobuf_to_python (requires generated protobuf types).
    """

    def test_primitive_passthrough(self):
        """Primitive scalars are returned unchanged."""
        self.assertEqual(pb_utils.protobuf_to_python(5), 5)
        self.assertEqual(pb_utils.protobuf_to_python("x"), "x")
        self.assertEqual(pb_utils.protobuf_to_python(b"x"), b"x")
        self.assertEqual(pb_utils.protobuf_to_python(True), True)

    def test_string_not_treated_as_iterable(self):
        """Strings are returned as-is, not split into a list of chars."""
        self.assertEqual(pb_utils.protobuf_to_python("hello"), "hello")

    def test_tuple_iterable_converts_to_list(self):
        """A non-string iterable is converted item-by-item into a list."""
        self.assertEqual(pb_utils.protobuf_to_python((1, "two", True)), [1, "two", True])

    def test_message_with_single_data_map(self):
        """A message with a single 'data' map field converts to a dict."""
        sm = pb2.StringMap()
        sm.data["a"] = "1"
        sm.data["b"] = "2"
        self.assertEqual(pb_utils.protobuf_to_python(sm), {"a": "1", "b": "2"})

    def test_map_container_directly(self):
        """A map container converts to a plain dict."""
        sm = pb2.StringMap()
        sm.data["k"] = "v"
        self.assertEqual(pb_utils.protobuf_to_python(sm.data), {"k": "v"})

    def test_repeated_scalar_container(self):
        """A repeated scalar container converts to a list."""
        resp = pb2.GetMpmCompatNumberResponse()
        resp.compat_num.extend([8, 3])
        self.assertEqual(pb_utils.protobuf_to_python(resp.compat_num), [8, 3])

    def test_repeated_composite_container(self):
        """A repeated message container converts to a list of dicts."""
        resp = pb2.ListMethodsResponse()
        method = resp.methods.add()
        method.method_name = "ping"
        method.requires_claim = False
        self.assertEqual(
            pb_utils.protobuf_to_python(resp.methods),
            [{"method_name": "ping", "docstring": "", "requires_claim": False}],
        )

    def test_multi_field_message(self):
        """A message with several fields converts to a dict keyed by field."""
        method = pb2.MethodInfo(method_name="ping", docstring="doc", requires_claim=True)
        self.assertEqual(
            pb_utils.protobuf_to_python(method),
            {"method_name": "ping", "docstring": "doc", "requires_claim": True},
        )

    def test_conversion_failure_raises_value_error(self):
        """A container whose items() raises is reported as a ValueError."""

        class Bad:
            """Looks like a map but explodes when iterated."""

            def keys(self):
                return []

            def items(self):
                raise RuntimeError("boom")

        with self.assertRaises(ValueError):
            pb_utils.protobuf_to_python(Bad())


@unittest.skipUnless(HAVE_PB2, _PB2_REASON)
class TestPythonToProtobuf(TestBase):
    """
    Tests for pb_utils.python_to_protobuf (requires generated protobuf types).
    """

    def _field(self, message_type, field_name):
        return message_type.DESCRIPTOR.fields_by_name[field_name]

    def test_double_field(self):
        """A DOUBLE field coerces its value to float."""
        fd = self._field(pb2.GetMasterClockRateResponse, "clock_rate")
        result = pb_utils.python_to_protobuf(122, fd)
        self.assertIsInstance(result, float)
        self.assertEqual(result, 122.0)

    def test_float_field(self):
        """A FLOAT field coerces its value to float."""
        fd = FakeFieldDescriptor(self._field(pb2.GetMasterClockRateResponse, "clock_rate").TYPE_FLOAT)
        result = pb_utils.python_to_protobuf("1.25", fd)
        self.assertIsInstance(result, float)
        self.assertEqual(result, 1.25)

    def test_int32_field(self):
        """An INT32 field coerces its value to int."""
        fd = self._field(pb2.GetFirResponse, "gain")
        self.assertEqual(pb_utils.python_to_protobuf("-7", fd), -7)

    def test_int64_field(self):
        """An INT64 field coerces its value to int."""
        fd = FakeFieldDescriptor(self._field(pb2.GetDeviceIdResponse, "device_id").TYPE_INT64)
        self.assertEqual(pb_utils.python_to_protobuf("-9223372036854775808", fd), -9223372036854775808)

    def test_uint_field(self):
        """A UINT field coerces its value to int."""
        fd = self._field(pb2.GetDeviceIdResponse, "device_id")
        self.assertEqual(pb_utils.python_to_protobuf(7.0, fd), 7)

    def test_uint64_field(self):
        """A UINT64 field coerces its value to int."""
        fd = self._field(pb2.GetTimekeeperTimeResponse, "time")
        self.assertEqual(pb_utils.python_to_protobuf("18446744073709551615", fd), 18446744073709551615)

    def test_string_field(self):
        """A STRING field coerces its value with str()."""
        fd = self._field(pb2.PingResponse, "data")
        self.assertEqual(pb_utils.python_to_protobuf(123, fd), "123")

    def test_string_field_from_dict_device_addr_format(self):
        """A dict assigned to a STRING field is encoded as 'key=val,key=val'."""
        fd = self._field(pb2.PingResponse, "data")
        self.assertEqual(
            pb_utils.python_to_protobuf({"a": 1, "b": 2}, fd),
            "a=1,b=2",
        )

    def test_bool_field(self):
        """A BOOL field coerces its value with bool()."""
        fd = self._field(pb2.ReclaimResponse, "success")
        self.assertEqual(pb_utils.python_to_protobuf(1, fd), True)

    def test_bytes_field_from_str(self):
        """A BYTES field encodes strings as utf-8."""
        fd = self._field(pb2.UpdateComponentRequest, "data_l")
        self.assertEqual(pb_utils.python_to_protobuf("hi", fd), b"hi")

    def test_bytes_field_passthrough(self):
        """A BYTES field leaves existing bytes unchanged."""
        fd = self._field(pb2.UpdateComponentRequest, "data_l")
        self.assertEqual(pb_utils.python_to_protobuf(b"raw", fd), b"raw")

    def test_message_map_field_converts_values(self):
        """A dict assigned to a map<string,string> field converts entries."""
        fd = self._field(pb2.StringMap, "data")
        self.assertEqual(
            pb_utils.python_to_protobuf({"k": "v"}, fd),
            {"k": "v"},
        )

    def test_message_map_field_bytes_values(self):
        """A dict assigned to a map<string,bytes> field encodes the values."""
        fd = self._field(pb2.BytesMap, "data")
        self.assertEqual(
            pb_utils.python_to_protobuf({"k": "v"}, fd),
            {"k": b"v"},
        )

    def test_unhandled_scalar_type_passthrough(self):
        """A scalar type without an explicit branch is returned unchanged."""
        fd = FakeFieldDescriptor(FakeFieldDescriptor.TYPE_ENUM)
        value = object()
        self.assertIs(pb_utils.python_to_protobuf(value, fd), value)


@unittest.skipUnless(HAVE_PB2, _PB2_REASON)
class TestPopulateResponseField(TestBase):
    """
    Tests for pb_utils.populate_response_field (requires generated protobuf types).
    """

    def _field(self, message_type, field_name):
        return message_type.DESCRIPTOR.fields_by_name[field_name]

    def test_repeated_primitive_list(self):
        """A list populates a repeated primitive field."""
        log = FakeLog()
        resp = pb2.GetMpmCompatNumberResponse()
        fd = self._field(pb2.GetMpmCompatNumberResponse, "compat_num")
        pb_utils.populate_response_field(resp, "compat_num", [8, 3], fd, log)
        self.assertEqual(list(resp.compat_num), [8, 3])

    def test_repeated_primitive_flattens_list_of_lists(self):
        """A list-of-lists is flattened one level into a repeated field."""
        log = FakeLog()
        resp = pb2.GetMpmCompatNumberResponse()
        fd = self._field(pb2.GetMpmCompatNumberResponse, "compat_num")
        pb_utils.populate_response_field(resp, "compat_num", [[8, 3], [1]], fd, log)
        self.assertEqual(list(resp.compat_num), [8, 3, 1])

    def test_repeated_string_from_dict_uses_keys(self):
        """A dict assigned to a repeated string field uses its keys."""
        log = FakeLog()
        resp = pb2.GetInitStatusResponse()
        fd = self._field(pb2.GetInitStatusResponse, "init_status")
        pb_utils.populate_response_field(resp, "init_status", {"x": 1, "y": 2}, fd, log)
        self.assertEqual(list(resp.init_status), ["x", "y"])

    def test_repeated_message_from_list_of_dicts(self):
        """A list of dicts populates a repeated message field."""
        log = FakeLog()
        resp = pb2.ListMethodsResponse()
        fd = self._field(pb2.ListMethodsResponse, "methods")
        pb_utils.populate_response_field(
            resp, "methods", [{"method_name": "ping", "requires_claim": True}], fd, log
        )
        self.assertEqual(len(resp.methods), 1)
        self.assertEqual(resp.methods[0].method_name, "ping")
        self.assertEqual(resp.methods[0].requires_claim, True)

    def test_repeated_message_from_single_dict(self):
        """A single dict populates one item in a repeated message field."""
        log = FakeLog()
        resp = pb2.ListMethodsResponse()
        fd = self._field(pb2.ListMethodsResponse, "methods")
        pb_utils.populate_response_field(
            resp, "methods", {"method_name": "ping", "requires_claim": False}, fd, log
        )
        self.assertEqual(len(resp.methods), 1)
        self.assertEqual(resp.methods[0].method_name, "ping")
        self.assertEqual(resp.methods[0].requires_claim, False)

    def test_map_field_sanitizes_none(self):
        """A map field replaces None values with 'n/a' and warns."""
        log = FakeLog()
        sm = pb2.StringMap()
        fd = self._field(pb2.StringMap, "data")
        pb_utils.populate_response_field(sm, "data", {"a": "1", "b": None}, fd, log)
        self.assertEqual(dict(sm.data), {"a": "1", "b": "n/a"})
        self.assertEqual(len(log.warnings), 1)

    def test_map_field_non_dict_logs_trace(self):
        """A non-dict value for a map field logs a trace warning and no-ops."""
        log = FakeLog()
        sm = pb2.StringMap()
        fd = self._field(pb2.StringMap, "data")
        pb_utils.populate_response_field(sm, "data", "not-a-dict", fd, log)
        self.assertEqual(dict(sm.data), {})
        self.assertEqual(len(log.traces), 1)

    def test_nested_message_field_from_dict(self):
        """A dict populates a nested message field (StringMap wrapper)."""
        log = FakeLog()
        resp = pb2.GetDeviceInfoResponse()
        fd = self._field(pb2.GetDeviceInfoResponse, "dev_info")
        pb_utils.populate_response_field(resp, "dev_info", {"a": "1"}, fd, log)
        self.assertEqual(dict(resp.dev_info.data), {"a": "1"})

    def test_primitive_field(self):
        """A scalar value populates a primitive field."""
        log = FakeLog()
        resp = pb2.GetDeviceIdResponse()
        fd = self._field(pb2.GetDeviceIdResponse, "device_id")
        pb_utils.populate_response_field(resp, "device_id", 7, fd, log)
        self.assertEqual(resp.device_id, 7)


@unittest.skipUnless(HAVE_PB2, _PB2_REASON)
class TestPopulateMessageFromDict(TestBase):
    """
    Tests for pb_utils.populate_message_from_dict (requires generated protobuf types).
    """

    def test_single_data_map_message(self):
        """A message with a single 'data' map field takes the whole dict."""
        log = FakeLog()
        sm = pb2.StringMap()
        pb_utils.populate_message_from_dict(sm, {"a": "1", "b": None}, log)
        self.assertEqual(dict(sm.data), {"a": "1", "b": "n/a"})
        self.assertEqual(len(log.warnings), 1)

    def test_bytes_map_message_encodes_values(self):
        """A map<string,bytes> 'data' message encodes string values to bytes."""
        log = FakeLog()
        bm = pb2.BytesMap()
        pb_utils.populate_message_from_dict(bm, {"k": "v"}, log)
        self.assertEqual(dict(bm.data), {"k": b"v"})

    def test_non_dict_input_logs_trace(self):
        """A non-dict input logs a trace warning and does not raise."""
        log = FakeLog()
        sm = pb2.StringMap()
        pb_utils.populate_message_from_dict(sm, "not-a-dict", log)
        self.assertEqual(dict(sm.data), {})
        self.assertEqual(len(log.traces), 1)

    def test_regular_multi_field_message(self):
        """Named fields map onto a regular multi-field message."""
        log = FakeLog()
        method = pb2.MethodInfo()
        pb_utils.populate_message_from_dict(
            method, {"method_name": "ping", "requires_claim": True}, log
        )
        self.assertEqual(method.method_name, "ping")
        self.assertEqual(method.requires_claim, True)

    def test_unknown_field_logs_trace(self):
        """An unknown field name is ignored with a trace warning."""
        log = FakeLog()
        method = pb2.MethodInfo()
        pb_utils.populate_message_from_dict(method, {"bogus": 1}, log)
        self.assertEqual(len(log.traces), 1)


@unittest.skipUnless(HAVE_PB2, _PB2_REASON)
class TestBuildResponseFromReturnValue(TestBase):
    """
    Tests for pb_utils.build_response_from_return_value (requires protobuf types).
    """

    def test_no_fields(self):
        """A response type with no fields yields an empty message."""
        log = FakeLog()
        resp = pb_utils.build_response_from_return_value(None, pb2.EmptyResponse, log)
        self.assertIsInstance(resp, pb2.EmptyResponse)

    def test_single_string_field(self):
        """A scalar return value populates a single string field."""
        log = FakeLog()
        resp = pb_utils.build_response_from_return_value("pong", pb2.PingResponse, log)
        self.assertEqual(resp.data, "pong")

    def test_single_uint_field(self):
        """A scalar return value populates a single uint field."""
        log = FakeLog()
        resp = pb_utils.build_response_from_return_value(7, pb2.GetDeviceIdResponse, log)
        self.assertEqual(resp.device_id, 7)

    def test_single_bool_field(self):
        """A scalar return value populates a single bool field."""
        log = FakeLog()
        resp = pb_utils.build_response_from_return_value(True, pb2.ReclaimResponse, log)
        self.assertEqual(resp.success, True)

    def test_single_double_field(self):
        """A scalar return value populates a single double field."""
        log = FakeLog()
        resp = pb_utils.build_response_from_return_value(
            122.88e6, pb2.GetMasterClockRateResponse, log
        )
        self.assertEqual(resp.clock_rate, 122.88e6)

    def test_single_repeated_field(self):
        """A list return value populates a single repeated field."""
        log = FakeLog()
        resp = pb_utils.build_response_from_return_value(
            [8, 3], pb2.GetMpmCompatNumberResponse, log
        )
        self.assertEqual(list(resp.compat_num), [8, 3])

    def test_single_message_field_from_dict(self):
        """A dict return value populates a single nested message field."""
        log = FakeLog()
        resp = pb_utils.build_response_from_return_value(
            {"a": "1"}, pb2.GetDeviceInfoResponse, log
        )
        self.assertEqual(dict(resp.dev_info.data), {"a": "1"})

    def test_multi_field_from_dict(self):
        """A dict return value maps onto a multi-field message by name."""
        log = FakeLog()
        resp = pb_utils.build_response_from_return_value(
            {"method_name": "ping", "docstring": "d", "requires_claim": True},
            pb2.MethodInfo,
            log,
        )
        self.assertEqual(resp.method_name, "ping")
        self.assertEqual(resp.docstring, "d")
        self.assertEqual(resp.requires_claim, True)

    def test_multi_field_from_partial_dict(self):
        """A partial dict leaves omitted protobuf fields at defaults."""
        log = FakeLog()
        resp = pb_utils.build_response_from_return_value(
            {"method_name": "ping"},
            pb2.MethodInfo,
            log,
        )
        self.assertEqual(resp.method_name, "ping")
        self.assertEqual(resp.docstring, "")
        self.assertEqual(resp.requires_claim, False)

    def test_multi_field_from_object_attributes(self):
        """An object's __dict__ maps onto a multi-field message by name."""
        log = FakeLog()

        class Ret:
            """Plain object exposing attributes matching field names."""

            def __init__(self):
                self.method_name = "ping"
                self.docstring = "d"
                self.requires_claim = False

        resp = pb_utils.build_response_from_return_value(Ret(), pb2.MethodInfo, log)
        self.assertEqual(resp.method_name, "ping")
        self.assertEqual(resp.docstring, "d")
        self.assertEqual(resp.requires_claim, False)

    def test_multi_field_from_positional_tuple(self):
        """A tuple return value maps onto a multi-field message by position."""
        log = FakeLog()
        resp = pb_utils.build_response_from_return_value(
            ("tok", 5), pb2.SetDeviceIdRequest, log
        )
        self.assertEqual(resp.token, "tok")
        self.assertEqual(resp.device_id, 5)

    def test_multi_field_unmappable_raises(self):
        """A value that cannot be mapped to multiple fields raises ValueError."""
        log = FakeLog()
        with self.assertRaises(ValueError):
            pb_utils.build_response_from_return_value(42, pb2.SetDeviceIdRequest, log)


if __name__ == "__main__":
    unittest.main()
