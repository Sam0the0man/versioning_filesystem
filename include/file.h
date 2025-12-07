#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <ctime>
#include <string>

struct FILE_INFO {
    std::string name;
    std::time_t timestamp;
    size_t size;
    unsigned long identifier; // Used for identifying location in filesystem
    unsigned int blockCount;
};

struct FILE_ENTRY {
    unsigned long identifier;
    unsigned int block;
};

#endif