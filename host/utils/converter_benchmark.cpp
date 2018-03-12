//
// Copyright 2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/safe_main.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/timer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <iomanip>
#include <map>
#include <complex>
#include <stdint.h>

namespace po = boost::program_options;
using namespace uhd::convert;

enum buf_init_t {
    RANDOM, INC
};

// Convert `sc16_item32_le' -> `sc16'
// Finds the first _ in format and returns the string
// until then. Returns the entire string if no _ is found.
std::string format_to_type(const std::string &format)
{
    std::string ret_val = "";
    for (size_t i = 0; i < format.length(); i++) {
        if (format[i] == '_') {
            return ret_val;
        }
        ret_val.append(1, format[i]);
    }

    return ret_val;
}

void configure_conv(
        converter::sptr conv,
        const std::string &in_type,
        const std::string &out_type
) {
    if (in_type == "sc16") {
        if (out_type == "fc32") {
            std::cout << "Setting scalar to 32767." << std::endl;
            conv->set_scalar(32767.);
            return;
        }
    }

    if (in_type == "fc32") {
        if (out_type == "sc16") {
            std::cout << "Setting scalar to 32767." << std::endl;
            conv->set_scalar(32767.);
            return;
        }
    }

    std::cout << "No configuration required." << std::endl;
}

template <typename T>
void init_random_vector_complex_float(std::vector<char> &buf_ptr, const size_t n_items)
{
    std::complex<T> * const buf = reinterpret_cast<std::complex<T> * const>(&buf_ptr[0]);
    for (size_t i = 0; i < n_items; i++) {
        buf[i] = std::complex<T>(
            T(std::rand()/(RAND_MAX/2.0) - 1),
            T(std::rand()/(RAND_MAX/2.0) - 1)
        );
    }
}

template <typename T>
void init_random_vector_complex_int(std::vector<char> &buf_ptr, const size_t n_items)
{
    std::complex<T> * const buf = reinterpret_cast<std::complex<T> * const>(&buf_ptr[0]);
    for (size_t i = 0; i < n_items; i++) {
        buf[i] = std::complex<T>(T(std::rand()), T(std::rand()));
    }
}

struct item32_sc12_3x
{
    uint32_t line0;
    uint32_t line1;
    uint32_t line2;
};

template <typename T>
void init_random_vector_complex_sc12(std::vector<char> &buf_ptr, const size_t n_items)
{
    item32_sc12_3x *const buf = reinterpret_cast<item32_sc12_3x * const>(&buf_ptr[0]);
    if (n_items % 4) throw std::invalid_argument("");

    for (size_t i = 0; i < n_items / 4; i++) {
        int16_t iq[8];
        for (auto &k : iq) k = rand() & 0xfff;
        buf[i].line0 = iq[0] << 20 | iq[1] <<  8 | iq[2] >> 4;
        buf[i].line1 = iq[2] << 28 | iq[3] << 16 | iq[4] << 4 | iq[5] >> 8;
        buf[i].line2 = iq[5] << 24 | iq[6] << 12 | iq[7] << 0;
    }
}

template <typename T>
void init_random_vector_real_int(std::vector<char> &buf_ptr, size_t n_items)
{
    T * const buf = reinterpret_cast<T * const>(&buf_ptr[0]);
    for (size_t i = 0; i < n_items; i++) {
        buf[i] = T(std::rand());
    }
}

// Fill a buffer with increasing numbers
template <typename T>
void init_inc_vector(std::vector<char> &buf_ptr, size_t n_items)
{
    T * const buf = reinterpret_cast<T * const>(&buf_ptr[0]);
    for (size_t i = 0; i < n_items; i++) {
        buf[i] = T(i);
    }
}

