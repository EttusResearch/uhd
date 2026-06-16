#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""C++ Client Interface Generator for gRPC Services.

This script parses a protobuf file and generates separate C++ header and
implementation files with automatic detection of daughterboard-related methods.
Generates clean interfaces with proper gRPC client implementation and automatic
token management.
"""

import re
import sys
from pathlib import Path

from mako.template import Template


class ProtoParser:
    """Parses a protobuf file to extract services, messages, and field types."""

    def __init__(self, proto_file):
        """Initialize the parser with the path to a .proto file."""
        self.proto_file = proto_file
        self.services = []
        self.messages = {}
        self.package_name = None
        self._parse_proto()

    def _parse_proto(self):
        """Parse the protobuf file to extract services and messages."""
        with open(self.proto_file, "r") as f:
            content = f.read()

        # Parse package name
        package_match = re.search(r"^\s*package\s+(\w+)\s*;", content, re.MULTILINE)
        if package_match:
            self.package_name = package_match.group(1)

        # Parse services
        service_pattern = r"service\s+(\w+)\s*\{([^}]+)\}"
        for service_match in re.finditer(service_pattern, content):
            service_name = service_match.group(1)
            service_body = service_match.group(2)

            methods = []
            # Updated pattern to capture timeout annotations in comments
            rpc_pattern = r"rpc\s+(\w+)\s*\(\s*(\w+)\s*\)\s*returns\s*\(\s*(\w+)\s*\);?\s*(?://\s*timeout=([\w_]+))?"
            for rpc_match in re.finditer(rpc_pattern, service_body):
                method_name = rpc_match.group(1)
                request_type = rpc_match.group(2)
                response_type = rpc_match.group(3)
                timeout_value = rpc_match.group(4)  # Can be None, string constant, or number

                methods.append(
                    {
                        "name": method_name,
                        "request_type": request_type,
                        "response_type": response_type,
                        "cpp_name": self._camel_to_snake(method_name),
                        "is_dboard_method": self._is_dboard_method(request_type, content),
                        "timeout": timeout_value,
                    }
                )

            self.services.append({"name": service_name, "methods": methods})

        # Parse messages for type information
        self._parse_messages(content)

    def _parse_messages(self, content):
        """Parse message definitions to understand field types."""
        message_pattern = r"message\s+(\w+)\s*\{([^}]+)\}"
        for msg_match in re.finditer(message_pattern, content):
            msg_name = msg_match.group(1)
            msg_body = msg_match.group(2)

            fields = []

            for line in msg_body.split("\n"):
                line = line.strip()
                if not line or line.startswith("//"):
                    continue

                # Check for repeated fields
                repeated_match = re.search(r"repeated\s+(\w+)\s+(\w+)\s*=\s*\d+;", line)
                if repeated_match:
                    field_type = repeated_match.group(1)
                    field_name = repeated_match.group(2)
                    cpp_type = f"std::vector<{self._proto_to_cpp_type(field_type)}>"
                    if field_type == "StringMap":
                        cpp_type = "std::vector<std::map<std::string, std::string>>"
                    fields.append(
                        {
                            "type": f"repeated {field_type}",
                            "name": field_name,
                            "cpp_type": cpp_type,
                        }
                    )
                    continue

                # Check for map fields
                map_match = re.search(r"map<(\w+),\s*(\w+)>\s+(\w+)\s*=\s*\d+;", line)
                if map_match:
                    key_type = map_match.group(1)
                    value_type = map_match.group(2)
                    field_name = map_match.group(3)
                    fields.append(
                        {
                            "type": f"map<{key_type}, {value_type}>",
                            "name": field_name,
                            "cpp_type": "std::map<std::string, std::string>",
                        }
                    )
                    continue

                # Check for regular fields
                field_match = re.search(r"(\w+)\s+(\w+)\s*=\s*\d+;", line)
                if field_match:
                    field_type = field_match.group(1)
                    field_name = field_match.group(2)
                    fields.append(
                        {
                            "type": field_type,
                            "name": field_name,
                            "cpp_type": self._proto_to_cpp_type(field_type),
                        }
                    )

            self.messages[msg_name] = {"name": msg_name, "fields": fields}

    def _camel_to_snake(self, name):
        """Convert CamelCase to snake_case."""
        result = re.sub("(.)([A-Z][a-z]+)", r"\1_\2", name)
        return re.sub("([a-z0-9])([A-Z])", r"\1_\2", result).lower()

    def _is_dboard_method(self, request_type, content):
        """Determine if a method is daughterboard-related by checking for db_idx parameter."""
        # Find the request message definition
        msg_pattern = f"message\\s+{request_type}\\s*\\{{([^}}]+)\\}}"
        msg_match = re.search(msg_pattern, content)
        if msg_match:
            msg_body = msg_match.group(1)
            # Check if db_idx field exists
            return "db_idx" in msg_body
        return False

    def _proto_to_cpp_type(self, proto_type):
        """Convert protobuf types to C++ types."""
        type_mapping = {
            "double": "double",
            "float": "float",
            "int32": "int32_t",
            "int64": "int64_t",
            "uint32": "uint32_t",
            "uint64": "uint64_t",
            "bool": "bool",
            "string": "std::string",
            "bytes": "std::vector<uint8_t>",
            # StringMap, BytesMap, and SensorValueMap are not standard types, they are
            # specific to the MPM proto file so we can more easily handle lists of maps.
            "StringMap": "std::map<std::string, std::string>",
            "BytesMap": "std::map<std::string, std::vector<uint8_t>>",
            "SensorValueMap": "std::map<std::string, std::string>",
            # MethodInfo should be converted to avoid leaking protobuf types
            "MethodInfo": "std::map<std::string, std::string>",
        }
        cpp_type = type_mapping.get(proto_type, None)
        if cpp_type:
            return cpp_type
        # For unknown types (likely protobuf messages), qualify with package namespace
        if self.package_name and proto_type[0].isupper():  # Check if it looks like a message type
            return f"{self.package_name}::{proto_type}"
        return proto_type

    def _determine_return_type(self, response_type):
        """Determine the C++ return type for a response message based on its structure."""
        if response_type not in self.messages:
            return "void"

        response_msg = self.messages[response_type]
        fields = response_msg["fields"]

        if not fields:
            return "void"
        elif len(fields) == 1:
            # Special case: get_proto_ver returns uint16_t (protobuf uses uint32)
            if response_type == "GetProtoVerResponse":
                return "uint16_t"
            # Single field response - return the field's C++ type directly
            field = fields[0]
            return field["cpp_type"]
        elif len(fields) == 2:
            # Two fields - return as std::pair<type1, type2> using proto field types.
            # Any narrowing (e.g. int32 -> int8_t) is the caller's responsibility.
            field1_type = fields[0]["cpp_type"]
            field2_type = fields[1]["cpp_type"]
            return f"std::pair<{field1_type}, {field2_type}>"
        else:
            # Multiple fields - could return a struct, but for now return the first non-token field
            # This handles cases where there might be additional metadata fields
            for field in fields:
                if field["name"] not in [
                    "token",
                    "status",
                    "error",
                ]:  # Skip common metadata fields
                    return field["cpp_type"]

            # If all fields are metadata, return void
            return "void"

    def _get_method_parameters(self, method):
        """Get the C++ parameters for a method based on its request type."""
        request_type = method["request_type"]
        if request_type not in self.messages:
            return []

        request_msg = self.messages[request_type]
        params = []

        # Skip token and db_idx fields as they're handled automatically
        special_params = ["token", "db_idx"]

        for field in request_msg["fields"]:
            if field["name"] in special_params:
                continue

            field_type = field["cpp_type"]
            field_name = field["name"]

            # Determine if parameter should be const reference
            if "std::" in field_type and field_type != "std::string":
                params.append(f"const {field_type}& {field_name}")
            else:
                params.append(f"{field_type} {field_name}")

        return params

    def _method_requires_token(self, method):
        """Check if a method requires a token by examining its request message."""
        request_type = method["request_type"]
        if request_type not in self.messages:
            return False

        request_msg = self.messages[request_type]
        for field in request_msg["fields"]:
            if field["name"] == "token":
                return True
        return False

    def _get_pair_field_names(self, response_type):
        """Return [field0_name, field1_name] for two-field (pair) response types, else None."""
        if response_type not in self.messages:
            return None
        fields = self.messages[response_type]["fields"]
        if len(fields) == 2:
            return [fields[0]["name"], fields[1]["name"]]
        return None

    def _get_response_field_info(self, response_type):
        """Get information about the main response field."""
        if response_type not in self.messages:
            return None

        response_msg = self.messages[response_type]
        fields = response_msg["fields"]

        if not fields:
            return None
        elif len(fields) == 1:
            return fields[0]
        else:
            # Find the first non-metadata field
            for field in fields:
                if field["name"] not in ["token", "status", "error"]:
                    return field
            return None


def generate_cpp_interface(
    proto_file,
    header_file,
    impl_file,
    header_template_file,
    impl_template_file,
    pybind_file,
    pybind_template_file,
):
    """Generate C++ interface and implementation from protobuf definition."""
    parser = ProtoParser(proto_file)

    # Load templates
    with open(header_template_file, "r") as f:
        header_template_content = f.read()

    with open(impl_template_file, "r") as f:
        impl_template_content = f.read()

    header_template = Template(header_template_content)
    impl_template = Template(impl_template_content)

    # Separate methods into main and dboard categories
    main_methods = []
    dboard_methods = []

    for service in parser.services:
        for method in service["methods"]:
            response_field = parser._get_response_field_info(method["response_type"])

            method_info = {
                "name": method["cpp_name"],
                "camel_name": method["name"],
                "return_type": parser._determine_return_type(method["response_type"]),
                "parameters": parser._get_method_parameters(method),
                "request_type": method["request_type"],
                "response_type": method["response_type"],
                "requires_token": parser._method_requires_token(method),
                "response_field": response_field,
                "pair_field_names": parser._get_pair_field_names(method["response_type"]),
                "timeout": method.get("timeout"),  # Add timeout information
            }

            if method["is_dboard_method"]:
                dboard_methods.append(method_info)
            else:
                main_methods.append(method_info)

    # Get package name from proto
    package_name = parser.package_name if parser.package_name else "rpc"
    for service in parser.services:
        service_name = service["name"]
        break

    # Extract just the filename from the header path for the include
    header_filename = Path(header_file).name

    template_data = {
        "main_methods": main_methods,
        "dboard_methods": dboard_methods,
        "services": parser.services,
        "package_name": package_name,
        "service_name": service_name,
        "header_filename": header_filename,
    }

    # Generate header
    header_code = header_template.render(**template_data)
    with open(header_file, "w") as f:
        f.write(header_code)

    # Generate implementation
    impl_code = impl_template.render(**template_data)
    with open(impl_file, "w") as f:
        f.write(impl_code)

    print(f"Generated C++ header: {header_file}")
    print(f"Generated C++ implementation: {impl_file}")
    print(f"Main methods: {[m['name'] for m in main_methods]}")
    print(f"Daughterboard methods: {[m['name'] for m in dboard_methods]}")

    # Generate pybind11 bindings
    with open(pybind_template_file, "r") as f:
        pybind_template_content = f.read()
    pybind_template = Template(pybind_template_content)
    pybind_code = pybind_template.render(**template_data)
    with open(pybind_file, "w") as f:
        f.write(pybind_code)
    print(f"Generated pybind11 bindings: {pybind_file}")


if __name__ == "__main__":
    if len(sys.argv) != 8:
        print(
            "Usage: python mpm_client_generate_cpp.py <proto_file> <header_file> <impl_file>"
            " <header_template> <impl_template> <pybind_file> <pybind_template>"
        )
        sys.exit(1)

    proto_file = sys.argv[1]
    header_file = sys.argv[2]
    impl_file = sys.argv[3]
    header_template_file = sys.argv[4]
    impl_template_file = sys.argv[5]
    pybind_file = sys.argv[6]
    pybind_template_file = sys.argv[7]

    generate_cpp_interface(
        proto_file,
        header_file,
        impl_file,
        header_template_file,
        impl_template_file,
        pybind_file,
        pybind_template_file,
    )
