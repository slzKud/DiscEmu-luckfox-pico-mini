#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <regex>
#include <optional>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <sys/statvfs.h>
#include "util.h"

namespace fs = boost::filesystem;
constexpr double BYTES_TO_GB = 1024.0 * 1024.0 * 1024.0;

bool getDiskSpace(const std::string& path,DiskSpaceInfo& disk_info) {
    struct statvfs vfs;
    
    if (statvfs(path.c_str(), &vfs) != 0) {
        return false;
    }

    // 计算磁盘空间（字节）
    disk_info.totalBytes = static_cast<double>(vfs.f_blocks) * vfs.f_frsize;
    disk_info.freeBytes = static_cast<double>(vfs.f_bfree) * vfs.f_frsize;
    disk_info.usedBytes = disk_info.totalBytes - disk_info.freeBytes;

    // 转换为 GB
    disk_info.totalGB = disk_info.totalBytes / BYTES_TO_GB;
    disk_info.freeGB = disk_info.freeBytes / BYTES_TO_GB;
    disk_info.usedGB = disk_info.usedBytes / BYTES_TO_GB;

    return true;
}

// 提取文件名中的数字和参数
std::optional<std::pair<int, std::string>> parse_filename(const std::string& filename) {
    // 正则表达式匹配 Disk_<number>_<suffix>.img
    std::regex pattern(R"(^Disk_(\d+)_(.+)\.img$)");
    std::smatch matches;
    
    if (std::regex_match(filename, matches, pattern)) {
        if (matches.size() == 3) {
            try {
                int num = std::stoi(matches[1].str());
                return std::make_pair(num, matches[2].str());
            } catch (...) {
                // 数字转换失败
            }
        }
    }
    return std::nullopt;
}

std::string getDiskImageFilename(const std::string& path,const std::string& suffix) {
    std::vector<int> numbers;
    std::cout << "getDiskImageFilename:path="<< path << ",suffix=" << suffix << std::endl;
    // 遍历当前目录
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file()) {
            auto result = parse_filename(entry.path().filename().string());
            if (result) {
                numbers.push_back(result->first);
            }
        }
    }
    
    // 按数字排序
    std::sort(numbers.begin(), numbers.end());
    
    // 计算下一个序号
    int next_number = 1;
    if (!numbers.empty()) {
        next_number = numbers.back() + 1;
    }
    
    // 生成新文件名
    std::string new_filename = "Disk_" + fillZero(next_number,2) + "_" + suffix + ".img";
    return new_filename;
}

std::string roundNumber(double rounded_number,int bit_len){
    std::ostringstream oss;
    oss << std::fixed 
    << std::setprecision(bit_len) 
    << rounded_number;
    return oss.str();
}

std::string fillZero(int rounded_number,int bit_len){
    std::ostringstream oss;
    oss << std::setw(bit_len) << std::setfill('0') << static_cast<int>(rounded_number);
    return oss.str();
}

std::string readFileIntoString(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filePath << std::endl;
        return ""; 
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // 移除所有换行符（包括\r和\n）
    content.erase(std::remove_if(content.begin(), content.end(),
        [](char c) { return c == '\r' || c == '\n'; }), content.end());
    
    return content;
}