void init_buffers(
        std::vector< std::vector<char> > &buf,
        const std::string &type,
        size_t bytes_per_item,
        buf_init_t buf_seed_mode
) {
    if (buf.empty()) {
        return;
    }
    size_t n_items = buf[0].size() / bytes_per_item;

    /// Fill with incrementing integers
    if (buf_seed_mode == INC) {
        for (size_t i = 0; i < buf.size(); i++) {
            if (type == "sc8") {
                init_inc_vector< std::complex<int8_t> >(buf[i], n_items);
            } else if (type == "sc16") {
                init_inc_vector< std::complex<int16_t> >(buf[i], n_items);
            } else if (type == "sc32") {
                init_inc_vector< std::complex<int32_t> >(buf[i], n_items);
            } else if (type == "fc32") {
                init_inc_vector< std::complex<float> >(buf[i], n_items);
            } else if (type == "fc64") {
                init_inc_vector< std::complex<double> >(buf[i], n_items);
            } else if (type == "s8") {
                init_inc_vector< int8_t >(buf[i], n_items);
            } else if (type == "s16") {
                init_inc_vector< int16_t >(buf[i], n_items);
            } else if (type == "item32") {
                init_inc_vector< uint32_t >(buf[i], n_items);
                init_random_vector_real_int<uint32_t>(buf[i], n_items);
            } else {
                throw uhd::runtime_error(str(
                            boost::format("Cannot handle data type: %s") % type
                ));
            }
        }

        return;
    }

    assert(buf_seed_mode == RANDOM);

    /// Fill with random data
    for (size_t i = 0; i < buf.size(); i++) {
        if (type == "sc8") {
            init_random_vector_complex_int<int8_t>(buf[i], n_items);
        } else if (type == "sc12") {
            init_random_vector_complex_sc12<int16_t>(buf[i], n_items);
        } else if (type == "sc16") {
            init_random_vector_complex_int<int16_t>(buf[i], n_items);
        } else if (type == "sc32") {
            init_random_vector_complex_int<int32_t>(buf[i], n_items);
        } else if (type == "fc32") {
            init_random_vector_complex_float<float>(buf[i], n_items);
        } else if (type == "fc64") {
            init_random_vector_complex_float<double>(buf[i], n_items);
        } else if (type == "s8") {
            init_random_vector_real_int<int8_t>(buf[i], n_items);
        } else if (type == "s16") {
            init_random_vector_real_int<int16_t>(buf[i], n_items);
        } else if (type == "item32") {
            init_random_vector_real_int<uint32_t>(buf[i], n_items);
        } else {
            throw uhd::runtime_error(str(
                boost::format("Cannot handle data type: %s") % type
            ));
        }
    }
}

// Returns time elapsed
double run_benchmark(
        converter::sptr conv,
        const std::vector<const void *> &input_buf_refs,
        const std::vector<void *> &output_buf_refs,
        size_t n_items,
        size_t iterations
) {
    boost::timer benchmark_timer;
    for (size_t i = 0; i < iterations; i++) {
        conv->conv(input_buf_refs, output_buf_refs, n_items);
    }
    return benchmark_timer.elapsed();
}

template <typename T>
std::string void_ptr_to_hexstring(const void *v_ptr, size_t index)
{
    const T *ptr = reinterpret_cast<const T *>(v_ptr);
    return str(boost::format("%X") % ptr[index]);
}

std::string item_to_hexstring(
    const void *v_ptr,
    size_t index,
    const std::string &type
) {
    if (type == "fc32") {
        return void_ptr_to_hexstring<uint64_t>(v_ptr, index);
    }
    else if (type == "sc16" || type == "item32") {
        return void_ptr_to_hexstring<uint32_t>(v_ptr, index);
    }
    else if (type == "sc8" || type == "s16") {
        return void_ptr_to_hexstring<uint16_t>(v_ptr, index);
    }
    else if (type == "u8") {
        return void_ptr_to_hexstring<uint8_t>(v_ptr, index);
    }
    else {
        return str(boost::format("<unhandled data type: %s>") % type);
    }
}

