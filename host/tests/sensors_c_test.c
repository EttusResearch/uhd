//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define UHD_TEST_EXECUTE_OR_GOTO(label, ...) \
    if(__VA_ARGS__){ \
        fprintf(stderr, "Error occurred at %s:%d\n", __FILE__, (__LINE__-1)); \
        return_code = EXIT_FAILURE; \
        goto label; \
    }

#define UHD_TEST_CHECK_CLOSE(lhs, rhs) (fabs(lhs-rhs) < 0.001)

#define BUFFER_SIZE 1024

int main(){

    // Variables
    int return_code;
    uhd_sensor_value_handle boolean_sensor, integer_sensor, realnum_sensor, string_sensor;
    uhd_sensor_value_data_type_t sensor_type;
    bool bool_out;
    int int_out;
    double realnum_out;
    char str_buffer[BUFFER_SIZE];

    return_code = EXIT_SUCCESS;

    /*
     * Test a sensor made from a boolean
     */

    // Create the sensor
    UHD_TEST_EXECUTE_OR_GOTO(end_of_test,
        uhd_sensor_value_make_from_bool(
            &boolean_sensor,
            "Bool sensor", false,
            "True", "False"
        )
    )

    // Check the name
    UHD_TEST_EXECUTE_OR_GOTO(free_boolean_sensor,
        uhd_sensor_value_name(
            boolean_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "Bool sensor")){
        fprintf(stderr, "%s:%d: Boolean sensor name invalid: \"%s\" vs. \"false\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_boolean_sensor;
    }

    // Check the value
    UHD_TEST_EXECUTE_OR_GOTO(free_boolean_sensor,
        uhd_sensor_value_value(
            boolean_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "false")){
        fprintf(stderr, "%s:%d: Boolean sensor value invalid: \"%s\" vs. \"false\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_boolean_sensor;
    }

    // Check the unit
    UHD_TEST_EXECUTE_OR_GOTO(free_boolean_sensor,
        uhd_sensor_value_unit(
            boolean_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "False")){
        fprintf(stderr, "%s:%d: Boolean sensor unit invalid: \"%s\" vs. \"False\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_boolean_sensor;
    }

    // Check the type
    UHD_TEST_EXECUTE_OR_GOTO(free_boolean_sensor,
        uhd_sensor_value_data_type(
            boolean_sensor,
            &sensor_type
        )
    )
    if(sensor_type != UHD_SENSOR_VALUE_BOOLEAN){
        fprintf(stderr, "%s:%d: Wrong sensor type detected: %d vs. %d\n",
                        __FILE__, __LINE__,
                        sensor_type, UHD_SENSOR_VALUE_BOOLEAN);
        return_code = EXIT_FAILURE;
        goto free_boolean_sensor;
    }

    bool_out = false;
    // Check the casted value
    UHD_TEST_EXECUTE_OR_GOTO(free_boolean_sensor,
        uhd_sensor_value_to_bool(
            boolean_sensor,
            &bool_out
        )
    )
    if(bool_out){
        fprintf(stderr, "%s:%d: Boolean sensor value invalid: true vs. false\n",
                        __FILE__, __LINE__);
        return_code = EXIT_FAILURE;
        goto free_boolean_sensor;
    }

    /*
     * Test a sensor made from a integer
     */

    // Create the sensor
    UHD_TEST_EXECUTE_OR_GOTO(free_boolean_sensor,
        uhd_sensor_value_make_from_int(
            &integer_sensor,
            "Int sensor", 50,
            "Int type", "%d"
        )
    )

    // Check the name
    UHD_TEST_EXECUTE_OR_GOTO(free_integer_sensor,
        uhd_sensor_value_name(
            integer_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "Int sensor")){
        fprintf(stderr, "%s:%d: Integer sensor name invalid: \"%s\" vs. \"Int sensor\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_integer_sensor;
    }

    // Check the value
    UHD_TEST_EXECUTE_OR_GOTO(free_integer_sensor,
        uhd_sensor_value_value(
            integer_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "50")){
        fprintf(stderr, "%s:%d: Integer sensor value invalid: \"%s\" vs. \"50\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_integer_sensor;
    }

    // Check the unit
    UHD_TEST_EXECUTE_OR_GOTO(free_integer_sensor,
        uhd_sensor_value_unit(
            integer_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "Int type")){
        fprintf(stderr, "%s:%d: Integer sensor unit invalid: \"%s\" vs. \"Int type\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_integer_sensor;
    }

    // Check the type
    UHD_TEST_EXECUTE_OR_GOTO(free_integer_sensor,
        uhd_sensor_value_data_type(
            integer_sensor,
            &sensor_type
        )
    )
    if(sensor_type != UHD_SENSOR_VALUE_INTEGER){
        fprintf(stderr, "%s:%d: Wrong sensor type detected: %d vs. %d\n",
                        __FILE__, __LINE__,
                        sensor_type, UHD_SENSOR_VALUE_INTEGER);
        return_code = EXIT_FAILURE;
        goto free_integer_sensor;
    }

    // Check the casted value
    UHD_TEST_EXECUTE_OR_GOTO(free_integer_sensor,
        uhd_sensor_value_to_int(
            integer_sensor,
            &int_out
        )
    )
    if(int_out != 50){
        fprintf(stderr, "%s:%d: Integer sensor value invalid: %d vs. 50\n",
                        __FILE__, __LINE__,
                        int_out);
        return_code = EXIT_FAILURE;
        goto free_integer_sensor;
    }

    /*
     * Test a sensor made from a real number
     */

    // Create the sensor
    UHD_TEST_EXECUTE_OR_GOTO(free_integer_sensor,
        uhd_sensor_value_make_from_realnum(
            &realnum_sensor,
            "Realnum sensor", 50.0,
            "Realnum type", "%d"
        )
    )

    // Check the name
    UHD_TEST_EXECUTE_OR_GOTO(free_realnum_sensor,
        uhd_sensor_value_name(
            realnum_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "Realnum sensor")){
        fprintf(stderr, "%s:%d: Realnum sensor name invalid: \"%s\" vs. \"Realnum sensor\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_realnum_sensor;
    }

    // Check the value
    UHD_TEST_EXECUTE_OR_GOTO(free_realnum_sensor,
        uhd_sensor_value_value(
            realnum_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "50")){
        fprintf(stderr, "%s:%d: Realnum sensor value invalid: \"%s\" vs. \"50\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_realnum_sensor;
    }

    // Check the unit
    UHD_TEST_EXECUTE_OR_GOTO(free_realnum_sensor,
        uhd_sensor_value_unit(
            realnum_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "Realnum type")){
        fprintf(stderr, "%s:%d: Realnum sensor unit invalid: \"%s\" vs. \"Realnum type\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_realnum_sensor;
    }

    // Check the type
    UHD_TEST_EXECUTE_OR_GOTO(free_realnum_sensor,
        uhd_sensor_value_data_type(
            realnum_sensor,
            &sensor_type
        )
    )
    if(sensor_type != UHD_SENSOR_VALUE_REALNUM){
        fprintf(stderr, "%s:%d: Wrong sensor type detected: %d vs. %d\n",
                        __FILE__, __LINE__,
                        sensor_type, UHD_SENSOR_VALUE_REALNUM);
        return_code = EXIT_FAILURE;
        goto free_realnum_sensor;
    }

    // Check the casted value
    UHD_TEST_EXECUTE_OR_GOTO(free_realnum_sensor,
        uhd_sensor_value_to_realnum(
            realnum_sensor,
            &realnum_out
        )
    )
    if(realnum_out != 50.0){
        fprintf(stderr, "%s:%d: Realnum sensor value invalid: %2.1f vs. 50.0\n",
                        __FILE__, __LINE__,
                        realnum_out);
        return_code = EXIT_FAILURE;
        goto free_realnum_sensor;
    }

    /*
     * Test a sensor made from a string
     */

    // Create the sensor
    UHD_TEST_EXECUTE_OR_GOTO(free_realnum_sensor,
        uhd_sensor_value_make_from_string(
            &string_sensor,
            "String sensor",
            "String value",
            "String unit"
        )
    )

    // Check the name
    UHD_TEST_EXECUTE_OR_GOTO(free_string_sensor,
        uhd_sensor_value_name(
            string_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "String sensor")){
        fprintf(stderr, "%s:%d: String sensor name invalid: \"%s\" vs. \"String sensor\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_string_sensor;
    }

    // Check the value
    UHD_TEST_EXECUTE_OR_GOTO(free_string_sensor,
        uhd_sensor_value_value(
            string_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "String value")){
        fprintf(stderr, "%s:%d: String sensor value invalid: \"%s\" vs. \"String value\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_string_sensor;
    }

    // Check the unit
    UHD_TEST_EXECUTE_OR_GOTO(free_string_sensor,
        uhd_sensor_value_unit(
            string_sensor,
            str_buffer, BUFFER_SIZE
        )
    )
    if(strcmp(str_buffer, "String unit")){
        fprintf(stderr, "%s:%d: String sensor unit invalid: \"%s\" vs. \"String unit\"\n",
                        __FILE__, __LINE__, str_buffer);
        return_code = EXIT_FAILURE;
        goto free_string_sensor;
    }

    // Check the type
    UHD_TEST_EXECUTE_OR_GOTO(free_string_sensor,
        uhd_sensor_value_data_type(
            string_sensor,
            &sensor_type
        )
    )
    if(sensor_type != UHD_SENSOR_VALUE_STRING){
        fprintf(stderr, "%s:%d: Wrong sensor type detected: %d vs. %d\n",
                        __FILE__, __LINE__,
                        sensor_type, UHD_SENSOR_VALUE_STRING);
        return_code = EXIT_FAILURE;
        goto free_string_sensor;
    }

    /*
     * Cleanup
     */

    free_string_sensor:
        if(return_code){
            uhd_sensor_value_last_error(string_sensor, str_buffer, BUFFER_SIZE);
            fprintf(stderr, "string_sensor error: %s\n", str_buffer);
        }
        uhd_sensor_value_free(&string_sensor);

    free_realnum_sensor:
        if(return_code){
            uhd_sensor_value_last_error(realnum_sensor, str_buffer, BUFFER_SIZE);
            fprintf(stderr, "realnum_sensor error: %s\n", str_buffer);
        }
        uhd_sensor_value_free(&realnum_sensor);

    free_integer_sensor:
        if(return_code){
            uhd_sensor_value_last_error(integer_sensor, str_buffer, BUFFER_SIZE);
            fprintf(stderr, "integer_sensor error: %s\n", str_buffer);
        }
        uhd_sensor_value_free(&integer_sensor);

    free_boolean_sensor:
        if(return_code){
            uhd_sensor_value_last_error(boolean_sensor, str_buffer, BUFFER_SIZE);
            fprintf(stderr, "boolean_sensor error: %s\n", str_buffer);
        }
        uhd_sensor_value_free(&boolean_sensor);

    end_of_test:
        if(!return_code){
            printf("\nNo errors detected.\n");
        }
        return return_code;
}
