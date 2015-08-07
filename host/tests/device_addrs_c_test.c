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
    uhd_device_addrs_handle device_addrs;
    size_t size;
    char str_buffer[BUFFER_SIZE];

    return_code = EXIT_SUCCESS;

    // Create device_addrs
    UHD_TEST_EXECUTE_OR_GOTO(end_of_test,
        uhd_device_addrs_make(&device_addrs)
    )

    // Add values
    UHD_TEST_EXECUTE_OR_GOTO(free_device_addrs,
	uhd_device_addrs_push_back(device_addrs, "key1=value1,key2=value2")
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_device_addrs,
	uhd_device_addrs_push_back(device_addrs, "key3=value3,key4=value4")
    )

    // Check size
    UHD_TEST_EXECUTE_OR_GOTO(free_device_addrs,
	uhd_device_addrs_size(device_addrs, &size)
    )
    if(size != 2){
	return_code = EXIT_FAILURE;
        fprintf(stderr, "%s:%d: Invalid size: %lu vs. 2",
                        __FILE__, __LINE__,size);
        goto free_device_addrs;
    }

    // Make sure we get right value
    UHD_TEST_EXECUTE_OR_GOTO(free_device_addrs,
	uhd_device_addrs_at(device_addrs, 1, str_buffer, BUFFER_SIZE)
    )
    if(strcmp(str_buffer, "key3=value3,key4=value4")){
        return_code = EXIT_FAILURE;
        fprintf(stderr, "%s:%d: Mismatched daughterboard serial: \"%s\" vs. \"key3=value3,key4=value4\"\n",
                        __FILE__, __LINE__,
                        str_buffer);
    }

    free_device_addrs:
        if(return_code){
	    uhd_device_addrs_last_error(device_addrs, str_buffer, BUFFER_SIZE);
            fprintf(stderr, "device_addrs error: %s\n", str_buffer);
	}
	uhd_device_addrs_free(&device_addrs);

    end_of_test:
        if(!return_code){
            printf("\nNo errors detected\n");
        }
        return return_code;
}
