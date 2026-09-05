// SPDX-License-Identifier: GPL-3.0-only

#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include "ConsoleTable.h"

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

Labels getLabels(){
    return {"Memory", "Swap", "Totals", "TOTAL", "USED", "FREE", "BUF/CACHE", "AVAILABLE", "USE%"};
}

void printHelp(){
    std::cout << "Usage: superfree [OPTIONS]\n\n"
              << "Display Linux memory, swap, and total usage in a colored table.\n\n"
              << "Options:\n"
              << "  -h, --help              Show this help and exit.\n"
              << "  --unit UNIT             Force output unit: auto, kB, KiB, MiB, GiB, TiB.\n"
              << "  --color WHEN            Control colored output: always, auto, never (default: always).\n"
              << "  --human                 Use automatic human-readable units (default).\n\n"
              << "Color thresholds:\n"
              << "  green < 60%, yellow 60-89%, red >= 90%.\n\n"
              << "Examples:\n"
              << "  superfree\n"
              << "  superfree --color never\n"
              << "  superfree --unit kB\n"
              << "  superfree --unit GiB\n";
}

struct ColorSettings {
    bool enabled = true;
};

class MemInfo {
private:
    const std::string pathMeminfo = "/proc/meminfo";

    enum Options {
        Option_Invalid,
        MemTotal,
        MemFree,
        MemAvailable,
        Buffers,
        Cached,
        SwapTotal,
        SwapFree,
    };

    const std::map<std::string, Options> optionStrings {
        {"MemTotal",MemTotal},
        {"MemFree",MemFree},
        {"MemAvailable",MemAvailable},
        {"Buffers",Buffers},
        {"Cached",Cached},
        {"SwapTotal",SwapTotal},
        {"SwapFree",SwapFree},
        {"MemFree",MemFree}
    };

    void trimLeft( std::string& str){
        while(1){
            if(str[0] != ' ') break;
            std::string::size_type pos = str.find_first_not_of(" ");
            str = str.erase(0, pos);
        }
    }


    std::string getColor(const std::string &percentage){
        std::string color = "\x1b[38;5;148m"; //green
        if (std::stof(percentage) < 60)
            color = "\x1b[38;5;148m";
        if ((std::stof(percentage) >= 60) && (std::stof(percentage) < 90))
            color = "\x1b[38;5;226m";
        if (std::stof(percentage) >= 90)
            color = "\x1b[38;5;197m";
        return color;
    }

    std::string genericPrintBar(const std::string &used, const std::string &total) {
        std::string percentageUsed = calculatePercentage(used, total);
        int numberHash = calculateNumberHash(used, total);
        std::string result = colorSettings.enabled ? getColor(percentageUsed) : "";
        for(int i = 0; i<24; i++){
            if(i == 0)
                result = result + "[";
            if ((i > 0) && (i <= numberHash) && (i < 23))
                result = result + "#";
            if ((i > 0) && (i > numberHash) && (i < 23))
                result = result + ".";
            if (i == 23)
                result = result + "]";
        }
        return result + " " + percentageUsed + " %" + (colorSettings.enabled ? "\x1b[0m" : "");
    }


        Options resolveOption(const std::string &input) {
            auto itr = optionStrings.find(input);
            if( itr != optionStrings.end() ) {
                return itr->second;
            }
            return Option_Invalid;
        }

        std::string calculatePercentage(const std::string &used, const std::string &total){
            const long totalValue = std::stol(total);
            if (totalValue == 0) {
                return "0.0";
            }

            float x = (std::stol(used) * 100.0f) / totalValue;
            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) << x;
            return stream.str();
        }

        int calculateNumberHash(const std::string &used, const std::string &total){
            float x = (std::stof(calculatePercentage(used, total)) * 22) / 100;
            return int(round(x));
        }

        std::string cleanData(const std::string &str){
            std::string delimiter = ":";
            std::string data = str.substr(str.find(delimiter) + 1, str.length() - str.find(delimiter));
            trimLeft(data);
            data.resize(data.size() - 3);
            return data;
        }

        void filterLine(const std::string &line){
            std::string delimiter = ":";
            std::string token = line.substr(0,line.find(delimiter));
            switch(resolveOption(token)){
        case MemTotal:
            memTotal = cleanData(line);
            break;
        case MemFree:
            memFree = cleanData(line);
            break;
        case MemAvailable:
            memAvailable = cleanData(line);
            break;
        case Buffers:
            memBuffers = cleanData(line);
            break;
        case Cached:
            memCached = cleanData(line);
            break;
        case SwapTotal:
            swapTotal = cleanData(line);
            break;
        case SwapFree:
            swapFree = cleanData(line);
            break;
        default:
            break;
        }
    }

