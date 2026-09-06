// SPDX-License-Identifier: GPL-3.0-only

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ConsoleTable.h"

namespace {

struct Labels {
    std::string memoryTitle;
    std::string swapTitle;
    std::string totalsTitle;
    std::string total;
    std::string used;
    std::string free;
    std::string buffCache;
    std::string available;
    std::string usePercent;
};

Labels getLabels() {
    return {"Memory", "Swap", "Totals", "TOTAL", "USED", "FREE", "BUF/CACHE", "AVAILABLE", "USE%"};
}

struct ColorSettings {
    bool enabled = true;
};

struct Options {
    bool showHelp = false;
    bool json = false;
    bool freeCompatible = false;
    bool line = false;
    bool total = false;
    bool wide = false;
    bool committed = false;
    bool lohi = false;
    bool si = false;
    int count = 1;
    double seconds = 0.0;
    std::string unit = "auto";
    ColorSettings colorSettings;
};

struct MemInfo {
    long long memTotal = 0;
    long long memFree = 0;
    long long memAvailable = 0;
    long long buffers = 0;
    long long cached = 0;
    long long shmem = 0;
    long long sReclaimable = 0;
    long long swapTotal = 0;
    long long swapFree = 0;
    long long highTotal = 0;
    long long highFree = 0;
    long long lowTotal = 0;
    long long lowFree = 0;
    long long commitLimit = 0;
    long long committedAs = 0;
};

struct Metrics {
    long long memUsed = 0;
    long long buffCache = 0;
    long long swapUsed = 0;
    long long total = 0;
    long long totalUsed = 0;
    long long totalFree = 0;
    long long highUsed = 0;
    long long lowUsed = 0;
};

std::string trim(const std::string &input) {
    const std::string whitespace = " \t\n\r";
    const std::string::size_type first = input.find_first_not_of(whitespace);
    if (first == std::string::npos)
        return "";
    const std::string::size_type last = input.find_last_not_of(whitespace);
    return input.substr(first, last - first + 1);
}

long long toLongLong(const std::string &text, long long fallback = 0) {
    try {
        return std::stoll(text);
    } catch (...) {
        return fallback;
    }
}

std::string meminfoPath() {
    const char *overridePath = std::getenv("SUPERFREE_MEMINFO");
    if (overridePath && *overridePath)
        return overridePath;
    return "/proc/meminfo";
}

void setMeminfoValue(MemInfo &info, const std::string &key, long long value) {
    if (key == "MemTotal") info.memTotal = value;
    else if (key == "MemFree") info.memFree = value;
    else if (key == "MemAvailable") info.memAvailable = value;
    else if (key == "Buffers") info.buffers = value;
    else if (key == "Cached") info.cached = value;
    else if (key == "Shmem") info.shmem = value;
    else if (key == "SReclaimable") info.sReclaimable = value;
    else if (key == "SwapTotal") info.swapTotal = value;
    else if (key == "SwapFree") info.swapFree = value;
    else if (key == "HighTotal") info.highTotal = value;
    else if (key == "HighFree") info.highFree = value;
    else if (key == "LowTotal") info.lowTotal = value;
    else if (key == "LowFree") info.lowFree = value;
    else if (key == "CommitLimit") info.commitLimit = value;
    else if (key == "Committed_AS") info.committedAs = value;
}

bool readMeminfo(MemInfo &info, std::string &error) {
    std::ifstream infile(meminfoPath());
    if (!infile) {
        error = "cannot open meminfo: " + meminfoPath();
        return false;
    }

    for (std::string line; std::getline(infile, line);) {
        const std::string::size_type colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        const std::string key = line.substr(0, colon);
        std::stringstream values(trim(line.substr(colon + 1)));
        long long value = 0;
        values >> value;
        setMeminfoValue(info, key, value);
    }

    if (info.memTotal <= 0) {
        error = "meminfo does not contain MemTotal";
        return false;
    }

    return true;
}

Metrics calculateMetrics(const MemInfo &info) {
    Metrics metrics;
    metrics.buffCache = info.buffers + info.cached + info.sReclaimable;
    metrics.memUsed = info.memAvailable > 0
        ? info.memTotal - info.memAvailable
        : info.memTotal - info.memFree - metrics.buffCache;
    metrics.memUsed = std::max(0LL, metrics.memUsed);
    metrics.swapUsed = std::max(0LL, info.swapTotal - info.swapFree);
    metrics.total = info.memTotal + info.swapTotal;
    metrics.totalUsed = metrics.memUsed + metrics.swapUsed;
    metrics.totalFree = info.memFree + info.swapFree;
    metrics.highUsed = std::max(0LL, info.highTotal - info.highFree);
    metrics.lowUsed = std::max(0LL, info.lowTotal - info.lowFree);
    return metrics;
}

void printHelp() {
    std::cout << "Usage: sfree [OPTIONS]\n\n"
              << "Display Linux memory and swap usage as a colorful table, free-compatible text, or JSON.\n\n"
              << "Output options:\n"
              << "      --json              Print machine-readable JSON.\n"
              << "      --unit UNIT         Force output unit: auto, B, kB, KiB, MB, MiB, GB, GiB, TB, TiB.\n"
              << "      --color WHEN        Control colored table output: always, auto, never (default: always).\n"
              << "      --human             Use automatic human-readable units (default table/JSON mode).\n\n"
              << "free-compatible options:\n"
              << "  -b, --bytes             Show bytes.\n"
              << "      --kilo, --mega, --giga, --tera, --peta\n"
              << "                           Show decimal KB/MB/GB/TB/PB units.\n"
              << "  -k, --kibi              Show kibibytes.\n"
              << "  -m, --mebi              Show mebibytes.\n"
              << "  -g, --gibi              Show gibibytes.\n"
              << "      --tebi, --pebi      Show tebibytes/pebibytes.\n"
              << "  -h                      Show human-readable free-style output.\n"
              << "      --si                Use powers of 1000 for human-readable output.\n"
              << "  -l, --lohi              Show high/low memory rows when available.\n"
              << "  -L, --line              Show output on a single line.\n"
              << "  -t, --total             Show RAM + swap total row.\n"
              << "  -v, --committed         Show commit limit/committed memory row.\n"
              << "  -s N, --seconds N       Repeat every N seconds.\n"
              << "  -c N, --count N         Repeat N times and exit.\n"
              << "      --help              Show this help and exit.\n"
              << "  -V, --version           Show version and exit.\n\n"
              << "Examples:\n"
              << "  sfree\n"
              << "  sfree --json --unit MiB\n"
              << "  sfree --bytes --total --wide\n";
}

std::string optionNeedsValue(const std::string &option) {
    return option + " requires a value";
}

bool parseArgs(int argc, char *argv[], Options &options, std::string &error) {
    const std::map<std::string, std::string> unitOptions{
        {"-b", "B"}, {"--bytes", "B"},
        {"--kilo", "kB"}, {"--mega", "MB"}, {"--giga", "GB"}, {"--tera", "TB"}, {"--peta", "PB"},
        {"-k", "KiB"}, {"--kibi", "KiB"},
        {"-m", "MiB"}, {"--mebi", "MiB"},
        {"-g", "GiB"}, {"--gibi", "GiB"},
        {"--tebi", "TiB"}, {"--pebi", "PiB"},
    };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const auto unitIt = unitOptions.find(arg);
        if (unitIt != unitOptions.end()) {
            options.unit = unitIt->second;
            options.freeCompatible = true;
        } else if (arg == "--help") {
            options.showHelp = true;
        } else if (arg == "-V" || arg == "--version") {
            std::cout << "sfree 1.0.1\n";
            std::exit(0);
        } else if (arg == "--json") {
            options.json = true;
        } else if (arg == "--human") {
            options.unit = "auto";
        } else if (arg == "-h") {
            options.unit = "auto";
            options.freeCompatible = true;
        } else if (arg == "--si") {
            options.si = true;
            options.freeCompatible = true;
        } else if (arg == "--unit") {
            if (i + 1 >= argc) {
                error = optionNeedsValue(arg);
                return false;
            }
            options.unit = argv[++i];
        } else if (arg == "--color") {
            if (i + 1 >= argc) {
                error = optionNeedsValue(arg);
                return false;
            }
            const std::string mode = argv[++i];
            if (mode == "never") {
                options.colorSettings.enabled = false;
            } else if (mode == "always") {
                options.colorSettings.enabled = true;
            } else if (mode == "auto") {
                options.colorSettings.enabled = isatty(STDOUT_FILENO) && std::getenv("NO_COLOR") == NULL;
            } else {
                error = "unsupported color mode: " + mode;
                return false;
            }
        } else if (arg == "-t" || arg == "--total") {
            options.total = true;
            options.freeCompatible = true;
        } else if (arg == "-w" || arg == "--wide") {
            options.wide = true;
            options.freeCompatible = true;
        } else if (arg == "-L" || arg == "--line") {
            options.line = true;
            options.freeCompatible = true;
        } else if (arg == "-v" || arg == "--committed") {
            options.committed = true;
            options.freeCompatible = true;
        } else if (arg == "-l" || arg == "--lohi") {
            options.lohi = true;
            options.freeCompatible = true;
        } else if (arg == "-s" || arg == "--seconds") {
            if (i + 1 >= argc) {
                error = optionNeedsValue(arg);
                return false;
            }
            options.seconds = std::stod(argv[++i]);
            options.freeCompatible = true;
        } else if (arg == "-c" || arg == "--count") {
            if (i + 1 >= argc) {
                error = optionNeedsValue(arg);
                return false;
            }
            options.count = std::max(1, static_cast<int>(toLongLong(argv[++i], 1)));
            options.freeCompatible = true;
        } else {
            error = "unknown option: " + arg;
            return false;
        }
    }

