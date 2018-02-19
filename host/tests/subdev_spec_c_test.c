//
// Copyright 2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd.h>

#include <stdio.h>
#include <stdlib.h>

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
    uhd_subdev_spec_pair_t subdev_spec_pair1, subdev_spec_pair2;
    uhd_subdev_spec_handle subdev_spec1, subdev_spec2;
    size_t size1, size2, i;
    bool pairs_equal;
    char str_buffer[BUFFER_SIZE];

    printf("Testing subdevice specification...\n");
    return_code = EXIT_SUCCESS;

    // Create subdev spec
    UHD_TEST_EXECUTE_OR_GOTO(end_of_test,
        uhd_subdev_spec_make(&subdev_spec1, "A:AB B:AB")
    )

    // Convert to and from args string
    UHD_TEST_EXECUTE_OR_GOTO(free_subdev_spec1,
        uhd_subdev_spec_to_pp_string(subdev_spec1, str_buffer, BUFFER_SIZE)
    )
    printf("Pretty Print:\n%s", str_buffer);

    UHD_TEST_EXECUTE_OR_GOTO(free_subdev_spec1,
        uhd_subdev_spec_to_string(subdev_spec1, str_buffer, BUFFER_SIZE)
    )
    printf("Markup String: %s\n", str_buffer);

    // Make a second subdev spec from the first's markup string
    UHD_TEST_EXECUTE_OR_GOTO(free_subdev_spec2,
        uhd_subdev_spec_make(&subdev_spec2, str_buffer)
    )

    // Make sure both subdev specs are equal
    UHD_TEST_EXECUTE_OR_GOTO(free_subdev_spec2,
        uhd_subdev_spec_size(subdev_spec1, &size1)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_subdev_spec2,
        uhd_subdev_spec_size(subdev_spec2, &size2)
    )
    if(size1 != size2){
        printf("%s:%d: Sizes do not match. %lu vs. %lu\n", __FILE__, __LINE__,
               (unsigned long)size1, (unsigned long)size2);
        return_code = EXIT_FAILURE;
        goto free_subdev_spec2;
    }
    for(i = 0; i < size1; i++){
        UHD_TEST_EXECUTE_OR_GOTO(free_subdev_spec_pair1,
            uhd_subdev_spec_at(subdev_spec1, i, &subdev_spec_pair1)
        )
        UHD_TEST_EXECUTE_OR_GOTO(free_subdev_spec_pair2,
            uhd_subdev_spec_at(subdev_spec2, i, &subdev_spec_pair2)
        )
        UHD_TEST_EXECUTE_OR_GOTO(free_subdev_spec_pair2,
            uhd_subdev_spec_pairs_equal(&subdev_spec_pair1, &subdev_spec_pair2, &pairs_equal)
        )
        if(!pairs_equal){
            printf("%s:%d: Subdev spec pairs are not equal.\n"
                   "    db_name: %s vs. %s\n"
                   "    sd_name: %s vs. %s\n",
                   __FILE__, __LINE__,
                   subdev_spec_pair1.db_name, subdev_spec_pair2.db_name,
                   subdev_spec_pair1.sd_name, subdev_spec_pair2.sd_name
                  );
            return_code = EXIT_FAILURE;
            goto free_subdev_spec_pair2;
        }
        uhd_subdev_spec_pair_free(&subdev_spec_pair1);
        uhd_subdev_spec_pair_free(&subdev_spec_pair2);
    }

    // Cleanup (and error report, if needed)

    free_subdev_spec_pair2:
        uhd_subdev_spec_pair_free(&subdev_spec_pair2);

    free_subdev_spec_pair1:
        uhd_subdev_spec_pair_free(&subdev_spec_pair1);

    free_subdev_spec2:
        if(return_code){
            uhd_subdev_spec_last_error(subdev_spec2, str_buffer, BUFFER_SIZE);
            fprintf(stderr, "subdev_spec2 error: %s\n", str_buffer);
        }
        uhd_subdev_spec_free(&subdev_spec2);

    free_subdev_spec1:
        if(return_code){
            uhd_subdev_spec_last_error(subdev_spec1, str_buffer, BUFFER_SIZE);
            fprintf(stderr, "subdev_spec1 error: %s\n", str_buffer);
        }
        uhd_subdev_spec_free(&subdev_spec1);

    end_of_test:
        if(!return_code){
            printf("\nNo errors detected.\n");
        }
        return return_code;
}
