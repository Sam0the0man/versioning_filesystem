
#ifndef DISK_LIBRARY_H
#define DISK_LIBRARY_H

#include <ctime>
#include <filesystem>
#include <string>
#include "file.h"

namespace fs = std::filesystem;

const int BLOCK_SIZE = 4096;
const std::string DEFAULT_FS = "default";
const int MAX_FILES = 255;



class FileSystemDisk {
    public:
        FileSystemDisk(std::string filesystemName=DEFAULT_FS);
        void SaveFile(FILE_INFO file);
        void DeleteFile(FILE_INFO file);

    private:
        std::string filesystemName;
        fs::path diskPath;
        std::bitset<MAX_FILES> fileBitmap;

        void CreateDisk(); 
        void LoadDisk();
        void AddFile(FILE_INFO file);
        void RemoveFile(FILE_INFO file);
        void UpdateBitmap(unsigned int block, bool allocated);
        FILE_ENTRY GetBlock(unsigned int block);
};
#endif