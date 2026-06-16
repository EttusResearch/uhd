#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Protobuf utilities.

This module provides utilities for handling protobuf types in Python. Note that
the functions offered here may not be generally applicable outside of MPM.
"""


def snake_to_camelcase(word):
    """Convert snake_case to CamelCase."""

    # preserve existing uppercase letters
    # e.g., get_rf_channelA_temp_sensor -> GetRfChannelATempSensor
    # This may cause problem, need to look out for.
    return "".join((x[0].upper() + x[1:]) if x else "_" for x in word.split("_"))


def protobuf_to_python(value):
    """Convert protobuf objects to Python native types.

    This will return a native Python type (string, map, list, ...) by inspecting
    the protobuf type.

    If this function cannot infer the Python type, it will throw a ValueError.

    Args:
        value: The protobuf value to convert

    Returns:
        The converted Python native value

    Raises:
        ValueError if the Python type cannot be inferred or converted to.
    """
    try:
        # Handle RepeatedCompositeContainer (repeated message fields)
        if hasattr(value, "__class__") and "RepeatedCompositeContainer" in str(type(value)):
            return [protobuf_to_python(item) for item in value]

        # Handle RepeatedScalarContainer (repeated primitive fields)
        elif hasattr(value, "__class__") and "RepeatedScalarContainer" in str(type(value)):
            return list(value)

        # Handle protobuf map fields (ScalarMapContainer)
        elif hasattr(value, "items") and hasattr(value, "keys"):
            return dict(value.items())

        # Handle protobuf messages
        elif hasattr(value, "DESCRIPTOR"):
            # This is a protobuf message object
            descriptor = value.DESCRIPTOR

            # Handle messages with a single 'data' map field (like DataMap)
            if len(descriptor.fields) == 1 and descriptor.fields[0].name == "data":
                data_field = getattr(value, "data")
                if hasattr(data_field, "items"):
                    # Convert map to regular Python dict
                    return dict(data_field.items())

            # Handle other message types by converting all fields
            result = {}
            for field in descriptor.fields:
                field_value = getattr(value, field.name)
                result[field.name] = protobuf_to_python(field_value)
            return result

        # Handle regular iterables (but not strings or bytes)
        elif hasattr(value, "__iter__") and not isinstance(value, (str, bytes)):
            return [protobuf_to_python(item) for item in value]

        # Return primitive types as-is
        else:
            return value
    except Exception as ex:
        raise ValueError(f"Failed to convert protobuf value to Python: {str(ex)}")


def python_to_protobuf(value, field_descriptor):
    """Convert Python values to protobuf-compatible values based on field type.

    Args:
        value: The Python value to convert
        field_descriptor: The protobuf field descriptor

    Returns:
        The converted value suitable for protobuf assignment
    """
    field_type = field_descriptor.type

    # Handle primitive types
    if field_type in [field_descriptor.TYPE_DOUBLE, field_descriptor.TYPE_FLOAT]:
        return float(value)
    elif field_type in [
        field_descriptor.TYPE_INT32,
        field_descriptor.TYPE_INT64,
        field_descriptor.TYPE_UINT32,
        field_descriptor.TYPE_UINT64,
    ]:
        return int(value)
    elif field_type == field_descriptor.TYPE_STRING:
        if isinstance(value, dict):
            # Dicts serialized into a string field are encoded in UHD device_addr
            # format ("key=val,key=val") so that C++ can parse them with
            # uhd::device_addr_t(std::string).
            return ",".join(f"{k}={v}" for k, v in value.items())
        return str(value)
    elif field_type == field_descriptor.TYPE_BOOL:
        return bool(value)
    elif field_type == field_descriptor.TYPE_BYTES:
        # Convert string to bytes if needed
        return value.encode("utf-8") if isinstance(value, str) else value
    elif field_type == field_descriptor.TYPE_MESSAGE:
        # For map fields, inspect the message type to determine key/value types
        if isinstance(value, dict) and hasattr(field_descriptor, "message_type"):
            message_type = field_descriptor.message_type
            # Check if this is a map entry (has 'key' and 'value' fields)
            if (
                hasattr(message_type, "fields_by_name")
                and "key" in message_type.fields_by_name
                and "value" in message_type.fields_by_name
            ):
                # This is a map field, convert keys and values according to their types
                key_field = message_type.fields_by_name["key"]
                value_field = message_type.fields_by_name["value"]
                converted_dict = {}
                for k, v in value.items():
                    converted_key = python_to_protobuf(k, key_field)
                    converted_value = python_to_protobuf(v, value_field)
                    converted_dict[converted_key] = converted_value
                return converted_dict

    # For complex types, return as-is and let protobuf handle it
    return value


def _sanitize_map_nones(value, field_name, log):
    """Return a copy of value with None entries replaced by "n/a".

    Protobuf map<string, string> fields cannot hold None values.  This helper
    centralises the coercion so callers do not need to sanitise their dicts
    before passing them to the RPC serialisation layer.
    """
    sanitized = {}
    for k, v in value.items():
        if v is None:
            log.warning(
                'casting parameter "{}" in field "{}" from None to "n/a"'.format(k, field_name)
            )
            sanitized[k] = "n/a"
        else:
            sanitized[k] = v
    return sanitized


def populate_response_field(response_obj, field_name, value, field_descriptor, log):
    """Populate a single field in the response object, handling nested types.

    Args:
        response_obj: The response protobuf message object
        field_name: Name of the field to populate
        value: The value to assign
        field_descriptor: The field descriptor from protobuf
        log: Logger object

    Raises:
        ValueError: If value cannot be converted to the expected type
        TypeError: If value type is incompatible with field type
    """
    field_type = field_descriptor.type

    # Handle repeated fields (lists)
    if field_descriptor.label == field_descriptor.LABEL_REPEATED:
        repeated_field = getattr(response_obj, field_name)

        # Check if this is actually a map field (not a true repeated field)
        if hasattr(repeated_field, "update") and hasattr(repeated_field, "items"):
            # This is a map field masquerading as repeated - handle as map
            if isinstance(value, dict):
                repeated_field.update(_sanitize_map_nones(value, field_name, log))
            else:
                log.trace(f"Warning: Expected dict for map field {field_name}, got {type(value)}")
        elif field_type == field_descriptor.TYPE_MESSAGE:
            # True repeated message type (like repeated Option)
            if isinstance(value, (list, tuple)):
                for item in value:
                    new_item = repeated_field.add()
                    populate_message_from_dict(new_item, item, log)
            else:
                # Single item, add as one element
                new_item = repeated_field.add()
                populate_message_from_dict(new_item, value, log)
        else:
            # Repeated primitive type
            if isinstance(value, (list, tuple)):
                for v in value:
                    if isinstance(v, (list, tuple)):
                        # Flatten one level: some methods return a list-of-lists
                        # (e.g. one inner list per converter). Proto3 has no
                        # nested repeated scalars, so flatten into a single sequence.
                        repeated_field.extend(
                            python_to_protobuf(item, field_descriptor) for item in v
                        )
                    else:
                        repeated_field.append(python_to_protobuf(v, field_descriptor))
            elif isinstance(value, dict):
                # dict assigned to repeated string: use keys as the list elements
                repeated_field.extend(python_to_protobuf(k, field_descriptor) for k in value.keys())
            else:
                repeated_field.append(python_to_protobuf(value, field_descriptor))

    # Handle map fields (dictionaries) and message fields
    elif field_type == field_descriptor.TYPE_MESSAGE:
        field_obj = getattr(response_obj, field_name)

        # Check if this is a map field by examining the field object type
        if hasattr(field_obj, "update") and hasattr(field_obj, "items"):
            # This is a map field
            if isinstance(value, dict):
                field_obj.update(_sanitize_map_nones(value, field_name, log))
            else:
                log.trace(f"Warning: Expected dict for map field {field_name}, got {type(value)}")
        else:
            # Regular message type
            if isinstance(value, dict):
                populate_message_from_dict(field_obj, value, log)
            else:
                setattr(response_obj, field_name, value)

    # Handle primitive fields
    else:
        converted_value = python_to_protobuf(value, field_descriptor)
        setattr(response_obj, field_name, converted_value)


def populate_message_from_dict(message_obj, data_dict, log):
    """Populate a protobuf message object from a dictionary.

    Args:
        message_obj: The protobuf message object to populate
        data_dict: Dictionary containing the data
        log: Logger object
    """
    if not isinstance(data_dict, dict):
        log.trace(f"Warning: Expected dict, got {type(data_dict)}")
        return

    descriptor = message_obj.DESCRIPTOR

    # Special handling for messages like Option that have a single 'data' map field
    if (
        len(descriptor.fields) == 1
        and descriptor.fields[0].name == "data"
        and descriptor.fields[0].type == descriptor.fields[0].TYPE_MESSAGE
    ):
        # This message has a single 'data' field that's a map
        # Put the entire dictionary into the 'data' field
        data_field = getattr(message_obj, "data")
        if hasattr(data_field, "update"):
            # Sanitize None values before conversion (protobuf map<string,string>
            # cannot hold None; coerce to "n/a" consistent with populate_response_field)
            # Convert string values to bytes if needed (eg, EEPROM data)
            converted_dict = python_to_protobuf(
                _sanitize_map_nones(data_dict, "data", log), descriptor.fields[0]
            )
            data_field.update(converted_dict)
        return

    # Regular field mapping
    for field_name, value in data_dict.items():
        if field_name in descriptor.fields_by_name:
            field_descriptor = descriptor.fields_by_name[field_name]
            populate_response_field(message_obj, field_name, value, field_descriptor, log)
        else:
            log.trace(
                f"Warning: Field {field_name} not found in message {type(message_obj).__name__}"
            )


def build_response_from_return_value(ret_val, response_type, log):
    """Build a protobuf response object from the return value of a function.

    Args:
        ret_val: The return value from the domain function
        response_type: The protobuf response message type

    Returns:
        An instance of response_type populated with ret_val data
    """
    response_obj = response_type()
    return_fields = [f for f in response_type.DESCRIPTOR.fields]

    if not return_fields:
        # No fields in response, return empty response
        return response_obj
    elif len(return_fields) == 1:
        # Single field response
        field = return_fields[0]
        populate_response_field(response_obj, field.name, ret_val, field, log)
    else:
        # Multiple fields - ret_val should be a dict or have matching attributes
        if isinstance(ret_val, dict):
            for field in return_fields:
                if field.name in ret_val:
                    populate_response_field(
                        response_obj, field.name, ret_val[field.name], field, log
                    )
        elif hasattr(ret_val, "__dict__"):
            # Object with attributes
            ret_dict = ret_val.__dict__
            for field in return_fields:
                if field.name in ret_dict:
                    populate_response_field(
                        response_obj, field.name, ret_dict[field.name], field, log
                    )
        else:
            # Try to match by position if it's a tuple/list
            if isinstance(ret_val, (tuple, list)) and len(ret_val) == len(return_fields):
                for field, value in zip(return_fields, ret_val):
                    populate_response_field(response_obj, field.name, value, field, log)
            else:
                raise ValueError(
                    f"Cannot map return value {type(ret_val)} to response with multiple fields"
                )

    return response_obj
