//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Converter benchmark tool
// Tests all available converters across different buffer sizes and priorities
//
// Run:
//   ./examples/convert_benchmark
//   ./examples/convert_benchmark --help
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <chrono>
#include <complex>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <vector>

namespace po = boost::program_options;
using namespace uhd;

typedef std::complex<int16_t> sc16_t;
typedef std::complex<int8_t> sc8_t;
typedef std::complex<float> fc32_t;
typedef std::complex<double> fc64_t;

struct BenchmarkResult
{
    convert::id_type id;
    int priority;
    std::string priority_name;
    size_t buffer_size;
    double ns_per_sample;
    double samples_per_sec;
    double throughput_gbps;
    size_t bytes_per_sample;
};

std::string get_priority_name(int prio)
{
    switch (prio) {
        case -1:
            return "Auto (Best)";
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
        case 5:
            return "AVX512";
        default:
            return "Unknown(" + std::to_string(prio) + ")";
    }
}

size_t get_format_bytes(const std::string& format)
{
    if (format == "fc64")
        return 16; // 2 x 8-byte double
    if (format == "fc32")
        return 8; // 2 x 4-byte float
    if (format == "sc16")
        return 4; // 2 x 2-byte int16
    if (format == "sc8")
        return 2; // 2 x 1-byte int8
    if (format == "f64")
        return 8;
    if (format == "f32")
        return 4;
    if (format == "s16")
        return 2;
    if (format == "s8" || format == "u8")
        return 1;
    // Wire formats
    if (format.find("item32") != std::string::npos)
        return 4;
    if (format.find("chdr") != std::string::npos)
        return 4;
    if (format.find("sc12") != std::string::npos)
        return 3; // approximate
    return 4; // default
}

