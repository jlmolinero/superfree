#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include "ConsoleTable.h"

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
        std::string color = "\e[38;5;148m"; //green
        if (std::stof(percentage) < 60)
            color = "\e[38;5;148m";
        if ((std::stof(percentage) >= 60) && (std::stof(percentage) < 90))
            color = "\e[38;5;226m";
        if (std::stof(percentage) >= 90)
            color = "\e[38;5;197m";
        return color;
    }

    std::string genericPrintBar(const std::string &used, const std::string &total) {
        std::string percentageUsed = calculatePercentage(used, total);
        int numberHash = calculateNumberHash(used, total);
        std::string result = getColor(percentageUsed);
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
        return result + " " + percentageUsed + " %\e[0m";
    }


        Options resolveOption(const std::string &input) {
            auto itr = optionStrings.find(input);
            if( itr != optionStrings.end() ) {
                return itr->second;
            }
            return Option_Invalid;
        }

        std::string calculatePercentage(const std::string &used, const std::string &total){
            float x = (std::stol(used) * 100) / std::stol(total);
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

    enum barOptions {
        bOption_Invalid,
        Memory,
        Swap,
        Totals,
    };

    MemInfo() {
        readFile();
        dataType = "kB";
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



int main() {

    MemInfo info = MemInfo();

    ConsoleTable tableMemory{"TOTAL", "USED", "FREE", "BUF/CACHE", "AVAILABLE", "USE%"};

    tableMemory.setPadding(1);
    tableMemory.setStyle(4);

    tableMemory += {"\e[38;5;75m" +info.memTotal + " " + info.dataType  + "\e[0m",
            info.memUsed + " " + info.dataType,
            info.memFree + " " + info.dataType,
            info.buffCached + " " + info.dataType,
            info.memAvailable + " " + info.dataType,
            info.printBar(1)};

    tableMemory.setTittle("Memory");
    std::cout << tableMemory;

    ConsoleTable tableSwap{"TOTAL", "USED", "FREE", "USE%"};

    tableSwap.setPadding(1);
    tableSwap.setStyle(4);

    tableSwap += {"\e[38;5;75m" + info.swapTotal + " " + info.dataType  + "\e[0m",
            info.swapUsed + " " + info.dataType,
            info.swapFree + " " + info.dataType,
            info.printBar(2)};


    tableSwap.setTittle("Swap");
    std::cout << tableSwap;

    ConsoleTable tableTotals{"TOTAL", "USED", "FREE", "USE%"};

    tableTotals.setPadding(1);
    tableTotals.setStyle(4);

    tableTotals += {"\e[38;5;75m" + info.Total + " " + info.dataType + "\e[0m",
            info.TotalUsed + " " + info.dataType,
            info.TotalFree + " " + info.dataType,
            info.printBar(3)};

    tableTotals.setTittle("Totals");
    std::cout << tableTotals;

    return 0;
}