    const std::vector<std::string> validUnits{"auto", "B", "kB", "KiB", "MB", "MiB", "GB", "GiB", "TB", "TiB", "PB", "PiB"};
    if (std::find(validUnits.begin(), validUnits.end(), options.unit) == validUnits.end()) {
        error = "unsupported unit: " + options.unit;
        return false;
    }

    if (std::getenv("NO_COLOR") != NULL && options.colorSettings.enabled)
        options.colorSettings.enabled = false;

    return true;
}

struct DisplayValue {
    DisplayValue(double value, const std::string &unit) : value(value), unit(unit) {}

    double value;
    std::string unit;
};

DisplayValue convertValue(long long kib, const Options &options) {
    const std::string unit = options.unit;
    if (unit == "B") return {static_cast<double>(kib) * 1024.0, "B"};
    if (unit == "kB") return {static_cast<double>(kib), "kB"};
    if (unit == "KiB") return {static_cast<double>(kib), "KiB"};
    if (unit == "MB") return {static_cast<double>(kib) * 1024.0 / 1000.0 / 1000.0, "MB"};
    if (unit == "MiB") return {static_cast<double>(kib) / 1024.0, "MiB"};
    if (unit == "GB") return {static_cast<double>(kib) * 1024.0 / 1000.0 / 1000.0 / 1000.0, "GB"};
    if (unit == "GiB") return {static_cast<double>(kib) / 1024.0 / 1024.0, "GiB"};
    if (unit == "TB") return {static_cast<double>(kib) * 1024.0 / 1000.0 / 1000.0 / 1000.0 / 1000.0, "TB"};
    if (unit == "TiB") return {static_cast<double>(kib) / 1024.0 / 1024.0 / 1024.0, "TiB"};
    if (unit == "PB") return {static_cast<double>(kib) * 1024.0 / 1000.0 / 1000.0 / 1000.0 / 1000.0 / 1000.0, "PB"};
    if (unit == "PiB") return {static_cast<double>(kib) / 1024.0 / 1024.0 / 1024.0 / 1024.0, "PiB"};

    const double base = options.si ? 1000.0 : 1024.0;
    double value = static_cast<double>(kib);
    std::vector<std::string> units = options.si
        ? std::vector<std::string>{"KiB", "MB", "GB", "TB", "PB"}
        : std::vector<std::string>{"KiB", "MiB", "GiB", "TiB", "PiB"};
    size_t index = 0;
    while (value >= base && index < units.size() - 1) {
        value /= base;
        ++index;
    }
    return {value, units[index]};
}

