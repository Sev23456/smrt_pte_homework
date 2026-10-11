#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "include/UniquePtr.h"
#include "include/SharedPtr.h"
#include "Лаба 2 сам_min_upt/include/ArraySequence.h"

namespace {
constexpr std::size_t batch_limit = 4096;
volatile std::uint64_t checksum = 0;

struct Result {
    std::size_t count;
    double raw_ms;
    double unique_ms;
    double shared_ms;
    std::uint64_t raw_bytes;
    std::uint64_t unique_bytes;
    std::uint64_t shared_bytes;
};

class RawBatch {
    int** items;
    std::size_t count;

public:
    explicit RawBatch(std::size_t count) : items(new int*[count]()), count(count) {}
    ~RawBatch() {
        for (std::size_t index = 0; index < count; ++index) {
            delete items[index];
        }
        delete[] items;
    }

    RawBatch(const RawBatch&) = delete;
    RawBatch& operator=(const RawBatch&) = delete;
    int*& operator[](std::size_t index) { return items[index]; }
};

std::size_t current_batch(std::size_t remaining) {
    return std::min(remaining, batch_limit);
}

double benchmark_raw(std::size_t count) {
    const auto started = std::chrono::steady_clock::now();
    for (std::size_t offset = 0; offset < count;) {
        const std::size_t batch_size = current_batch(count - offset);
        {
            RawBatch batch(batch_size);
            for (std::size_t index = 0; index < batch_size; ++index) {
                batch[index] = new int(static_cast<int>((offset + index) % 97));
            }
            for (std::size_t index = 0; index < batch_size; ++index) {
                checksum += static_cast<std::uint64_t>(*batch[index]);
            }
        }
        offset += batch_size;
    }
    const auto finished = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(finished - started).count();
}

double benchmark_unique(std::size_t count) {
    const auto started = std::chrono::steady_clock::now();
    for (std::size_t offset = 0; offset < count;) {
        const std::size_t batch_size = current_batch(count - offset);
        {
            UniquePtr<UniquePtr<int>[]> batch(new UniquePtr<int>[batch_size]);
            for (std::size_t index = 0; index < batch_size; ++index) {
                batch[index].reset(new int(static_cast<int>((offset + index) % 97)));
            }
            for (std::size_t index = 0; index < batch_size; ++index) {
                checksum += static_cast<std::uint64_t>(*batch[index]);
            }
        }
        offset += batch_size;
    }
    const auto finished = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(finished - started).count();
}

double benchmark_shared(std::size_t count) {
    const auto started = std::chrono::steady_clock::now();
    for (std::size_t offset = 0; offset < count;) {
        const std::size_t batch_size = current_batch(count - offset);
        {
            UniquePtr<SharedPtr<int>[]> batch(new SharedPtr<int>[batch_size]);
            for (std::size_t index = 0; index < batch_size; ++index) {
                batch[index].reset(new int(static_cast<int>((offset + index) % 97)));
            }
            for (std::size_t index = 0; index < batch_size; ++index) {
                checksum += static_cast<std::uint64_t>(*batch[index]);
            }
        }
        offset += batch_size;
    }
    const auto finished = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(finished - started).count();
}

Result measure(std::size_t count) {
    Result result{};
    result.count = count;
    checksum = 0;
    result.raw_ms = benchmark_raw(count);
    const std::uint64_t raw_checksum = checksum;
    checksum = 0;
    result.unique_ms = benchmark_unique(count);
    const std::uint64_t unique_checksum = checksum;
    checksum = 0;
    result.shared_ms = benchmark_shared(count);
    const std::uint64_t shared_checksum = checksum;
    if (raw_checksum != unique_checksum || raw_checksum != shared_checksum) {
        throw std::logic_error("Benchmark implementations produced different results");
    }

    // These are payload and handle sizes, not allocator or OS memory usage.
    result.raw_bytes = count * (sizeof(int*) + sizeof(int));
    result.unique_bytes = count * (sizeof(UniquePtr<int>) + sizeof(int));
    result.shared_bytes = count * (sizeof(SharedPtr<int>) + sizeof(int) + sizeof(std::size_t));
    return result;
}

void write_csv(const Result* results, std::size_t size) {
    std::ofstream file("benchmark_results.csv");
    if (!file) {
        throw std::runtime_error("Cannot create benchmark_results.csv");
    }
    file << "count,raw_ms,unique_ms,shared_ms,raw_estimated_bytes,"
            "unique_estimated_bytes,shared_estimated_bytes\n";
    file << std::fixed << std::setprecision(3);
    for (std::size_t index = 0; index < size; ++index) {
        const Result& row = results[index];
        file << row.count << ',' << row.raw_ms << ',' << row.unique_ms << ','
             << row.shared_ms << ',' << row.raw_bytes << ',' << row.unique_bytes
             << ',' << row.shared_bytes << '\n';
    }
}

double chart_value(const Result& row, int series, bool memory) {
    if (memory) {
        const double mib = 1024.0 * 1024.0;
        if (series == 0) return row.raw_bytes / mib;
        if (series == 1) return row.unique_bytes / mib;
        return row.shared_bytes / mib;
    }
    if (series == 0) return row.raw_ms;
    if (series == 1) return row.unique_ms;
    return row.shared_ms;
}

void draw_chart(std::ofstream& file, const Result* results, std::size_t size,
                int top, bool memory) {
    const int left = 85;
    const int width = 760;
    const int height = 190;
    const char* colors[] = {"#2563eb", "#16a34a", "#ea580c"};
    double maximum = 0.0;
    for (std::size_t index = 0; index < size; ++index) {
        for (int series = 0; series < 3; ++series) {
            maximum = std::max(maximum, chart_value(results[index], series, memory));
        }
    }
    if (maximum == 0.0) maximum = 1.0;

    file << "<text x='85' y='" << top - 20 << "' font-size='18' font-weight='bold'>";
    if (memory) {
        file << "Estimated memory for N live objects (MiB)";
    } else {
        file << "Elapsed time (ms)";
    }
    file << "</text>\n";
    file << "<path d='M" << left << ' ' << top << " V" << top + height
         << " H" << left + width << "' fill='none' stroke='#444'/>\n";
    file << std::fixed << std::setprecision(2);
    file << "<text x='8' y='" << top + 5 << "' font-size='12'>" << maximum << "</text>\n";
    file << "<text x='38' y='" << top + height + 4 << "' font-size='12'>0</text>\n";

    for (int series = 0; series < 3; ++series) {
        file << "<polyline fill='none' stroke='" << colors[series]
             << "' stroke-width='3' points='";
        for (std::size_t index = 0; index < size; ++index) {
            double x = left + width / 2.0;
            if (size > 1) {
                x = left + static_cast<double>(index) * width / (size - 1);
            }
            const double y = top + height *
                (1.0 - chart_value(results[index], series, memory) / maximum);
            file << x << ',' << y << ' ';
        }
        file << "'/>\n";
    }

    for (std::size_t index = 0; index < size; ++index) {
        double x = left + width / 2.0;
        if (size > 1) {
            x = left + static_cast<double>(index) * width / (size - 1);
        }
        file << "<text x='" << x << "' y='" << top + height + 19
             << "' font-size='11' text-anchor='middle'>" << results[index].count
             << "</text>\n";
    }
}

void write_svg(const Result* results, std::size_t size) {
    std::ofstream file("benchmark_results.svg");
    if (!file) {
        throw std::runtime_error("Cannot create benchmark_results.svg");
    }
    file << "<svg xmlns='http://www.w3.org/2000/svg' width='900' height='620' "
            "viewBox='0 0 900 620'>\n"
            "<rect width='900' height='620' fill='white'/>\n"
            "<text x='85' y='28' font-size='21' font-weight='bold'>Smart pointer comparison</text>\n"
            "<text x='85' y='51' font-size='13'>"
            "Raw (blue)   UniquePtr (green)   SharedPtr (orange)</text>\n";
    draw_chart(file, results, size, 95, false);
    draw_chart(file, results, size, 375, true);
    file << "<text x='85' y='603' font-size='12'>"
            "Time uses batches of up to 4096. Memory excludes allocator overhead.</text>\n"
            "</svg>\n";
}

std::size_t parse_max_count(int argc, char** argv) {
    if (argc == 1) return 1000000;
    if (argc != 2) throw std::invalid_argument("Usage: main [max_count]");

    std::size_t parsed = 0;
    const std::string text(argv[1]);
    const unsigned long long value = std::stoull(text, &parsed);
    if (parsed != text.size() || value == 0 || value > 100000000) {
        throw std::invalid_argument("max_count must be between 1 and 100000000");
    }
    return static_cast<std::size_t>(value);
}

void demonstrate() {
    UniquePtr<int> one(new int(7));
    UniquePtr<int[]> array(new int[3]{1, 2, 3});
    SharedPtr<int> shared(new int(42));
    MutableArraySequence<SharedPtr<int>> sequence;
    sequence.append(shared);
    shared.reset();

    std::cout << "UniquePtr<int>: " << *one << '\n'
              << "UniquePtr<int[]>: " << array[0] << ", " << array[1]
              << ", " << array[2] << '\n'
              << "ArraySequence<SharedPtr<int>> after external reset: "
              << *sequence.get(0) << "\n\n";
}
} // namespace