public:
    std::string memTotal;
    std::string memFree;
    std::string memUsed;
    std::string memAvailable;
    std::string memBuffers;
    std::string memCached;
    std::string buffCached;
    std::string swapTotal;
    std::string swapUsed;
    std::string swapFree;
    std::string Total;
    std::string TotalUsed;
    std::string TotalFree;
    std::string dataType;
    std::string outputUnit = "auto";
    ColorSettings colorSettings;

    enum barOptions {
        bOption_Invalid,
        Memory,
        Swap,
        Totals,
    };

    MemInfo() {
        readFile();
        dataType = "";
        long l_memUsed = std::stol(memTotal) - std::stol(memAvailable);
        memUsed = std::to_string(l_memUsed);
        long l_buffCached = std::stol(memBuffers) + std::stol(memCached);
        buffCached = std::to_string(l_buffCached);
        long l_swapUsed = std::stol(swapTotal) - std::stol(swapFree);
        swapUsed = std::to_string(l_swapUsed);
        long l_Total = std::stol(memTotal) + std::stol(swapTotal);
        Total = std::to_string(l_Total);
        long l_TotalUsed = std::stol(memUsed) + std::stol(swapUsed);
        TotalUsed = std::to_string(l_TotalUsed);
        long l_TotalFree = std::stol(memFree) + std::stol(swapFree);
        TotalFree = std::to_string(l_TotalFree);
    }

    void setHumanReadable(bool enabled){
        outputUnit = enabled ? "auto" : "kB";
    }

    void setOutputUnit(const std::string &unit){
        outputUnit = unit;
    }

    void setColorSettings(const ColorSettings &settings){
        colorSettings = settings;
    }

    std::string getUsageColor(const std::string &used, const std::string &total){
        if (!colorSettings.enabled)
            return "";
        return getColor(calculatePercentage(used, total));
    }

    std::string resetColor() const{
        return colorSettings.enabled ? "\x1b[0m" : "";
    }

    std::string formatValue(const std::string &value) const{
        if (outputUnit == "kB")
            return value + " kB";

        if (outputUnit == "KiB")
            return value + " KiB";

        if (outputUnit == "MiB" || outputUnit == "GiB" || outputUnit == "TiB") {
            double converted = std::stol(value);
            if (outputUnit == "MiB")
                converted = converted / 1024.0;
            if (outputUnit == "GiB")
                converted = converted / 1024.0 / 1024.0;
            if (outputUnit == "TiB")
                converted = converted / 1024.0 / 1024.0 / 1024.0;

            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) << converted << " " << outputUnit;
            return stream.str();
        }

        double bytes = std::stol(value) * 1024.0;
        const std::vector<std::string> units{"B", "KiB", "MiB", "GiB", "TiB"};
        size_t unitIndex = 0;
        while (bytes >= 1024.0 && unitIndex < units.size() - 1) {
            bytes = bytes / 1024.0;
            unitIndex++;
        }

        std::stringstream stream;
        stream << std::fixed << std::setprecision(1) << bytes << " " << units[unitIndex];
        return stream.str();
    }

    void readFile(){
        std::ifstream infile(pathMeminfo);
        for( std::string line; getline(infile, line);)
            filterLine(line);
    }

    std::string printBar(){
        return " ";
    }

    void printData(){
        std::cout << "MemTotal: " << memTotal << std::endl;
        std::cout << "MemUsed: " << memUsed << std::endl;
        std::cout << "MemFree: " << memFree << std::endl;
        std::cout << "MemAvailable: " << memAvailable << std::endl;
        std::cout << "Buffers: " << memBuffers << std::endl;
        std::cout << "Cached: " << memCached << std::endl;
        std::cout << "BuffCached: " << buffCached << std::endl;
        std::cout << "SwapTotal: " << swapTotal << std::endl;
        std::cout << "SwapUsed: " << swapUsed << std::endl;
        std::cout << "SwapFree: " << swapFree << std::endl;
    }

    std::string printBar(int type){
        std::string result = "";
        switch(type){
        case Memory:
            return genericPrintBar(memUsed, memTotal);
            break;
        case Swap:
            return genericPrintBar(swapUsed, swapTotal);
            break;
        case Totals:
            return genericPrintBar(TotalUsed, Total);
        default:
            return result;
        }
        return result;
    }

};



int main(int argc, char *argv[]) {

    bool showHelp = false;
    ColorSettings colorSettings;

    for (int i = 1; i < argc; i++) {
        std::string argument = argv[i];
        if (argument == "--help" || argument == "-h") {
            showHelp = true;
        }
    }

    if (showHelp) {
        printHelp();
        return 0;
    }

    MemInfo info = MemInfo();
    Labels labels = getLabels();

    for (int i = 1; i < argc; i++) {
        std::string argument = argv[i];
        if (argument == "--human") {
            info.setHumanReadable(true);
        } else if (argument == "--unit" && i + 1 < argc) {
            info.setOutputUnit(argv[++i]);
        } else if (argument == "--color" && i + 1 < argc) {
            std::string mode = argv[++i];
            if (mode == "never") {
                colorSettings.enabled = false;
            } else if (mode == "always" || mode == "auto") {
                colorSettings.enabled = true;
            }
        }
    }
    info.setColorSettings(colorSettings);

    std::string memoryColor = info.getUsageColor(info.memUsed, info.memTotal);
    std::string swapColor = info.getUsageColor(info.swapUsed, info.swapTotal);
    std::string totalsColor = info.getUsageColor(info.TotalUsed, info.Total);
    std::string reset = info.resetColor();

    ConsoleTable tableMemory{"TYPE", labels.total, labels.used, labels.free, labels.buffCache, labels.available, labels.usePercent};

    tableMemory.setPadding(1);
    tableMemory.setStyle(4);

    tableMemory += {memoryColor + labels.memoryTitle + reset,
            memoryColor + info.formatValue(info.memTotal) + reset,
            memoryColor + info.formatValue(info.memUsed) + reset,
            info.formatValue(info.memFree),
            info.formatValue(info.buffCached),
            info.formatValue(info.memAvailable),
            info.printBar(1)};

    tableMemory += {swapColor + labels.swapTitle + reset,
            swapColor + info.formatValue(info.swapTotal) + reset,
            swapColor + info.formatValue(info.swapUsed) + reset,
            info.formatValue(info.swapFree),
            "-",
            "-",
            info.printBar(2)};

    tableMemory += {totalsColor + labels.totalsTitle + reset,
            totalsColor + info.formatValue(info.Total) + reset,
            totalsColor + info.formatValue(info.TotalUsed) + reset,
            info.formatValue(info.TotalFree),
            "-",
            "-",
            info.printBar(3)};

    tableMemory.setTittle("Memory Usage");
    std::cout << tableMemory;

    return 0;
}