std::string numberString(double value, int precision = 1) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::string freeNumber(long long kib, const Options &options) {
    const DisplayValue display = convertValue(kib, options);
    if (options.unit == "auto") {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(display.value < 10.0 ? 1 : 0) << display.value << display.unit;
        return stream.str();
    }

    std::stringstream stream;
    stream << std::fixed << std::setprecision(0) << display.value;
    return stream.str();
}

std::string tableValue(long long kib, const Options &options) {
    const DisplayValue display = convertValue(kib, options);
    std::stringstream stream;
    stream << std::fixed << std::setprecision(options.unit == "auto" || display.value < 10.0 ? 1 : 0)
           << display.value << " " << display.unit;
    return stream.str();
}

double percentage(long long used, long long total) {
    if (total <= 0)
        return 0.0;
    return (static_cast<double>(used) * 100.0) / static_cast<double>(total);
}

int terminalColumns() {
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
        return size.ws_col;

    const char *columns = std::getenv("COLUMNS");
    if (columns && *columns) {
        const long long parsed = toLongLong(columns, 0);
        if (parsed > 0)
            return static_cast<int>(parsed);
    }

    return 100;
}

std::string usageColor(double pct, const ColorSettings &colorSettings) {
    if (!colorSettings.enabled)
        return "";
    if (pct < 60.0) return "\x1b[38;5;148m";
    if (pct < 90.0) return "\x1b[38;5;226m";
    return "\x1b[38;5;197m";
}