int main(int argc, char** argv) {
    try {
        demonstrate();
        const std::size_t maximum = parse_max_count(argc, argv);
        const std::size_t standard_counts[] = {
            10, 1000, 100000, 1000000, 10000000, 100000000
        };
        Result results[7]{};
        std::size_t result_count = 0;
        for (std::size_t count : standard_counts) {
            if (count <= maximum) {
                results[result_count++] = measure(count);
            }
        }
        if (result_count == 0 || results[result_count - 1].count != maximum) {
            results[result_count++] = measure(maximum);
        }

        std::cout << std::fixed << std::setprecision(2);
        std::cout << std::setw(12) << "N" << std::setw(13) << "raw ms"
                  << std::setw(13) << "unique ms" << std::setw(13) << "shared ms"
                  << std::setw(17) << "raw bytes" << std::setw(17) << "unique bytes"
                  << std::setw(17) << "shared bytes" << '\n';
        for (std::size_t index = 0; index < result_count; ++index) {
            const Result& row = results[index];
            std::cout << std::setw(12) << row.count << std::setw(13) << row.raw_ms
                      << std::setw(13) << row.unique_ms << std::setw(13) << row.shared_ms
                      << std::setw(17) << row.raw_bytes << std::setw(17) << row.unique_bytes
                      << std::setw(17) << row.shared_bytes << '\n';
        }
        std::cout << "Memory columns estimate N simultaneously live objects; "
                     "allocator overhead is excluded.\n";

        write_csv(results, result_count);
        write_svg(results, result_count);
        std::cout << "Saved benchmark_results.csv and benchmark_results.svg\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
