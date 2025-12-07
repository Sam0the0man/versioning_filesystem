
#ifndef DISK_LIBRARY_H
#define DISK_LIBRARY_H

#include <bitset>
#include <ctime>
#include <filesystem>
#include <set>
#include <string>

namespace fs = std::filesystem;

const int BLOCK_SIZE = 4096;
const std::string DEFAULT_FS = "default";
const int MAX_FILES = 255;

#if defined(_WIN32)
constexpr const char* SYSTEM_NAME = "Windows";
#elif defined(__APPLE__)
constexpr const char* SYSTEM_NAME = "macOS";
#else
constexpr const char* SYSTEM_NAME = "Linux";
#endif

struct FILE_INFO {
    std::string name;
    std::time_t timestamp;
    size_t size;
    unsigned long identifier; // Used for identifying location in filesystem
    unsigned int blockCount;
};

class FileSystemDisk {
    public:
        FileSystemDisk(std::string filesystemName=DEFAULT_FS);
        void createFile(std::string filename); // touch equivalent
        void editFile(std::string filename); // $EDITOR equivalent
        void deleteFile(std::string filename); // rm equivalent
        void listFiles() const; // ls equivalent
        void viewFile(std::string filename, int version=0) const; // cat equivalent
        void viewAllVersions(std::string filename) const;
        void restoreFile(std::string filename, int version); // res command


    private:
        std::string filesystemName;
        fs::path diskPath;
        std::bitset<MAX_FILES + 1> fileBitmap;

        void CreateDisk(); 
        void LoadDisk();

        FILE_INFO CreateFile(std::string filename);
        void EditFile(FILE_INFO &file);
        void RemoveFile(FILE_INFO file);
        void ListFiles() const;
        void ViewFile(std::string filename, int version) const;
        void RestoreFile(std::string filename, int version);
        void UpdateBitmap(unsigned int block, bool allocated);
        void UpdateFileSystem(unsigned int block, FILE_INFO file);
        FILE_INFO GetFileInfo(const std::string& filename) const;
        int GetNextFreeBlock() const;
        int FindBlock(const std::string &filename) const;
        FILE_INFO ReadBlock(unsigned int block) const;
};
#endif