std::string item_to_string(
    const void *v_ptr,
    size_t index,
    const std::string &type,
    const bool print_hex
) {
    if (print_hex) {
        return item_to_hexstring(v_ptr, index, type);
    }

    if (type == "sc16") {
        const std::complex<int16_t> *ptr = reinterpret_cast<const std::complex<int16_t> *>(v_ptr);
        return boost::lexical_cast<std::string>(ptr[index]);
    }
    else if (type == "sc8") {
        const std::complex<int8_t> *ptr = reinterpret_cast<const std::complex<int8_t> *>(v_ptr);
        return boost::lexical_cast<std::string>(ptr[index]);
    }
    else if (type == "fc32") {
        const std::complex<float> *ptr = reinterpret_cast<const std::complex<float> *>(v_ptr);
        return boost::lexical_cast<std::string>(ptr[index]);
    }
    else if (type == "item32") {
        const uint32_t *ptr = reinterpret_cast<const uint32_t *>(v_ptr);
        return boost::lexical_cast<std::string>(ptr[index]);
    }
    else if (type == "s16") {
        const int16_t *ptr = reinterpret_cast<const int16_t *>(v_ptr);
        return boost::lexical_cast<std::string>(ptr[index]);
    }
    else {
        return str(boost::format("<unhandled data type: %s>") % type);
    }
}

