#include <filesystem>
#include <bitset>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include "diskLibrary.h"
#include "file.h"

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
    // this->fileBitmap[MAX_FILES - 1] = 1;
    this->fileBitmap[0] = 1;
    std::string bitmapStr = this->fileBitmap.to_string();
    file.write(bitmapStr.c_str(), bitmapStr.length());

    // Allocate blocks
    char *buffer = new char[BLOCK_SIZE];
    memset(buffer, '0', BLOCK_SIZE);

    // Ensure consistency in block allocation
    for (int i = 0; i < MAX_FILES; ++i)
    {
        // TODO: Correct block allocation for file information
        file.write("\n\0", 2);
        // file.write(buffer, BLOCK_SIZE);
    }

    delete[] buffer;
    file.close();
}
void FileSystemDisk::LoadDisk()
{
    // Load filesystemName
    std::ifstream file(this->diskPath / this->filesystemName);
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
    this->fileBitmap = std::bitset<MAX_FILES + 1>(bitmapStr.substr(0, MAX_FILES + 1));
    // Load set of filled bits
    for (int i = 1; i < MAX_FILES + 1; ++i) {
        if (this->fileBitmap.test(i)) {
            this->setBits.insert(i);
        }
    }
    file.close();
}
// void FileSystemDisk::SaveFile(FILE_INFO file) {
//     this->AddFile(file);
// }

