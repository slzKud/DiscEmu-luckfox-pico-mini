#ifndef __UTIL_H__
#include <string>
typedef struct {
    double totalBytes;
    double freeBytes;
    double usedBytes;
    double totalGB;
    double freeGB;
    double usedGB;
} DiskSpaceInfo;

bool getDiskSpace(const std::string& path,DiskSpaceInfo& disk_info);
std::string getDiskImageFilename(const std::string& path,const std::string& suffix);
std::string roundNumber(double rounded_number,int bit_len);
std::string fillZero(int rounded_number,int bit_len);
#endif