std::string resetColor(const ColorSettings &colorSettings) {
    return colorSettings.enabled ? "\x1b[0m" : "";
}

std::string usageBar(long long used, long long total, const ColorSettings &colorSettings, int barWidth = 22, bool compact = false) {
    const double pct = percentage(used, total);
    if (compact && barWidth <= 0)
        return usageColor(pct, colorSettings) + numberString(pct, 0) + "%" + resetColor(colorSettings);

    const int width = std::max(4, barWidth);
    const int hashes = static_cast<int>(std::round((pct * width) / 100.0));
    std::string result = usageColor(pct, colorSettings) + "[";
    for (int i = 0; i < width; ++i)
        result += i < hashes ? "#" : ".";
    result += "] " + numberString(pct, compact ? 0 : 1) + (compact ? "%" : " %") + resetColor(colorSettings);
    return result;
}

void printJsonNumber(std::ostream &out, long long kib, const Options &options) {
    out << numberString(convertValue(kib, options).value, 1);
}

void printJsonRow(std::ostream &out, const std::string &name, long long total, long long used, long long free, const Options &options) {
    out << "  \"" << name << "\": {"
        << "\"total\": "; printJsonNumber(out, total, options);
    out << ", \"used\": "; printJsonNumber(out, used, options);
    out << ", \"free\": "; printJsonNumber(out, free, options);
    out << ", \"usage_percent\": " << numberString(percentage(used, total), 1) << "}";
}

void printJson(const MemInfo &info, const Metrics &metrics, const Options &options) {
    const std::string unit = options.unit == "auto" ? (options.si ? "human-si" : "human-binary") : convertValue(0, options).unit;
    std::cout << "{\n"
              << "  \"unit\": \"" << unit << "\",\n"
              << "  \"si\": " << (options.si ? "true" : "false") << ",\n"
              << "  \"memory\": {"
              << "\"total\": "; printJsonNumber(std::cout, info.memTotal, options);
    std::cout << ", \"used\": "; printJsonNumber(std::cout, metrics.memUsed, options);
    std::cout << ", \"free\": "; printJsonNumber(std::cout, info.memFree, options);
    std::cout << ", \"shared\": "; printJsonNumber(std::cout, info.shmem, options);
    std::cout << ", \"buffers\": "; printJsonNumber(std::cout, info.buffers, options);
    std::cout << ", \"cache\": "; printJsonNumber(std::cout, info.cached + info.sReclaimable, options);
    std::cout << ", \"buff_cache\": "; printJsonNumber(std::cout, metrics.buffCache, options);
    std::cout << ", \"available\": "; printJsonNumber(std::cout, info.memAvailable, options);
    std::cout << ", \"usage_percent\": " << numberString(percentage(metrics.memUsed, info.memTotal), 1) << "},\n";
    printJsonRow(std::cout, "swap", info.swapTotal, metrics.swapUsed, info.swapFree, options);
    std::cout << ",\n";
    printJsonRow(std::cout, "total", metrics.total, metrics.totalUsed, metrics.totalFree, options);
    if (options.committed) {
        std::cout << ",\n  \"committed\": {\"limit\": "; printJsonNumber(std::cout, info.commitLimit, options);
        std::cout << ", \"used\": "; printJsonNumber(std::cout, info.committedAs, options);
        std::cout << ", \"free\": "; printJsonNumber(std::cout, info.commitLimit - info.committedAs, options);
        std::cout << "}";
    }
    std::cout << "\n}\n";
}

void printFreeRows(const std::vector<std::vector<std::string>> &rows) {
    std::vector<size_t> widths;
    for (const auto &row : rows) {
        if (widths.size() < row.size())
            widths.resize(row.size(), 0);
        for (size_t i = 0; i < row.size(); ++i)
            widths[i] = std::max(widths[i], row[i].size());
    }

    for (const auto &row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i == 0) {
                std::cout << std::left << std::setw(static_cast<int>(widths[i] + 1)) << row[i];
            } else {
                std::cout << std::right << std::setw(static_cast<int>(widths[i] + 1)) << row[i];
            }
        }
        std::cout << "\n";
    }
}

