
#ifndef DISK_LIBRARY_H
#define DISK_LIBRARY_H

#include <ctime>
#include <filesystem>
#include <string>
#include <set>
#include "file.h"

namespace fs = std::filesystem;

const int BLOCK_SIZE = 4096;
const std::string DEFAULT_FS = "default";
const int MAX_FILES = 255;



class FileSystemDisk {
    public:
        FileSystemDisk(std::string filesystemName=DEFAULT_FS);
        void createFile(std::string filename); // touch equivalent
        void editFile(std::string filename); // $EDITOR equivalent
        // void SaveFile(FILE_INFO file);
        void deleteFile(std::string filename); // rm equivalent
        void listFiles(); // ls equivalent
        void viewFile(std::string filename, int version=-1); // cat equivalent
        void viewAllVersions(std::string filename);
        void restoreFile(std::string filename, int version); // res command


    private:
        std::string filesystemName;
        fs::path diskPath;
        std::bitset<MAX_FILES + 1> fileBitmap;
        std::set<int> setBits;

        void CreateDisk(); 
        void LoadDisk();

        FILE_INFO CreateFile(std::string filename);
        void EditFile(FILE_INFO &file);
        // void AddFile(FILE_INFO file);
        void RemoveFile(FILE_INFO file);

        void UpdateBitmap(unsigned int block, bool allocated);
        FILE_ENTRY GetBlock(unsigned int block);
};
#endif