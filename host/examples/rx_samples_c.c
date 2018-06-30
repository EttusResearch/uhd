/*
 * Copyright 2015 Ettus Research LLC
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <uhd.h>

#include "getopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXECUTE_OR_GOTO(label, ...) \
    if(__VA_ARGS__){ \
        return_code = EXIT_FAILURE; \
        goto label; \
    }

void print_help(void){
    fprintf(stderr, "rx_samples_c - A simple RX example using UHD's C API\n\n"

                    "Options:\n"
                    "    -a (device args)\n"
                    "    -f (frequency in Hz)\n"
                    "    -r (sample rate in Hz)\n"
                    "    -g (gain)\n"
                    "    -n (number of samples to receive)\n"
                    "    -o (output filename, default = \"out.dat\")\n"
                    "    -v (enable verbose prints)\n"
                    "    -h (print this help message)\n");
};

int main(int argc, char* argv[])
{
    if(uhd_set_thread_priority(uhd_default_thread_priority, true)){
        fprintf(stderr, "Unable to set thread priority. Continuing anyway.\n");
    }

    int option = 0;
    double freq = 500e6;
    double rate = 1e6;
    double gain = 5.0;
    char* device_args = NULL;
    size_t channel = 0;
    char* filename = "out.dat";
    size_t n_samples = 1000000;
    bool verbose = false;
    int return_code = EXIT_SUCCESS;
    bool custom_filename = false;
    char error_string[512];

    // Process options
    while((option = getopt(argc, argv, "a:f:r:g:n:o:vh")) != -1){
        switch(option){
            case 'a':
                device_args = strdup(optarg);
                break;

            case 'f':
                freq = atof(optarg);
                break;

            case 'r':
                rate = atof(optarg);
                break;

            case 'g':
                gain = atof(optarg);
                break;

            case 'n':
                n_samples = atoi(optarg);
                break;

            case 'o':
                filename = strdup(optarg);
                custom_filename = true;
                break;

            case 'v':
                verbose = true;
                break;

            case 'h':
                print_help();
                goto free_option_strings;

            default:
                print_help();
                return_code = EXIT_FAILURE;
                goto free_option_strings;
        }
    }

    if (!device_args)
            device_args = strdup("");

    // Create USRP
    uhd_usrp_handle usrp;
    fprintf(stderr, "Creating USRP with args \"%s\"...\n", device_args);
    EXECUTE_OR_GOTO(free_option_strings,
        uhd_usrp_make(&usrp, device_args)
    )

    // Create RX streamer
    uhd_rx_streamer_handle rx_streamer;
    EXECUTE_OR_GOTO(free_usrp,
        uhd_rx_streamer_make(&rx_streamer)
    )

    // Create RX metadata
    uhd_rx_metadata_handle md;
    EXECUTE_OR_GOTO(free_rx_streamer,
        uhd_rx_metadata_make(&md)
    )

    // Create other necessary structs
    uhd_tune_request_t tune_request = {
        .target_freq = freq,
        .rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
        .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
    };
    uhd_tune_result_t tune_result;

    uhd_stream_args_t stream_args = {
        .cpu_format = "fc32",
        .otw_format = "sc16",
        .args = "",
        .channel_list = &channel,
        .n_channels = 1
    };

    uhd_stream_cmd_t stream_cmd = {
        .stream_mode = UHD_STREAM_MODE_NUM_SAMPS_AND_DONE,
        .num_samps = n_samples,
        .stream_now = true
    };

    size_t samps_per_buff;
    float *buff = NULL;
    void **buffs_ptr = NULL;
    FILE *fp = NULL;
    size_t num_acc_samps = 0;

    // Set rate
    fprintf(stderr, "Setting RX Rate: %f...\n", rate);
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_rx_rate(usrp, rate, channel)
    )

    // See what rate actually is
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_rx_rate(usrp, channel, &rate)
    )
    fprintf(stderr, "Actual RX Rate: %f...\n", rate);

    // Set gain
    fprintf(stderr, "Setting RX Gain: %f dB...\n", gain);
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_rx_gain(usrp, gain, channel, "")
    )

    // See what gain actually is
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_rx_gain(usrp, channel, "", &gain)
    )
    fprintf(stderr, "Actual RX Gain: %f...\n", gain);

    // Set frequency
    fprintf(stderr, "Setting RX frequency: %f MHz...\n", freq/1e6);
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_set_rx_freq(usrp, &tune_request, channel, &tune_result)
    )

    // See what frequency actually is
    EXECUTE_OR_GOTO(free_rx_metadata,
        uhd_usrp_get_rx_freq(usrp, channel, &freq)
    )
    fprintf(stderr, "Actual RX frequency: %f MHz...\n", freq / 1e6);

    // Set up streamer
    stream_args.channel_list = &channel;
    EXECUTE_OR_GOTO(free_rx_streamer,
        uhd_usrp_get_rx_stream(usrp, &stream_args, rx_streamer)
    )

    // Set up buffer
    EXECUTE_OR_GOTO(free_rx_streamer,
        uhd_rx_streamer_max_num_samps(rx_streamer, &samps_per_buff)
    )
    fprintf(stderr, "Buffer size in samples: %zu\n", samps_per_buff);
    buff = malloc(samps_per_buff * 2 * sizeof(float));
    buffs_ptr = (void**)&buff;

    // Issue stream command
    fprintf(stderr, "Issuing stream command.\n");
    EXECUTE_OR_GOTO(free_buffer,
        uhd_rx_streamer_issue_stream_cmd(rx_streamer, &stream_cmd)
    )

    // Set up file output
    fp = fopen(filename, "wb");

    // Actual streaming
    while (num_acc_samps < n_samples) {
        size_t num_rx_samps = 0;
        EXECUTE_OR_GOTO(close_file,
            uhd_rx_streamer_recv(rx_streamer, buffs_ptr, samps_per_buff, &md, 3.0, false, &num_rx_samps)
        )

        uhd_rx_metadata_error_code_t error_code;
        EXECUTE_OR_GOTO(close_file,
            uhd_rx_metadata_error_code(md, &error_code)
        )
        if(error_code != UHD_RX_METADATA_ERROR_CODE_NONE){
            fprintf(stderr, "Error code 0x%x was returned during streaming. Aborting.\n", return_code);
            goto close_file;
        }

        // Handle data
        fwrite(buff, sizeof(float) * 2, num_rx_samps, fp);
        if (verbose) {
            time_t full_secs;
            double frac_secs;
            uhd_rx_metadata_time_spec(md, &full_secs, &frac_secs);
            fprintf(stderr, "Received packet: %zu samples, %.f full secs, %f frac secs\n",
                    num_rx_samps,
                    difftime(full_secs, (time_t) 0),
                    frac_secs);
        }

        num_acc_samps += num_rx_samps;
    }

    // Cleanup
    close_file:
        fclose(fp);

    free_buffer:
        if(buff){
            if(verbose){
                fprintf(stderr, "Freeing buffer.\n");
            }
            free(buff);
        }
        buff = NULL;
        buffs_ptr = NULL;

    free_rx_streamer:
        if(verbose){
            fprintf(stderr, "Cleaning up RX streamer.\n");
        }
        uhd_rx_streamer_free(&rx_streamer);

    free_rx_metadata:
        if(verbose){
            fprintf(stderr, "Cleaning up RX metadata.\n");
        }
        uhd_rx_metadata_free(&md);

    free_usrp:
        if(verbose){
            fprintf(stderr, "Cleaning up USRP.\n");
        }
        if(return_code != EXIT_SUCCESS && usrp != NULL){
            uhd_usrp_last_error(usrp, error_string, 512);
            fprintf(stderr, "USRP reported the following error: %s\n", error_string);
        }
        uhd_usrp_free(&usrp);

    free_option_strings:
        if(device_args) {
            free(device_args);
        }
        if(custom_filename){
            free(filename);
        }

    fprintf(stderr, (return_code ? "Failure\n" : "Success\n"));
    return return_code;
}