void printLine(const MemInfo &info, const Metrics &metrics, const Options &options) {
    std::cout << "MemTotal " << freeNumber(info.memTotal, options)
              << " MemUsed " << freeNumber(metrics.memUsed, options)
              << " MemFree " << freeNumber(info.memFree, options)
              << " MemShared " << freeNumber(info.shmem, options)
              << " MemBuffCache " << freeNumber(metrics.buffCache, options)
              << " MemAvailable " << freeNumber(info.memAvailable, options)
              << " SwapTotal " << freeNumber(info.swapTotal, options)
              << " SwapUsed " << freeNumber(metrics.swapUsed, options)
              << " SwapFree " << freeNumber(info.swapFree, options);
    if (options.total) {
        std::cout << " Total " << freeNumber(metrics.total, options)
                  << " TotalUsed " << freeNumber(metrics.totalUsed, options)
                  << " TotalFree " << freeNumber(metrics.totalFree, options);
    }
    if (options.committed) {
        std::cout << " CommitLimit " << freeNumber(info.commitLimit, options)
                  << " Committed_AS " << freeNumber(info.committedAs, options)
                  << " CommitFree " << freeNumber(info.commitLimit - info.committedAs, options);
    }
    std::cout << "\n";
}

void printFreeCompatible(const MemInfo &info, const Metrics &metrics, const Options &options) {
    if (options.line) {
        printLine(info, metrics, options);
        return;
    }

    std::vector<std::vector<std::string>> rows;
    if (options.wide) {
        rows.push_back({"", "total", "used", "free", "shared", "buffers", "cache", "available"});
        rows.push_back({"Mem:", freeNumber(info.memTotal, options), freeNumber(metrics.memUsed, options), freeNumber(info.memFree, options), freeNumber(info.shmem, options), freeNumber(info.buffers, options), freeNumber(info.cached + info.sReclaimable, options), freeNumber(info.memAvailable, options)});
    } else {
        rows.push_back({"", "total", "used", "free", "shared", "buff/cache", "available"});
        rows.push_back({"Mem:", freeNumber(info.memTotal, options), freeNumber(metrics.memUsed, options), freeNumber(info.memFree, options), freeNumber(info.shmem, options), freeNumber(metrics.buffCache, options), freeNumber(info.memAvailable, options)});
    }
    rows.push_back({"Swap:", freeNumber(info.swapTotal, options), freeNumber(metrics.swapUsed, options), freeNumber(info.swapFree, options)});
    if (options.lohi && info.highTotal > 0 && info.lowTotal > 0) {
        rows.push_back({"High:", freeNumber(info.highTotal, options), freeNumber(metrics.highUsed, options), freeNumber(info.highFree, options)});
        rows.push_back({"Low:", freeNumber(info.lowTotal, options), freeNumber(metrics.lowUsed, options), freeNumber(info.lowFree, options)});
    }
    if (options.total)
        rows.push_back({"Total:", freeNumber(metrics.total, options), freeNumber(metrics.totalUsed, options), freeNumber(metrics.totalFree, options)});
    if (options.committed)
        rows.push_back({"Comm:", freeNumber(info.commitLimit, options), freeNumber(info.committedAs, options), freeNumber(info.commitLimit - info.committedAs, options)});

    printFreeRows(rows);
}

