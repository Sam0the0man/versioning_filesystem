#include <filesystem>
#include <bitset>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include "diskLibrary.h"

FileSystemDisk::FileSystemDisk(std::string filesystemName)
{
    this->filesystemName = filesystemName;

    fs::path homePath;
    homePath = getenv("HOME");
    // Check if filesystem_name exists
    this->diskPath = homePath / filesystemName;
    if (fs::is_directory(diskPath))
    {
        // If it does, load filesystem
        LoadDisk();
    }
    else
    {
        // If not, create filesystem
        CreateDisk();
        LoadDisk();
    }

    std::cout << this->fileBitmap << '\n';
}

void FileSystemDisk::CreateDisk()
{
    // Create directory called filesystemName
    fs::create_directory(diskPath);
    // Create file called filesystemName
    std::ofstream file(diskPath / filesystemName);
    if (file.fail())
    {
        std::cerr << "Filesystem could not be created.";
        exit(1);
    }
    // Write bitmap to allocate data at the beginning of the resulting string
    this->fileBitmap[MAX_FILES - 1] = 1;
    std::string bitmapStr = this->fileBitmap.to_string();
    file.write(bitmapStr.c_str(), bitmapStr.length());

    // Allocate blocks
    char *buffer = new char[BLOCK_SIZE];
    memset(buffer, '0', BLOCK_SIZE);

    // Ensure consistency in block allocation
    for (int i = 0; i < MAX_FILES; ++i)
    {
        // TODO: Correct block allocation for file information
        file.write("\n", 1);
        file.write(buffer, BLOCK_SIZE);
    }

    delete[] buffer;
    file.close();
}
void FileSystemDisk::LoadDisk()
{
    // Load filesystemName
    std::ifstream file(diskPath / filesystemName);
    if (file.fail())
    {
        std::cerr << "Filesystem not found.";
        exit(1);
    }
    // Read bitmap
    std::string bitmapStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Skip empty lines
    size_t pos = bitmapStr.find_first_not_of('\n');
    if (pos != std::string::npos)
    {
        bitmapStr = bitmapStr.substr(pos);
    }
    this->fileBitmap = std::bitset<MAX_FILES>(bitmapStr.substr(0, MAX_FILES));
    file.close();
}
void FileSystemDisk::SaveFile(FILE_INFO file) {
    this->AddFile(file);
}
void FileSystemDisk::DeleteFile(FILE_INFO file) {
    this->RemoveFile(file);
}
void FileSystemDisk::AddFile(FILE_INFO file)
{ // Adds file to directory, updates bitmap, adds to table (hashing)
    std::string filename = file.name;
    std::ofstream fileToAdd;
    fileToAdd.open(this->diskPath / filename);

    if (!fileToAdd.is_open())
    {
        std::cerr << "Failed to add file.";
        exit(1);
    }

    unsigned int block = 200; // Temporary test value, update to hash function later
    
    this->UpdateBitmap(block, true);
    
    fileToAdd.close();
}
void FileSystemDisk::RemoveFile(FILE_INFO file)
{
    std::string filename = file.name;
    fs::path filePath = this->diskPath / file.name;
    unsigned int block = 200; // Temporary test value, update to hash function later

    if (!fs::remove(filePath)) {
        std::cerr << "Failed to remove file.";
        exit(1);
    }

    this->UpdateBitmap(block, false);
}
void FileSystemDisk::UpdateBitmap(unsigned int block, bool allocated)
{
    this->fileBitmap[block - 1] = allocated;

    // Write bitmap to file
    // 1) Open original file (read) and create temporary file (write)
    std::string originalName = fs::path(this->diskPath / filesystemName).string();
    std::string tempName = fs::path(this->diskPath / "temp").string();

    std::ifstream original(originalName);
    std::ofstream temp(tempName);
    if (!original.is_open() || !temp.is_open())
    {
        std::cerr << "Failed to make file.";
        exit(1);
    }

    // 2) Modify the first line (bitmap)
    std::string line;
    if (std::getline(original, line))
    {
        std::string bitmapStr = this->fileBitmap.to_string();
        temp.write(bitmapStr.c_str(), bitmapStr.length());
    }

    // 3) Write everything else to the temporary file
    while (std::getline(original, line))
    {
        temp << '\n'
             << line;
    }
    original.close();
    temp.close();
    // 4) Replace the original with the temporary file
    remove(originalName.c_str());
    rename(tempName.c_str(), originalName.c_str());
    // std::ofstream file(diskPath / filesystemName);
    // if (file.fail())
    // {
    //     std::cerr << "Filesystem could not be created.";
    //     exit(1);
    // }
}
FILE_ENTRY FileSystemDisk::GetBlock(unsigned int block)
{
    // Return the FILE_ENTRY at the associated block number
    return FILE_ENTRY{0, block};
}