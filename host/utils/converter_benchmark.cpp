//
// Copyright 2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/safe_main.hpp>
#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <chrono>
#include <complex>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

namespace po = boost::program_options;
using namespace uhd::convert;

enum buf_init_t { RANDOM, INC };

// Convert `sc16_item32_le' -> `sc16'
// Finds the first _ in format and returns the string
// until then. Returns the entire string if no _ is found.
std::string format_to_type(const std::string& format)
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
    converter::sptr conv, const std::string& in_type, const std::string& out_type)
{
    if (in_type == "sc16") {
        if (out_type == "fc32") {
            std::cout << "Setting scalar to 1./32767." << std::endl;
            conv->set_scalar(1. / 32767.);
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
void init_random_vector_complex_float(std::vector<char>& buf_ptr, const size_t n_items)
{
    std::complex<T>* const buf = reinterpret_cast<std::complex<T>* const>(&buf_ptr[0]);
    for (size_t i = 0; i < n_items; i++) {
        buf[i] = std::complex<T>(
            T(std::rand() / (RAND_MAX / 2.0) - 1), T(std::rand() / (RAND_MAX / 2.0) - 1));
    }
}

template <typename T>
void init_random_vector_complex_int(std::vector<char>& buf_ptr, const size_t n_items)
{
    std::complex<T>* const buf = reinterpret_cast<std::complex<T>* const>(&buf_ptr[0]);
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
void init_random_vector_complex_sc12(std::vector<char>& buf_ptr, const size_t n_items)
{
    item32_sc12_3x* const buf = reinterpret_cast<item32_sc12_3x* const>(&buf_ptr[0]);
    if (n_items % 4)
        throw std::invalid_argument("");

    for (size_t i = 0; i < n_items / 4; i++) {
        int16_t iq[8];
        for (auto& k : iq)
            k = rand() & 0xfff;
        buf[i].line0 = iq[0] << 20 | iq[1] << 8 | iq[2] >> 4;
        buf[i].line1 = iq[2] << 28 | iq[3] << 16 | iq[4] << 4 | iq[5] >> 8;
        buf[i].line2 = iq[5] << 24 | iq[6] << 12 | iq[7] << 0;
    }
}

template <typename T>
void init_random_vector_real_int(std::vector<char>& buf_ptr, size_t n_items)
{
    T* const buf = reinterpret_cast<T* const>(&buf_ptr[0]);
    for (size_t i = 0; i < n_items; i++) {
        buf[i] = T(std::rand());
    }
}

// Fill a buffer with increasing numbers
template <typename T>
void init_inc_vector(std::vector<char>& buf_ptr, size_t n_items)
{
    T* const buf = reinterpret_cast<T* const>(&buf_ptr[0]);
    for (size_t i = 0; i < n_items; i++) {
        buf[i] = T(i);
    }
}

void init_buffers(std::vector<std::vector<char>>& buf,
    const std::string& type,
    size_t bytes_per_item,
    buf_init_t buf_seed_mode)
{
    if (buf.empty()) {
        return;
    }
    size_t n_items = buf[0].size() / bytes_per_item;

    /// Fill with incrementing integers
    if (buf_seed_mode == INC) {
        for (size_t i = 0; i < buf.size(); i++) {
            if (type == "sc8") {
                init_inc_vector<std::complex<int8_t>>(buf[i], n_items);
            } else if (type == "sc16") {
                init_inc_vector<std::complex<int16_t>>(buf[i], n_items);
            } else if (type == "sc32") {
                init_inc_vector<std::complex<int32_t>>(buf[i], n_items);
            } else if (type == "fc32") {
                init_inc_vector<std::complex<float>>(buf[i], n_items);
            } else if (type == "fc64") {
                init_inc_vector<std::complex<double>>(buf[i], n_items);
            } else if (type == "s8") {
                init_inc_vector<int8_t>(buf[i], n_items);
            } else if (type == "s16") {
                init_inc_vector<int16_t>(buf[i], n_items);
            } else if (type == "item32") {
                init_inc_vector<uint32_t>(buf[i], n_items);
                init_random_vector_real_int<uint32_t>(buf[i], n_items);
            } else {
                throw uhd::runtime_error(
                    str(boost::format("Cannot handle data type: %s") % type));
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
            throw uhd::runtime_error(
                str(boost::format("Cannot handle data type: %s") % type));
        }
    }
}

// Returns time elapsed
double run_benchmark(converter::sptr conv,
    const std::vector<const void*>& input_buf_refs,
    const std::vector<void*>& output_buf_refs,
    size_t n_items,
    size_t iterations)
{
    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        conv->conv(input_buf_refs, output_buf_refs, n_items);
    }
    auto stop                              = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = stop - start;
    return duration.count();
}

template <typename T>
std::string void_ptr_to_hexstring(const void* v_ptr, size_t index)
{
    const T* ptr = reinterpret_cast<const T*>(v_ptr);
    return str(boost::format("%X") % ptr[index]);
}

std::string item_to_hexstring(const void* v_ptr, size_t index, const std::string& type)
{
    if (type == "fc32") {
        return void_ptr_to_hexstring<uint64_t>(v_ptr, index);
    } else if (type == "sc16" || type == "item32") {
        return void_ptr_to_hexstring<uint32_t>(v_ptr, index);
    } else if (type == "sc8" || type == "s16") {
        return void_ptr_to_hexstring<uint16_t>(v_ptr, index);
    } else if (type == "u8") {
        return void_ptr_to_hexstring<uint8_t>(v_ptr, index);
    } else {
        return str(boost::format("<unhandled data type: %s>") % type);
    }
}

std::string item_to_string(
    const void* v_ptr, size_t index, const std::string& type, const bool print_hex)
{
    if (print_hex) {
        return item_to_hexstring(v_ptr, index, type);
    }

    if (type == "sc16") {
        const std::complex<int16_t>* ptr =
            reinterpret_cast<const std::complex<int16_t>*>(v_ptr);
        return boost::lexical_cast<std::string>(ptr[index]);
    } else if (type == "sc8") {
        const std::complex<int8_t>* ptr =
            reinterpret_cast<const std::complex<int8_t>*>(v_ptr);
        return boost::lexical_cast<std::string>(ptr[index]);
    } else if (type == "fc32") {
        const std::complex<float>* ptr =
            reinterpret_cast<const std::complex<float>*>(v_ptr);
        return boost::lexical_cast<std::string>(ptr[index]);
    } else if (type == "item32") {
        const uint32_t* ptr = reinterpret_cast<const uint32_t*>(v_ptr);
        return boost::lexical_cast<std::string>(ptr[index]);
    } else if (type == "s16") {
        const int16_t* ptr = reinterpret_cast<const int16_t*>(v_ptr);
        return boost::lexical_cast<std::string>(ptr[index]);
    } else {
        return str(boost::format("<unhandled data type: %s>") % type);
    }
}

// For batch of benchmarks
std::string get_priority_name(int prio)
{
    switch (prio) {
        case 0:
            return "Generic";
        case 1:
            return "Unrolled";
        case 2:
            return "NEON";
        case 3:
            return "SSE2/SSSE3";
        case 4:
            return "AVX2";
        default:
            return "Unknown(" + std::to_string(prio) + ")";
    }
}

struct ConverterConfig
{
    std::string input_format;
    std::string output_format;
    double scale_factor;
    std::string description;
};

std::vector<ConverterConfig> get_converter_configs()
{
    return {
        // sc16 <-> fc32
        {"sc16_item32_le", "fc32", 1.0 / 32768.0, "Wire LE to float"},
        {"fc32", "sc16_item32_le", 32768.0, "Float to wire LE"},
        {"sc16_item32_be", "fc32", 1.0 / 32768.0, "Wire BE to float"},
        {"fc32", "sc16_item32_be", 32768.0, "Float to wire BE"},
        {"sc16_chdr", "fc32", 1.0 / 32768.0, "CHDR to float"},
        {"fc32", "sc16_chdr", 32768.0, "Float to CHDR"},

        // sc16 <-> fc64
        {"sc16_item32_le", "fc64", 1.0 / 32768.0, "Wire LE to double"},
        {"fc64", "sc16_item32_le", 32768.0, "Double to wire LE"},
        {"sc16_chdr", "fc64", 1.0 / 32768.0, "CHDR to double"},
        {"fc64", "sc16_chdr", 32768.0, "Double to CHDR"},

        // sc8 <-> fc32
        {"sc8_item32_le", "fc32", 1.0 / 128.0, "8-bit wire to float"},
        {"fc32", "sc8_item32_le", 128.0, "Float to 8-bit wire"},

        // sc16 passthrough
        {"sc16_item32_le", "sc16", 1.0, "Wire LE to native sc16"},
        {"sc16", "sc16_item32_le", 1.0, "Native sc16 to wire LE"},
        {"sc16_item32_be", "sc16", 1.0, "Wire BE to native sc16"},
        {"sc16", "sc16_item32_be", 1.0, "Native sc16 to wire BE"},

        // sc12
        {"sc12_item32_le", "sc16", 1.0, "12-bit to sc16"},
        {"sc16", "sc12_item32_le", 1.0, "sc16 to 12-bit"},
        {"sc12_item32_le", "fc32", 1.0 / 2048.0, "12-bit to float"},
        {"fc32", "sc12_item32_le", 2048.0, "Float to 12-bit"},
    };
}

struct BenchmarkResult
{
    id_type id;
    int priority;
    std::string priority_name;
    size_t buffer_size;
    double ns_per_sample;
    double samples_per_sec;
    double throughput_gbps;
    size_t bytes_per_sample;
};

BenchmarkResult run_batch_benchmark(id_type id,
    int prio,
    size_t buffer_size,
    size_t iterations,
    double scale_factor = 1.0)
{
    BenchmarkResult result;
    result.id            = id;
    result.priority      = prio;
    result.priority_name = get_priority_name(prio);
    result.buffer_size   = buffer_size;

    converter::sptr conv;
    try {
        conv = get_converter(id, prio)();
    } catch (uhd::key_error&) {
        result.ns_per_sample   = -1;
        result.samples_per_sec = 0;
        result.throughput_gbps = 0;
        return result;
    }

    conv->set_scalar(scale_factor);

    const size_t alignment  = 64;
    const size_t alloc_size = buffer_size * 16 + alignment;
    std::vector<uint8_t> input_storage(alloc_size);
    std::vector<uint8_t> output_storage(alloc_size);

    void* input_ptr = reinterpret_cast<void*>(
        (reinterpret_cast<size_t>(input_storage.data()) + alignment - 1)
        & ~(alignment - 1));
    void* output_ptr = reinterpret_cast<void*>(
        (reinterpret_cast<size_t>(output_storage.data()) + alignment - 1)
        & ~(alignment - 1));

    // Initialize input with deterministic data
    uint32_t* input_u32 = static_cast<uint32_t*>(input_ptr);
    for (size_t i = 0; i < buffer_size * 4; i++) {
        input_u32[i] = static_cast<uint32_t>(i * 12345 + 67890);
    }

    std::vector<const void*> input_buf_refs(1, input_ptr);
    std::vector<void*> output_buf_refs(1, output_ptr);

    // Warm-up runs
    conv->conv(input_buf_refs, output_buf_refs, buffer_size);
    conv->conv(input_buf_refs, output_buf_refs, buffer_size);

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t iter = 0; iter < iterations; iter++) {
        conv->conv(input_buf_refs, output_buf_refs, buffer_size);
    }
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double total_samples = static_cast<double>(buffer_size) * iterations;

    result.ns_per_sample    = elapsed_ns / total_samples;
    result.samples_per_sec  = total_samples / (elapsed_ns * 1e-9);
    const std::string in_type  = format_to_type(id.input_format);
    const std::string out_type = format_to_type(id.output_format);
    result.bytes_per_sample =
        get_bytes_per_item(in_type) + get_bytes_per_item(out_type);
    result.throughput_gbps =
        (result.samples_per_sec * result.bytes_per_sample * 8) / 1e9;

    return result;
}

void print_results_table(const std::vector<BenchmarkResult>& results,
    const std::string& title,
    bool show_throughput = false)
{
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(80, '=') << "\n";

    // Group by conversion type
    std::map<std::string, std::vector<BenchmarkResult>> grouped;
    for (const auto& r : results) {
        std::string key = r.id.input_format + " -> " + r.id.output_format;
        grouped[key].push_back(r);
    }

    for (const auto& kv : grouped) {
        std::cout << "\n" << kv.first << ":\n";
        std::cout << std::string(70, '-') << "\n";

        if (show_throughput) {
            std::cout << std::setw(12) << "Buffer Size" << std::setw(15) << "Priority"
                      << std::setw(15) << "ns/sample" << std::setw(15) << "MSamples/s"
                      << std::setw(12) << "Gbps" << "\n";
        } else {
            std::cout << std::setw(12) << "Buffer Size" << std::setw(15) << "Priority"
                      << std::setw(15) << "ns/sample" << std::setw(15) << "MSamples/s"
                      << "\n";
        }
        std::cout << std::string(70, '-') << "\n";

        for (const auto& r : kv.second) {
            if (r.ns_per_sample < 0) {
                std::cout << std::setw(12) << r.buffer_size << std::setw(15)
                          << r.priority_name << std::setw(15) << "N/A" << std::setw(15)
                          << "N/A" << "\n";
            } else {
                std::cout << std::setw(12) << r.buffer_size << std::setw(15)
                          << r.priority_name << std::setw(15) << std::fixed
                          << std::setprecision(3) << r.ns_per_sample << std::setw(15)
                          << std::setprecision(2) << r.samples_per_sec / 1e6;
                if (show_throughput) {
                    std::cout << std::setw(12) << std::setprecision(2)
                              << r.throughput_gbps;
                }
                std::cout << "\n";
            }
        }
    }
}

void print_comparison_table(const std::vector<BenchmarkResult>& results,
    const std::string& conversion,
    size_t buffer_size)
{
    std::cout << "\n" << conversion << " @ " << buffer_size << " samples:\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << std::setw(15) << "Priority" << std::setw(15) << "ns/sample"
              << std::setw(15) << "MSamples/s" << std::setw(15) << "Speedup" << "\n";
    std::cout << std::string(60, '-') << "\n";

    // Find baseline (Generic, prio 0)
    double baseline_ns = 0;
    for (const auto& r : results) {
        if (r.priority == 0 && r.ns_per_sample > 0) {
            baseline_ns = r.ns_per_sample;
            break;
        }
    }

    for (const auto& r : results) {
        if (r.ns_per_sample < 0) {
            std::cout << std::setw(15) << r.priority_name << std::setw(15) << "N/A"
                      << std::setw(15) << "N/A" << std::setw(15) << "N/A" << "\n";
        } else if (baseline_ns <= 0) {
            std::cout << std::setw(15) << r.priority_name << std::setw(15) << std::fixed
                      << std::setprecision(3) << r.ns_per_sample << std::setw(15)
                      << std::setprecision(2) << r.samples_per_sec / 1e6 << std::setw(15)
                      << "N/A" << "\n";
        } else {
            double speedup = baseline_ns / r.ns_per_sample;
            std::cout << std::setw(15) << r.priority_name << std::setw(15) << std::fixed
                      << std::setprecision(3) << r.ns_per_sample << std::setw(15)
                      << std::setprecision(2) << r.samples_per_sec / 1e6 << std::setw(14)
                      << std::setprecision(2) << speedup << "x" << "\n";
        }
    }
}

int UHD_SAFE_MAIN(int argc, char* argv[])
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
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("in",  po::value<std::string>(&in_format), "Input format (e.g. 'sc16')")
        ("out", po::value<std::string>(&out_format), "Output format (e.g. 'sc16')")
        ("samples",  po::value<size_t>(&n_samples)->default_value(1000000), "Number of samples per iteration")
        ("iterations",  po::value<size_t>(&iterations)->default_value(10000), "Number of iterations per benchmark")
        ("priorities", po::value<std::string>(&priorities)->default_value("default"), "Converter priorities. Can be 'default', 'all', or a comma-separated list of priorities.")
        ("max-prio", po::value<priority_type>(&max_prio)->default_value(5), "Largest available priority (advanced feature)")
        ("n-inputs",   po::value<size_t>(&n_inputs)->default_value(1),  "Number of input vectors")
        ("n-outputs",  po::value<size_t>(&n_outputs)->default_value(1), "Number of output vectors")
        ("debug-converter", "Skip benchmark and print conversion results. Implies iterations==1 and will only run on a single converter.")
        ("seed-mode", po::value<std::string>(&seed_mode)->default_value("random"), "How to initialize the data: random, incremental")
        ("hex", "When using debug mode, dump memory in hex")
        ("batch", "Run batch benchmark across all predefined converter configurations")
        ("buffer-sizes", po::value<std::string>()->default_value("64,256,1024,4096,16384,65536,262144"), "Buffer sizes for batch mode (comma-separated)")
        ("compare", "Show comparison tables with speedup (batch mode)")
        ("throughput", "Show throughput in Gbps (batch mode)")
        ("csv", "Output batch results in CSV format")
        ("quick", "Quick batch mode: fewer buffer sizes and iterations")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("UHD Converter Benchmark Tool %s") % desc << std::endl
                  << std::endl;
        std::cout
            << "  Use this to benchmark or debug converters." << std::endl
            << "  When using as a benchmark tool, it will output the execution time\n"
               "  for every conversion run in CSV format to stdout. Every line between\n"
               "  the output delimiters {{{ }}} is of the format: <PRIO>,<TIME IN "
               "MILLISECONDS>\n"
               "  When using for converter debugging, every line is formatted as\n"
               "  <INPUT_VALUE>,<OUTPUT_VALUE>\n"
            << std::endl;
        return EXIT_FAILURE;
    }

    // Batch mode: run predefined converter configs across multiple buffer sizes
    if (vm.count("batch")) {
        std::vector<size_t> buffer_sizes;
        std::string sizes_str = vm["buffer-sizes"].as<std::string>();
        if (vm.count("quick")) {
            sizes_str = "256,4096,65536";
        }
        {
            std::stringstream ss(sizes_str);
            std::string item;
            while (std::getline(ss, item, ',')) {
                buffer_sizes.push_back(std::stoull(item));
            }
        }

        std::vector<int> batch_priorities;
        {
            // Use max_prio to determine which priorities to test
            for (int i = 0; i < static_cast<int>(max_prio); i++) {
                batch_priorities.push_back(i);
            }
        }

        size_t batch_iterations = iterations;
        if (vm.count("quick")) {
            batch_iterations = 20;
        }

        std::string filter;
        bool show_comparison = vm.count("compare") > 0;
        bool show_throughput = vm.count("throughput") > 0;
        bool csv_output      = vm.count("csv") > 0;
        auto converter_configs = get_converter_configs();

        std::cout << "========================================\n";
        std::cout << "UHD Converter Batch Benchmark\n";
        std::cout << "========================================\n";
        std::cout << "Buffer sizes: ";
        for (auto s : buffer_sizes)
            std::cout << s << " ";
        std::cout << "\n";
        std::cout << "Iterations: " << batch_iterations << "\n";
        std::cout << "Priorities: ";
        for (auto p : batch_priorities)
            std::cout << p << " ";
        std::cout << "\n\n";

        std::vector<BenchmarkResult> all_results;

        if (csv_output) {
            std::cout << "input_format,output_format,buffer_size,priority,"
                         "priority_name,ns_per_sample,msamples_per_sec,gbps\n";
        }

        // Run benchmarks
        for (const auto& config : converter_configs) {
            id_type bid;
            bid.input_format  = config.input_format;
            bid.num_inputs    = 1;
            bid.output_format = config.output_format;
            bid.num_outputs   = 1;

            std::string conv_name =
                config.input_format + " -> " + config.output_format;

            if (!csv_output) {
                std::cout << "Benchmarking: " << conv_name << " ("
                          << config.description << ")...\n";
            }

            std::vector<BenchmarkResult> conv_results;

            for (size_t buf_size : buffer_sizes) {
                for (int bprio : batch_priorities) {
                    auto result = run_batch_benchmark(
                        bid, bprio, buf_size, batch_iterations, config.scale_factor);

                    if (csv_output && result.ns_per_sample > 0) {
                        std::cout << config.input_format << ","
                                  << config.output_format << "," << buf_size
                                  << "," << bprio << "," << result.priority_name
                                  << "," << std::fixed << std::setprecision(3)
                                  << result.ns_per_sample << ","
                                  << std::setprecision(2)
                                  << result.samples_per_sec / 1e6 << ","
                                  << result.throughput_gbps << "\n";
                    }

                    all_results.push_back(result);
                    conv_results.push_back(result);
                }
            }

            if (show_comparison && !csv_output) {
                for (size_t buf_size : buffer_sizes) {
                    std::vector<BenchmarkResult> size_results;
                    for (const auto& r : conv_results) {
                        if (r.buffer_size == buf_size) {
                            size_results.push_back(r);
                        }
                    }
                    print_comparison_table(size_results, conv_name, buf_size);
                }
            }
        }

        // summary tables
        if (!csv_output) {
            print_results_table(all_results, "All Benchmark Results", show_throughput);

            // Print fastest converter for each type at largest buffer size
            std::cout << "\n" << std::string(80, '=') << "\n";
            std::cout << "Summary: Fastest Converter for Each Type\n";
            std::cout << std::string(80, '=') << "\n";
            std::cout << std::setw(35) << "Conversion" << std::setw(15)
                      << "Best Priority" << std::setw(15) << "ns/sample"
                      << std::setw(15) << "Speedup vs Gen" << "\n";
            std::cout << std::string(80, '-') << "\n";

            size_t largest_buf =
                *std::max_element(buffer_sizes.begin(), buffer_sizes.end());

            // Group by conversion
            std::map<std::string, std::vector<BenchmarkResult>> by_conv;
            for (const auto& r : all_results) {
                if (r.buffer_size == largest_buf && r.ns_per_sample > 0) {
                    std::string key =
                        r.id.input_format + " -> " + r.id.output_format;
                    by_conv[key].push_back(r);
                }
            }

            for (auto& kv : by_conv) {
                auto& results = kv.second;
                auto fastest  = std::min_element(results.begin(),
                    results.end(),
                    [](const BenchmarkResult& a, const BenchmarkResult& b) {
                        return a.ns_per_sample < b.ns_per_sample;
                    });

                double generic_ns = 0;
                for (const auto& r : results) {
                    if (r.priority == 0 && r.ns_per_sample > 0) {
                        generic_ns = r.ns_per_sample;
                        break;
                    }
                }

                std::cout << std::setw(35) << kv.first << std::setw(15)
                          << fastest->priority_name << std::setw(15) << std::fixed
                          << std::setprecision(3) << fastest->ns_per_sample;

                if (generic_ns > 0) {
                    double speedup = generic_ns / fastest->ns_per_sample;
                    std::cout << std::setw(14) << std::setprecision(2) << speedup
                              << "x";
                } else {
                    std::cout << std::setw(15) << "N/A";
                }
                std::cout << "\n";
            }
        }

        return EXIT_SUCCESS;
    }

    // Parse more arguments
    if (seed_mode == "incremental") {
        buf_seed_mode = INC;
    } else if (seed_mode == "random") {
        buf_seed_mode = RANDOM;
    } else {
        std::cout
            << "Invalid argument: --seed-mode must be either 'incremental' or 'random'."
            << std::endl;
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
    std::cout << "Requested converter format: " << converter_id.to_string() << std::endl;
    uhd::dict<priority_type, converter::sptr> conv_list;
    if (priorities == "default" or priorities.empty()) {
        try {
            conv_list[prio] =
                get_converter(converter_id, prio)(); // Can throw a uhd::key_error
        } catch (const uhd::key_error&) {
            std::cout << "No converters found." << std::endl;
            return EXIT_FAILURE;
        }
    } else if (priorities == "all") {
        for (priority_type i = 0; i < max_prio; i++) {
            try {
                // get_converter() returns a factory function, execute that immediately:
                converter::sptr conv_for_prio =
                    get_converter(converter_id, i)(); // Can throw a uhd::key_error
                conv_list[i] = conv_for_prio;
            } catch (...) {
                continue;
            }
        }
    } else { // Assume that priorities contains a list of prios (e.g. 0,2,3)
        std::vector<std::string> prios_in_list;
        boost::split(prios_in_list,
            priorities,
            boost::is_any_of(","), // Split at ,
            boost::token_compress_on // Avoid empty results
        );
        for (const std::string& this_prio : prios_in_list) {
            size_t prio_index = boost::lexical_cast<size_t>(this_prio);
            converter::sptr conv_for_prio =
                get_converter(converter_id, prio_index)(); // Can throw a uhd::key_error
            conv_list[prio_index] = conv_for_prio;
        }
    }
    std::cout << "Found " << conv_list.size() << " converter(s)." << std::endl;

    /// Create input and output buffers ///////////////////////////////////////
    // First, convert the types to plain types (e.g. sc16_item32_le -> sc16)
    const std::string in_type  = format_to_type(in_format);
    const std::string out_type = format_to_type(out_format);
    const size_t in_size       = get_bytes_per_item(in_type);
    const size_t out_size      = get_bytes_per_item(out_type);
    // Create the buffers and fill them with random data & zeros, respectively
    std::vector<std::vector<char>> input_buffers(
        n_inputs, std::vector<char>(in_size * n_samples, 0));
    std::vector<std::vector<char>> output_buffers(
        n_outputs, std::vector<char>(out_size * n_samples, 0));
    init_buffers(input_buffers, in_type, in_size, buf_seed_mode);
    // Create ref vectors for the converter:
    std::vector<const void*> input_buf_refs(n_inputs);
    std::vector<void*> output_buf_refs(n_outputs);
    for (size_t i = 0; i < n_inputs; i++) {
        input_buf_refs[i] = reinterpret_cast<const void*>(&input_buffers[i][0]);
    }
    for (size_t i = 0; i < n_outputs; i++) {
        output_buf_refs[i] = reinterpret_cast<void*>(&output_buffers[i][0]);
    }

    /// Final configurations to the converter:
    std::cout << "Configuring converters:" << std::endl;
    for (priority_type prio_i : conv_list.keys()) {
        std::cout << "* [" << prio_i << "]: ";
        configure_conv(conv_list[prio_i], in_type, out_type);
    }

    /// Run the benchmark for every converter ////////////////////////////////
    std::cout << "{{{" << std::endl;
    if (not debug_mode) {
        std::cout << "prio,duration_ms,avg_duration_ms,n_samples,iterations" << std::endl;
        for (priority_type prio_i : conv_list.keys()) {
            double duration = run_benchmark(conv_list[prio_i],
                input_buf_refs,
                output_buf_refs,
                n_samples,
                iterations);
            std::cout << boost::format("%i,%d,%d,%d,%d") % prio_i % (duration * 1000)
                             % (duration * 1000.0 / iterations) % n_samples % iterations
                      << std::endl;
        }
    }

    /// Or run debug mode, which runs one conversion and prints the results ////
    if (debug_mode) {
        // Only run on the first converter:
        run_benchmark(conv_list[conv_list.keys().at(0)],
            input_buf_refs,
            output_buf_refs,
            n_samples,
            iterations);
        for (size_t i = 0; i < n_samples; i++) {
            std::cout << item_to_string(input_buf_refs[0], i, in_type, vm.count("hex"))
                      << ";"
                      << item_to_string(reinterpret_cast<const void*>(output_buf_refs[0]),
                             i,
                             out_type,
                             vm.count("hex"))
                      << std::endl;
        }
    }
    std::cout << "}}}" << std::endl;

    return EXIT_SUCCESS;
}