void printSuperTable(const MemInfo &info, const Metrics &metrics, const Options &options) {
    const Labels labels = getLabels();
    const int columns = terminalColumns();
    const bool tiny = columns < 52;
    const bool compact = !tiny && columns < 72;
    const bool medium = !compact && columns < 112;
    const int barWidth = compact ? 0 : (medium ? 10 : 22);
    const std::string memoryColor = usageColor(percentage(metrics.memUsed, info.memTotal), options.colorSettings);
    const std::string swapColor = usageColor(percentage(metrics.swapUsed, info.swapTotal), options.colorSettings);
    const std::string totalsColor = usageColor(percentage(metrics.totalUsed, metrics.total), options.colorSettings);
    const std::string reset = resetColor(options.colorSettings);

    if (tiny) {
        ConsoleTable tableMemory{"TYPE", labels.used, labels.usePercent};
        tableMemory.setPadding(1);
        tableMemory.setStyle(4);

        tableMemory += {memoryColor + labels.memoryTitle + reset,
                memoryColor + tableValue(metrics.memUsed, options) + reset,
                usageBar(metrics.memUsed, info.memTotal, options.colorSettings, 0, true)};
        tableMemory += {swapColor + labels.swapTitle + reset,
                swapColor + tableValue(metrics.swapUsed, options) + reset,
                usageBar(metrics.swapUsed, info.swapTotal, options.colorSettings, 0, true)};
        tableMemory += {totalsColor + labels.totalsTitle + reset,
                totalsColor + tableValue(metrics.totalUsed, options) + reset,
                usageBar(metrics.totalUsed, metrics.total, options.colorSettings, 0, true)};

        tableMemory.setTittle("Mem");
        std::cout << tableMemory;
        return;
    }

    if (compact || medium) {
        ConsoleTable tableMemory{"TYPE", labels.total, labels.used, labels.free, labels.usePercent};
        tableMemory.setPadding(1);
        tableMemory.setStyle(4);

        tableMemory += {memoryColor + labels.memoryTitle + reset,
                memoryColor + tableValue(info.memTotal, options) + reset,
                memoryColor + tableValue(metrics.memUsed, options) + reset,
                tableValue(info.memFree, options),
                usageBar(metrics.memUsed, info.memTotal, options.colorSettings, barWidth, compact)};

        tableMemory += {swapColor + labels.swapTitle + reset,
                swapColor + tableValue(info.swapTotal, options) + reset,
                swapColor + tableValue(metrics.swapUsed, options) + reset,
                tableValue(info.swapFree, options),
                usageBar(metrics.swapUsed, info.swapTotal, options.colorSettings, barWidth, compact)};

        tableMemory += {totalsColor + labels.totalsTitle + reset,
                totalsColor + tableValue(metrics.total, options) + reset,
                totalsColor + tableValue(metrics.totalUsed, options) + reset,
                tableValue(metrics.totalFree, options),
                usageBar(metrics.totalUsed, metrics.total, options.colorSettings, barWidth, compact)};

        tableMemory.setTittle(compact ? "Memory" : "Memory Usage");
        std::cout << tableMemory;
        return;
    }

    ConsoleTable tableMemory{"TYPE", labels.total, labels.used, labels.free, labels.buffCache, labels.available, labels.usePercent};
    tableMemory.setPadding(1);
    tableMemory.setStyle(4);

    tableMemory += {memoryColor + labels.memoryTitle + reset,
            memoryColor + tableValue(info.memTotal, options) + reset,
            memoryColor + tableValue(metrics.memUsed, options) + reset,
            tableValue(info.memFree, options),
            tableValue(metrics.buffCache, options),
            tableValue(info.memAvailable, options),
            usageBar(metrics.memUsed, info.memTotal, options.colorSettings, barWidth)};

    tableMemory += {swapColor + labels.swapTitle + reset,
            swapColor + tableValue(info.swapTotal, options) + reset,
            swapColor + tableValue(metrics.swapUsed, options) + reset,
            tableValue(info.swapFree, options),
            "-",
            "-",
            usageBar(metrics.swapUsed, info.swapTotal, options.colorSettings, barWidth)};

    tableMemory += {totalsColor + labels.totalsTitle + reset,
            totalsColor + tableValue(metrics.total, options) + reset,
            totalsColor + tableValue(metrics.totalUsed, options) + reset,
            tableValue(metrics.totalFree, options),
            "-",
            "-",
            usageBar(metrics.totalUsed, metrics.total, options.colorSettings, barWidth)};

    tableMemory.setTittle("Memory Usage");
    std::cout << tableMemory;
}

void printOnce(const Options &options) {
    MemInfo info;
    std::string error;
    if (!readMeminfo(info, error)) {
        std::cerr << "sfree: " << error << "\n";
        std::exit(1);
    }

    const Metrics metrics = calculateMetrics(info);
    if (options.json)
        printJson(info, metrics, options);
    else if (options.freeCompatible)
        printFreeCompatible(info, metrics, options);
    else
        printSuperTable(info, metrics, options);
}

} // namespace

int main(int argc, char *argv[]) {
    Options options;
    std::string error;
    if (!parseArgs(argc, argv, options, error)) {
        std::cerr << "sfree: " << error << "\nTry 'sfree --help' for more information.\n";
        return 2;
    }

    if (options.showHelp) {
        printHelp();
        return 0;
    }

    for (int i = 0; i < options.count; ++i) {
        if (i > 0 && options.seconds > 0.0)
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(options.seconds * 1000.0)));
        printOnce(options);
    }

    return 0;
}