BenchmarkResult run_benchmark(convert::id_type id,
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

    convert::converter::sptr conv;
    try {
        conv = convert::get_converter(id, prio)();
    } catch (uhd::key_error&) {
        // Converter not available at this priority
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

    // Align pointers
    void* input_ptr = reinterpret_cast<void*>(
        (reinterpret_cast<size_t>(input_storage.data()) + alignment - 1)
        & ~(alignment - 1));
    void* output_ptr = reinterpret_cast<void*>(
        (reinterpret_cast<size_t>(output_storage.data()) + alignment - 1)
        & ~(alignment - 1));

    // Initialize input with random-ish data
    uint32_t* input_u32 = static_cast<uint32_t*>(input_ptr);
    for (size_t i = 0; i < buffer_size * 4; i++) {
        input_u32[i] = static_cast<uint32_t>(i * 12345 + 67890);
    }

    const void* input_ptrs[1] = {input_ptr};
    void* output_ptrs[1]      = {output_ptr};
    uhd::ref_vector<const void*> in_t(input_ptrs, 1);
    uhd::ref_vector<void*> out_t(output_ptrs, 1);

    conv->conv(in_t, out_t, buffer_size);
    conv->conv(in_t, out_t, buffer_size);

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t iter = 0; iter < iterations; iter++) {
        conv->conv(in_t, out_t, buffer_size);
    }
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double total_samples = static_cast<double>(buffer_size) * iterations;

    result.ns_per_sample   = elapsed_ns / total_samples;
    result.samples_per_sec = total_samples / (elapsed_ns * 1e-9);
    result.bytes_per_sample =
        get_format_bytes(id.input_format) + get_format_bytes(id.output_format);
    result.throughput_gbps =
        (result.samples_per_sec * result.bytes_per_sample * 8) / 1e9; // Gbps

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

    // Find baseline (Generic, prio 0) - speedup is always relative to Generic
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
            // No Generic baseline available
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

//-----------------------------------------------------------------------------
// Define converter test configurations
//-----------------------------------------------------------------------------
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

int main(int argc, char* argv[])
{
    po::options_description desc("Converter Benchmark Options");

    // clang-format off
    desc.add_options()
        ("help,h", "Show this help message")
        ("buffer-sizes", po::value<std::string>()->default_value("64,256,1024,4096,16384,65536,262144"),
            "List of buffer sizes to test")
        ("iterations", po::value<size_t>()->default_value(100),
            "Number of iterations per benchmark")
        ("priorities", po::value<std::string>()->default_value("0,1,2,3,4,5"),
            "List of priorities to test (-1 = auto/best available)")
        ("filter", po::value<std::string>()->default_value(""),
            "Filter conversions (e.g., 'fc32' to only test fc32-related)")
        ("compare", "Show comparison tables with speedup")
        ("throughput", "Show throughput in Gbps")
        ("csv", "Output results in CSV format")
        ("quick", "Quick mode: fewer buffer sizes and iterations")
    ;
    // clang-format on

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << desc << "\n";
        return 1;
    }

    if (vm.count("help")) {
        std::cout << "UHD Converter Benchmark Tool\n\n";
        std::cout << "This tool benchmarks all available format converters across\n";
        std::cout << "different buffer sizes and implementation priorities.\n\n";
        std::cout << "Priority levels:\n";
        std::cout << "  -1: Auto (uses best available - highest priority registered)\n";
        std::cout << "   0: Generic C++ implementation\n";
        std::cout << "   1: Unrolled / specialized\n";
        std::cout << "   2: NEON (ARM)\n";
        std::cout << "   3: SSE2 / SSSE3 (x86)\n";
        std::cout << "   4: AVX2 (x86)\n";
        std::cout << "   5: AVX512 (x86)\n\n";
        std::cout << desc << "\n";
        return 0;
    }

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

    std::vector<int> priorities;
    {
        std::stringstream ss(vm["priorities"].as<std::string>());
        std::string item;
        while (std::getline(ss, item, ',')) {
            priorities.push_back(std::stoi(item));
        }
    }

    size_t iterations = vm["iterations"].as<size_t>();
    if (vm.count("quick")) {
        iterations = 20;
    }

    std::string filter     = vm["filter"].as<std::string>();
    bool show_comparison   = vm.count("compare") > 0;
    bool show_throughput   = vm.count("throughput") > 0;
    bool csv_output        = vm.count("csv") > 0;
    auto converter_configs = get_converter_configs();

    std::cout << "========================================\n";
    std::cout << "UHD Converter Benchmark\n";
    std::cout << "========================================\n";
    std::cout << "Buffer sizes: ";
    for (auto s : buffer_sizes)
        std::cout << s << " ";
    std::cout << "\n";
    std::cout << "Iterations: " << iterations << "\n";
    std::cout << "Priorities: ";
    for (auto p : priorities)
        std::cout << p << " ";
    std::cout << "\n";
    if (!filter.empty()) {
        std::cout << "Filter: " << filter << "\n";
    }
    std::cout << "\n";

    std::vector<BenchmarkResult> all_results;

    // CSV header
    if (csv_output) {
        std::cout
            << "input_format,output_format,buffer_size,priority,priority_name,ns_per_"
               "sample,msamples_per_sec,gbps\n";
    }

    // Run benchmarks
    for (const auto& config : converter_configs) {
        // Apply filter
        if (!filter.empty()) {
            if (config.input_format.find(filter) == std::string::npos
                && config.output_format.find(filter) == std::string::npos) {
                continue;
            }
        }

        convert::id_type id;
        id.input_format  = config.input_format;
        id.num_inputs    = 1;
        id.output_format = config.output_format;
        id.num_outputs   = 1;

        std::string conv_name = config.input_format + " -> " + config.output_format;

        if (!csv_output) {
            std::cout << "Benchmarking: " << conv_name << " (" << config.description
                      << ")...\n";
        }

        std::vector<BenchmarkResult> conv_results;

        for (size_t buf_size : buffer_sizes) {
            for (int prio : priorities) {
                auto result =
                    run_benchmark(id, prio, buf_size, iterations, config.scale_factor);

                if (csv_output && result.ns_per_sample > 0) {
                    std::cout << config.input_format << "," << config.output_format << ","
                              << buf_size << "," << prio << "," << result.priority_name
                              << "," << std::fixed << std::setprecision(3)
                              << result.ns_per_sample << "," << std::setprecision(2)
                              << result.samples_per_sec / 1e6 << ","
                              << result.throughput_gbps << "\n";
                }

                all_results.push_back(result);
                conv_results.push_back(result);
            }
        }

        if (show_comparison && !csv_output) {
            // Group by buffer size
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

    // Print summary tables
    if (!csv_output) {
        print_results_table(all_results, "All Benchmark Results", show_throughput);

        // Print fastest converter for each type at largest buffer size
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "Summary: Fastest Converter for Each Type\n";
        std::cout << std::string(80, '=') << "\n";
        std::cout << std::setw(35) << "Conversion" << std::setw(15) << "Best Priority"
                  << std::setw(15) << "ns/sample" << std::setw(15) << "Speedup vs Gen"
                  << "\n";
        std::cout << std::string(80, '-') << "\n";

        size_t largest_buf = *std::max_element(buffer_sizes.begin(), buffer_sizes.end());

        // Group by conversion
        std::map<std::string, std::vector<BenchmarkResult>> by_conv;
        for (const auto& r : all_results) {
            if (r.buffer_size == largest_buf && r.ns_per_sample > 0) {
                std::string key = r.id.input_format + " -> " + r.id.output_format;
                by_conv[key].push_back(r);
            }
        }

        for (auto& kv : by_conv) {
            // Find fastest and generic
            auto& results = kv.second;
            auto fastest  = std::min_element(results.begin(),
                results.end(),
                [](const BenchmarkResult& a, const BenchmarkResult& b) {
                    return a.ns_per_sample < b.ns_per_sample;
                });

            // Always use Generic (priority 0) as baseline
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
                std::cout << std::setw(14) << std::setprecision(2) << speedup << "x";
            } else {
                std::cout << std::setw(15) << "N/A";
            }
            std::cout << "\n";
        }
    }

    return 0;
}
