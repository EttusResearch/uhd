//
// Copyright 2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UHD_TEST_EXECUTE_OR_GOTO(label, ...) \
    if(__VA_ARGS__){ \
        fprintf(stderr, "Error occurred at %s:%d\n", __FILE__, (__LINE__-1)); \
        return_code = EXIT_FAILURE; \
        goto label; \
    }

#define BUFFER_SIZE 1024

int main(){

    // Variables
    int return_code;
    uhd_string_vector_handle string_vector;
    size_t size;
    char str_buffer[BUFFER_SIZE];

    return_code = EXIT_SUCCESS;

    // Create string_vector
    UHD_TEST_EXECUTE_OR_GOTO(end_of_test,
        uhd_string_vector_make(&string_vector)
    )

    // Add values
    UHD_TEST_EXECUTE_OR_GOTO(free_string_vector,
        uhd_string_vector_push_back(&string_vector, "foo")
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_string_vector,
        uhd_string_vector_push_back(&string_vector, "bar")
    )

    // Check size
    UHD_TEST_EXECUTE_OR_GOTO(free_string_vector,
        uhd_string_vector_size(string_vector, &size)
    )
    if(size != 2){
        return_code = EXIT_FAILURE;
        fprintf(stderr, "%s:%d: Invalid size: %lu vs. 2",
                        __FILE__, __LINE__, (unsigned long)size);
        goto free_string_vector;
    }

    // Make sure we get right value
    UHD_TEST_EXECUTE_OR_GOTO(free_string_vector,
        uhd_string_vector_at(string_vector, 1, str_buffer, BUFFER_SIZE)
    )
    if(strcmp(str_buffer, "bar")){
        return_code = EXIT_FAILURE;
        fprintf(stderr, "%s:%d: Mismatched daughterboard serial: \"%s\" vs. \"key3=value3,key4=value4\"\n",
                        __FILE__, __LINE__,
                        str_buffer);
    }

    free_string_vector:
        if(return_code){
            uhd_string_vector_last_error(string_vector, str_buffer, BUFFER_SIZE);
            fprintf(stderr, "string_vector error: %s\n", str_buffer);
        }
        uhd_string_vector_free(&string_vector);

    end_of_test:
        if(!return_code){
            printf("\nNo errors detected\n");
        }
        return return_code;
}