int UHD_SAFE_MAIN(int argc, char *argv[])
{
    std::string in_format, out_format;
    std::string priorities;
    std::string seed_mode;
    priority_type prio = -1, max_prio;
    size_t iterations, n_samples;
    size_t n_inputs, n_outputs;
    buf_init_t buf_seed_mode = RANDOM;

    /// Command line arguments
    po::options_description desc("Converter benchmark options:");
    desc.add_options()
        ("help", "help message")
        ("in",  po::value<std::string>(&in_format), "Input format (e.g. 'sc16')")
        ("out", po::value<std::string>(&out_format), "Output format (e.g. 'sc16')")
        ("samples",  po::value<size_t>(&n_samples)->default_value(1000000), "Number of samples per iteration")
        ("iterations",  po::value<size_t>(&iterations)->default_value(10000), "Number of iterations per benchmark")
        ("priorities", po::value<std::string>(&priorities)->default_value("default"), "Converter priorities. Can be 'default', 'all', or a comma-separated list of priorities.")
        ("max-prio", po::value<priority_type>(&max_prio)->default_value(4), "Largest available priority (advanced feature)")
        ("n-inputs",   po::value<size_t>(&n_inputs)->default_value(1),  "Number of input vectors")
        ("n-outputs",  po::value<size_t>(&n_outputs)->default_value(1), "Number of output vectors")
        ("debug-converter", "Skip benchmark and print conversion results. Implies iterations==1 and will only run on a single converter.")
        ("seed-mode", po::value<std::string>(&seed_mode)->default_value("random"), "How to initialize the data: random, incremental")
        ("hex", "When using debug mode, dump memory in hex")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD Converter Benchmark Tool %s") % desc << std::endl << std::endl;
        std::cout << "  Use this to benchmark or debug converters." << std::endl
                  << "  When using as a benchmark tool, it will output the execution time\n"
                     "  for every conversion run in CSV format to stdout. Every line between\n"
                     "  the output delimiters {{{ }}} is of the format: <PRIO>,<TIME IN MILLISECONDS>\n"
                     "  When using for converter debugging, every line is formatted as\n"
                     "  <INPUT_VALUE>,<OUTPUT_VALUE>\n" << std::endl;
        return EXIT_FAILURE;
    }

    // Parse more arguments
    if (seed_mode == "incremental") {
        buf_seed_mode = INC;
    } else if (seed_mode == "random") {
        buf_seed_mode = RANDOM;
    } else {
        std::cout << "Invalid argument: --seed-mode must be either 'incremental' or 'random'." << std::endl;
    }

    bool debug_mode = vm.count("debug-converter") > 0;
    if (debug_mode) {
        iterations = 1;
    }

    /// Create the converter(s) //////////////////////////////////////////////
    id_type converter_id;
    converter_id.input_format  = in_format;
    converter_id.output_format = out_format;
    converter_id.num_inputs    = n_inputs;
    converter_id.num_outputs   = n_outputs;
    std::cout << "Requested converter format: " << converter_id.to_string()
              << std::endl;
    uhd::dict<priority_type, converter::sptr> conv_list;
    if (priorities == "default" or priorities.empty()) {
        try {
            conv_list[prio] = get_converter(converter_id, prio)(); // Can throw a uhd::key_error
        } catch(const uhd::key_error &) {
            std::cout << "No converters found." << std::endl;
            return EXIT_FAILURE;
        }
    } else if (priorities == "all") {
        for (priority_type i = 0; i < max_prio; i++) {
            try {
                // get_converter() returns a factory function, execute that immediately:
                converter::sptr conv_for_prio = get_converter(converter_id, i)(); // Can throw a uhd::key_error
                conv_list[i] = conv_for_prio;
            } catch (...) {
                continue;
            }
        }
    } else { // Assume that priorities contains a list of prios (e.g. 0,2,3)
        std::vector<std::string> prios_in_list;
        boost::split(
                prios_in_list,
                priorities,
                boost::is_any_of(","), // Split at ,
                boost::token_compress_on // Avoid empty results
        );
        for(const std::string &this_prio:  prios_in_list) {
            size_t prio_index = boost::lexical_cast<size_t>(this_prio);
            converter::sptr conv_for_prio = get_converter(converter_id, prio_index)(); // Can throw a uhd::key_error
            conv_list[prio_index] = conv_for_prio;
        }
    }
    std::cout << "Found " << conv_list.size() << " converter(s)." << std::endl;

    /// Create input and output buffers ///////////////////////////////////////
    // First, convert the types to plain types (e.g. sc16_item32_le -> sc16)
    const std::string in_type  = format_to_type(in_format);
    const std::string out_type = format_to_type(out_format);
    const size_t in_size  = get_bytes_per_item(in_type);
    const size_t out_size = get_bytes_per_item(out_type);
    // Create the buffers and fill them with random data & zeros, respectively
    std::vector< std::vector<char> > input_buffers(n_inputs, std::vector<char>(in_size * n_samples, 0));
    std::vector< std::vector<char> > output_buffers(n_outputs, std::vector<char>(out_size * n_samples, 0));
    init_buffers(input_buffers, in_type, in_size, buf_seed_mode);
    // Create ref vectors for the converter:
    std::vector<const void *>  input_buf_refs(n_inputs);
    std::vector<void *> output_buf_refs(n_outputs);
    for (size_t i = 0; i < n_inputs; i++) {
        input_buf_refs[i] = reinterpret_cast<const void *>(&input_buffers[i][0]);
    }
    for (size_t i = 0; i < n_outputs; i++) {
        output_buf_refs[i] = reinterpret_cast<void *>(&output_buffers[i][0]);
    }

    /// Final configurations to the converter:
    std::cout << "Configuring converters:" << std::endl;
    for(priority_type prio_i:  conv_list.keys()) {
        std::cout << "* [" << prio_i << "]: ";
        configure_conv(conv_list[prio_i], in_type, out_type);
    }

    /// Run the benchmark for every converter ////////////////////////////////
    std::cout << "{{{" << std::endl;
    if (not debug_mode) {
        std::cout << "prio,duration_ms,avg_duration_ms,n_samples,iterations" << std::endl;
        for(priority_type prio_i:  conv_list.keys()) {
            double duration = run_benchmark(
                    conv_list[prio_i],
                    input_buf_refs,
                    output_buf_refs,
                    n_samples,
                    iterations
            );
            std::cout << boost::format("%i,%d,%d,%d,%d")
                % prio_i
                % (duration * 1000)
                % (duration * 1000.0 / iterations)
                % n_samples
                % iterations
                << std::endl;
        }
    }

    /// Or run debug mode, which runs one conversion and prints the results ////
    if (debug_mode) {
        // Only run on the first converter:
        run_benchmark(
            conv_list[conv_list.keys().at(0)],
            input_buf_refs,
            output_buf_refs,
            n_samples,
            iterations
        );
        for (size_t i = 0; i < n_samples; i++) {
            std::cout << item_to_string(input_buf_refs[0], i, in_type, vm.count("hex"))
                      << ";"
                      << item_to_string(reinterpret_cast< const void * >(output_buf_refs[0]), i, out_type, vm.count("hex"))
                      << std::endl;
        }
    }
    std::cout << "}}}" << std::endl;

    return EXIT_SUCCESS;
}
