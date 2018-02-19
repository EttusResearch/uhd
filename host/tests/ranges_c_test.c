//
// Copyright 2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define UHD_TEST_EXECUTE_OR_GOTO(label, ...) \
    if(__VA_ARGS__){ \
        fprintf(stderr, "Error occurred at %s:%d\n", __FILE__, (__LINE__-1)); \
        return_code = EXIT_FAILURE; \
        goto label; \
    }

#define UHD_TEST_CHECK_CLOSE(lhs, rhs) (fabs(lhs-rhs) < 0.001)

#define BUFFER_SIZE 1024

static UHD_INLINE int test_range_values(
    const uhd_range_t *range,
    double start_input, double stop_input, double step_input
){
    if(!UHD_TEST_CHECK_CLOSE(range->start, start_input)){
        fprintf(stderr, "%s:%d: Starts did not match: %f vs. %f\n",
                        __FILE__, __LINE__,
                        range->start, start_input);
        return 1;
    }
    if(!UHD_TEST_CHECK_CLOSE(range->stop, stop_input)){
        fprintf(stderr, "%s:%d: Stops did not match: %f vs. %f\n",
                        __FILE__, __LINE__,
                        range->stop, stop_input);
        return 1;
    }
    if(!UHD_TEST_CHECK_CLOSE(range->step, step_input)){
        fprintf(stderr, "%s:%d: Steps did not match: %f vs. %f\n",
                        __FILE__, __LINE__,
                        range->step, step_input);
        return 1;
    }

    return 0;
}

static UHD_INLINE int test_meta_range_values(
    uhd_meta_range_handle meta_range,
    double start_input, double stop_input, double step_input,
    double start_test, double stop_test, double step_test
){
    // Add range
    uhd_range_t range;
    range.start = start_input;
    range.stop = stop_input;
    range.step = step_input;
    if(uhd_meta_range_push_back(meta_range, &range)){
        fprintf(stderr, "%s:%d: Failed to push back range.\n",
                __FILE__, __LINE__);
        return 1;
    }

    // Test bounds
    uhd_meta_range_start(meta_range, &range.start);
    if(!UHD_TEST_CHECK_CLOSE(range.start, start_test)){
        fprintf(stderr, "%s:%d: Starts did not match: %f vs. %f\n",
                        __FILE__, __LINE__,
                        range.start, start_test);
        return 1;
    }
    uhd_meta_range_stop(meta_range, &range.stop);
    if(!UHD_TEST_CHECK_CLOSE(range.stop, stop_test)){
        fprintf(stderr, "%s:%d: Stops did not match: %f vs. %f\n",
                        __FILE__, __LINE__,
                        range.stop, stop_test);
        return 1;
    }
    uhd_meta_range_step(meta_range, &range.step);
    if(!UHD_TEST_CHECK_CLOSE(range.step, step_test)){
        fprintf(stderr, "%s:%d: Steps did not match: %f vs. %f\n",
                        __FILE__, __LINE__,
                        range.step, step_test);
        return 1;
    }

    return 0;
}

static UHD_INLINE int test_meta_range_clip(
    uhd_meta_range_handle meta_range,
    double clip_value, double test_value,
    bool clip_step
){
    double clip_result;

    uhd_meta_range_clip(meta_range, clip_value, clip_step, &clip_result);
    if(!UHD_TEST_CHECK_CLOSE(test_value, clip_result)){
        fprintf(stderr, "%s:%d: Values did not match: %f vs. %f\n",
                        __FILE__, __LINE__,
                        test_value, clip_result);
        return 1;
    }

    return 0;
}

int main(){

    // Variables
    int return_code;
    uhd_range_t range;
    uhd_meta_range_handle meta_range1, meta_range2;
    char str_buffer[BUFFER_SIZE];
    size_t size;

    return_code = EXIT_SUCCESS;

    // Create meta range 1
    UHD_TEST_EXECUTE_OR_GOTO(end_of_test,
        uhd_meta_range_make(&meta_range1)
    )

    // Test bounds
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        test_meta_range_values(meta_range1, -1.0, +1.0, 0.1,
                                            -1.0, +1.0, 0.1)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        test_meta_range_values(meta_range1, 40.0, 60.0, 1.0,
                                            -1.0, 60.0, 0.1)
    )
    uhd_meta_range_at(meta_range1, 0, &range);
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        test_range_values(&range, -1.0, +1.0, 0.1)
    )

    // Check meta range size
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        uhd_meta_range_size(meta_range1, &size)
    )
    if(size != 2){
        fprintf(stderr, "%s:%d: Invalid size: %lu vs. 2",
                        __FILE__, __LINE__,
                        (unsigned long)size);
        goto free_meta_range1;
    }

    // Test clipping (with steps)
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        test_meta_range_clip(meta_range1, -30.0, -1.0, false)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        test_meta_range_clip(meta_range1, 70.0, 60.0, false)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        test_meta_range_clip(meta_range1, 20.0, 1.0, false)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        test_meta_range_clip(meta_range1, 50.0, 50.0, false)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        test_meta_range_clip(meta_range1, 50.9, 50.9, false)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        test_meta_range_clip(meta_range1, 50.9, 51.0, true)
    )

    // Create meta range 2
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range1,
        uhd_meta_range_make(&meta_range2)
    )
    range.step = 0.0;
    range.start = range.stop = 1.;
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range2,
        uhd_meta_range_push_back(meta_range2, &range)
    )
    range.start = range.stop = 2.;
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range2,
        uhd_meta_range_push_back(meta_range2, &range)
    )
    range.start = range.stop = 3.;
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range2,
        uhd_meta_range_push_back(meta_range2, &range)
    )

    // Test clipping (without steps)
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range2,
        test_meta_range_clip(meta_range2, 2., 2., true)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range2,
        test_meta_range_clip(meta_range2, 0., 1., true)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range2,
        test_meta_range_clip(meta_range2, 1.2, 1., true)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range2,
        test_meta_range_clip(meta_range2, 3.1, 3., true)
    )
    UHD_TEST_EXECUTE_OR_GOTO(free_meta_range2,
        test_meta_range_clip(meta_range2, 4., 3., true)
    )

    free_meta_range2:
        if(return_code){
            uhd_meta_range_last_error(meta_range2, str_buffer, BUFFER_SIZE);
            fprintf(stderr, "meta_range2 error: %s\n", str_buffer);
        }
        uhd_meta_range_free(&meta_range1);

    free_meta_range1:
        if(return_code){
            uhd_meta_range_last_error(meta_range1, str_buffer, BUFFER_SIZE);
            fprintf(stderr, "meta_range1 error: %s\n", str_buffer);
        }
        uhd_meta_range_free(&meta_range1);

    end_of_test:
        if(!return_code){
            printf("\nNo errors detected.\n");
        }
        return return_code;
}