void FileSystemDisk::createFile(std::string filename) {
    FILE_INFO info = this->CreateFile(filename);
}
FILE_INFO FileSystemDisk::CreateFile(std::string filename) {
    fs::path fileDir = this->diskPath / filename;

    try {
        if (fs::create_directory(fileDir)) {
            // Directory created successfully
            // Create default blank file
            std::ofstream blankFile(fileDir / "0");
            std::ofstream currentFile(fileDir / "current");
            if (blankFile.fail() || currentFile.fail()) {
                std::cerr << "Could not create file.\n";
                exit(1);
            }
            blankFile.close();
            currentFile.close();
        } else {
            // Directory already exists
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << '\n';
        exit(1);
    }

    // Get file info
    FILE_INFO info;
    struct stat file_stat;
    if (stat(fileDir.string().c_str(), &file_stat) == 0) {
        info.name = filename;
        info.identifier = file_stat.st_ino;
        info.timestamp = file_stat.st_birthtimespec.tv_sec;
        info.size = file_stat.st_size;
    } else {
        std::cerr << "Error accessing file.\n";
        exit(1);
    }
    // Update bitmap
    unsigned int block = (info.identifier % MAX_FILES) + 1;
    // Check for collision
    if (this->setBits.find(block) != this->setBits.end()) {
        // Collision detected
        // Rehash
    } else {
        this->UpdateBitmap(block, true);
        this->setBits.insert(block);
    }
    // Write inode in correct location
    // 1) Open original file (read) and create temporary file (write)
    std::string originalName = fs::path(this->diskPath / filesystemName).string();
    std::string tempName = fs::path(this->diskPath / "temp").string();

    std::ifstream original(originalName);
    std::ofstream temp(tempName);
    if (!original.is_open() || !temp.is_open()) {
        std::cerr << "Could not update filesystem.\n";
        fs::remove_all(fileDir);
        exit(1);
    }

    // 2) Get to correct line
    int currentLine = 0;
    std::string line;
    while (std::getline(original, line) && currentLine < MAX_FILES) {
        ++currentLine;
        if (currentLine == (block + 1)) { // + 1 so it doesn't overwrite bitmap
            // Write inode and name
            temp << info.identifier << '\t' << info.name << '\n';
        } else {
            // 3) Write everything else
            temp << line << '\n';
        }
    }
    temp << line;

    original.close();
    temp.close();
    // 4) Replace the original with the temporary file
    remove(originalName.c_str());
    rename(tempName.c_str(), originalName.c_str());

    return info;
}

void FileSystemDisk::editFile(std::string filename) {
    FILE_INFO info{filename};
    this->EditFile(info);
}
void FileSystemDisk::EditFile(FILE_INFO &file) {
    fs::path fileDir = this->diskPath / file.name;
    if (!std::filesystem::is_directory(fileDir)) {
        // Check if file exists, create if it doesn't
        FILE_INFO createdFile = this->CreateFile(file.name);
        this->EditFile(createdFile);
    } else {
        fs::path cwd;
        int maxVersionNumber = 0;
        try {
            cwd = fs::current_path();
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error getting current directory: " << e.what() << '\n';
            exit(1);
        }
        fs::path editFile = cwd / ("edit_"+file.name);
        pid_t child_pid;
        // Create hard link to current in current working directory
        std::string command;
        int rc;
        command = "cp '" + (fileDir / "current").string() + "' '" + (fileDir / "previous").string() + "'";
        rc = system(command.c_str());
        if (rc != 0) {
            std::cerr << "Failed to create a backup of the file.\n";
            exit(1);
        }
        command = "ln '" + (fileDir / "current").string() + "' '" + editFile.string() + "'";
        rc = system(command.c_str());
        if (rc != 0) {
            std::cerr << "Failed to create link to edit file.\n";
            exit(1);
        }
        // Begin editing file
        child_pid = fork();
        int child_status;
        if (child_pid < 0) {
            std::cerr << "Failed to open editor.\n";
            // exit(1);
        } else if (child_pid == 0) {
            char *args[3];
            args[0] = strdup(getenv("EDITOR"));
            args[1] = strdup(editFile.c_str());
            args[2] = NULL;
            execvp(args[0], args);
        } else {
            // Get latest file before editing
            try {
                for (const auto& entry: fs::directory_iterator(fileDir)) {
                    if (entry.is_regular_file()) {
                        try {
                            int versionNum = std::stoi(entry.path().filename());
                            if (maxVersionNumber < versionNum) {
                                maxVersionNumber = versionNum;
                            }
                        } catch (const std::invalid_argument& e) {
                            // Not a version number
                        }
                    }
                }
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Failed to get files in directory.\n";
            }
            fs::path latestFile = fileDir / std::to_string(maxVersionNumber + 1);

            wait(&child_status);
            if (child_status != 0) {
                std::cerr << "Failed to save file.\n";
                exit(1);
            }
            // Compare to previous file
            command = "diff '" + (fileDir / "previous").string() + "' '" + (fileDir / "current").string() + "'";
            std::array<char, BLOCK_SIZE> buffer;
            FILE* pipe = popen(command.c_str(), "r");

            if (!pipe) {
                std::cerr << "Failed to open pipe.\n";
                exit(1);
            }

            // Write diff output
            std::ofstream latest(latestFile);
            if (latest.fail()) {
                std::cerr << "Failed to open updated version file.\n";
                exit(1);
            }
            std::cout << buffer.empty() << '\n';
            while (fgets(buffer.data(), buffer.size(), pipe)) {
                // diffOutput += buffer.data();
                std::cout << buffer.data();
                latest << buffer.data();
            }
            latest.close();
            int diffStatus = pclose(pipe);
            // Perform cleanup
            remove(editFile.c_str());
            remove((fileDir / "previous").c_str());
            if (diffStatus == 0) {
                remove(latestFile); // No changes made
            }
            // Update file info
            struct stat file_stat;
            if (stat(fileDir.c_str(), &file_stat) == 0) {
                file.name = fileDir.filename().string();
                file.identifier = file_stat.st_ino;
                file.timestamp = file_stat.st_birthtimespec.tv_sec;
                file.size = file_stat.st_size;
            } else {
                std::cerr << "Error accessing file.\n";
                exit(1);
            }
        }
    }
}
// void FileSystemDisk::AddFile(FILE_INFO file)
// { // Adds file to directory, updates bitmap, adds to table (hashing)
//     std::string filename = file.name;
//     std::ofstream fileToAdd;
//     fileToAdd.open(this->diskPath / filename);

//     if (!fileToAdd.is_open())
//     {
//         std::cerr << "Failed to add file.";
//         exit(1);
//     }

//     unsigned int block = file.identifier % MAX_FILES;
//     // Collision handling    
//     /*
//         if identifier = iterate through existing identifiers (or check for 1 in hash locations)
//         rerun for a new one
//     */
//     this->UpdateBitmap(block, true);
    
//     fileToAdd.close();
// }

void FileSystemDisk::deleteFile(std::string filename) {
    FILE_INFO fileToDelete = this->CreateFile(filename);
    this->RemoveFile(fileToDelete);
}
void FileSystemDisk::RemoveFile(FILE_INFO file)
{
    std::string filename = file.name;
    // Find file
    fs::path fileDir = this->diskPath / file.name;
    unsigned int block = file.identifier % static_cast<unsigned long>(MAX_FILES) + 1; // Temporary test value, update to hash function later
    // Check for file
    if (!fs::remove_all(fileDir)) {
        std::cerr << "Failed to remove file.";
        exit(1);
    }

    this->UpdateBitmap(block, false);
    this->setBits.erase(block);
   // 1) Open original file (read) and create temporary file (write)
    std::string originalName = fs::path(this->diskPath / filesystemName).string();
    std::string tempName = fs::path(this->diskPath / "temp").string();

    std::ifstream original(originalName);
    std::ofstream temp(tempName);
    if (!original.is_open() || !temp.is_open()) {
        std::cerr << "Could not update filesystem.\n";
        fs::remove_all(fileDir);
        exit(1);
    }

    // 2) Get to correct line
    int currentLine = 0;
    std::string line;
    while (std::getline(original, line) && currentLine < MAX_FILES) {
        ++currentLine;
        if (currentLine == (block + 1)) { // + 1 so it doesn't overwrite bitmap
            // Remove inode and name
            temp << '\0' << '\n';
        } else {
            // 3) Write everything else
            temp << line << '\n';
        }
    }
    temp << line;

    original.close();
    temp.close();
    // 4) Replace the original with the temporary file
    remove(originalName.c_str());
    rename(tempName.c_str(), originalName.c_str()); 
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
    if (!original.is_open() || !temp.is_open()) {
        std::cerr << "Failed to make file.";
        exit(1);
    }

    // 2) Modify the first line (bitmap)
    std::string line;
    if (std::getline(original, line)) {
        std::string bitmapStr = this->fileBitmap.to_string();
        temp.write(bitmapStr.c_str(), bitmapStr.length());
    }

    // 3) Write everything else to the temporary file
    while (std::getline(original, line)) {
        temp << '\n' << line;
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
FILE_ENTRY FileSystemDisk::GetBlock(unsigned int block) {
    // Maybe repurpose to find block
    // Given a filename, iterate through set bits and match the file name
    // Return the inode at the found block or the correct block


    // Return the FILE_ENTRY at the associated block number
    return FILE_ENTRY{0, block};
}