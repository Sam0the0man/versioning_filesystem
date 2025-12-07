#include <array>
#include <filesystem>
#include <bitset>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include "diskLibrary.h"

FileSystemDisk::FileSystemDisk(std::string filesystemName) {
    this->filesystemName = "." + filesystemName;

    fs::path homePath;
    homePath = getenv("VFS_FILESYSTEM_PATH");
    // Check if filesystem_name exists
    this->diskPath = homePath / this->filesystemName;
    if (fs::is_directory(diskPath)) {
        // If it does, load filesystem
        LoadDisk();
    } else {
        // If not, create filesystem
        CreateDisk();
        LoadDisk();
    }
}

void FileSystemDisk::CreateDisk() {
    // Create directory called filesystemName
    fs::create_directory(diskPath);
    // Create file called filesystemName
    std::ofstream file(diskPath / filesystemName);
    if (file.fail()) {
        std::cerr << "Filesystem could not be created.";
        exit(1);
    }
    // Write bitmap to allocate data at the beginning of the resulting string
    this->fileBitmap[0] = 1;
    std::string bitmapStr = this->fileBitmap.to_string();
    file.write(bitmapStr.c_str(), bitmapStr.length());

    // Allocate blocks

    // Ensure consistency in block allocation
    for (int i = 0; i < MAX_FILES; ++i) {
        // Correct block allocation for file information
        file.write("\n\0", 2);
    }

    file.close();
}
void FileSystemDisk::LoadDisk() {
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

    file.close();
}

void FileSystemDisk::createFile(std::string filename) {
    FILE_INFO info = this->CreateFile(filename);
}
FILE_INFO FileSystemDisk::CreateFile(std::string filename) {
    fs::path fileDir = this->diskPath / filename;

    unsigned int block = this->FindBlock(filename);
    if (block != -1) {
        // File already exists
        FILE_INFO file = this->ReadBlock(block);
        return file;
    }

    block = this->GetNextFreeBlock();
    if (block == -1) {
        // No free blocks, filesystem is full
        std::cerr << "You have reached your maximum number of files...\n";
        exit(1);
    }

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
    FILE_INFO info = this->GetFileInfo(filename);
    // Update bitmap
    this->UpdateBitmap(block, true);
    this->UpdateFileSystem(block, info);
    return info;
}

void FileSystemDisk::editFile(std::string filename) {
    FILE_INFO info{filename};
    this->EditFile(info);
}
void FileSystemDisk::EditFile(FILE_INFO &file) {
    int block = this->FindBlock(file.name);
    if (block == -1) {
        // Check if file exists, create if it doesn't
        FILE_INFO createdFile = this->CreateFile(file.name);
        this->EditFile(createdFile);
    } else {
        fs::path fileDir = this->diskPath / file.name;
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
            while (fgets(buffer.data(), buffer.size(), pipe)) {
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
            FILE_INFO old_info = this->ReadBlock(block);
            file = this->GetFileInfo(file.name);
            // Allocate new blocks if file size is larger than block size
            // Remove blocks if file size is smaller than previously required blocks
            int newBlocks = file.size / BLOCK_SIZE + 1;
            file.blockCount = newBlocks;
            this->UpdateFileSystem(block, file);
            if (newBlocks > old_info.blockCount) {
                for (int i = old_info.blockCount; i < newBlocks; ++i) {
                    unsigned int nextBlock = this->GetNextFreeBlock();
                    if (nextBlock < 1) {
                        break;
                    }
                    this->UpdateBitmap(nextBlock, true);
                    this->UpdateFileSystem(nextBlock, file);
                }
            } else if (newBlocks < old_info.blockCount) {
                for (int i = old_info.blockCount; i > newBlocks && i > 0; --i) {
                    unsigned int unusedBlock = this->FindBlock(file.name);
                    this->UpdateBitmap(unusedBlock, false);
                }
            }
        }
    }
}

void FileSystemDisk::deleteFile(std::string filename) {
    FILE_INFO fileToDelete = this->CreateFile(filename);
    this->RemoveFile(fileToDelete);
}
void FileSystemDisk::listFiles() const {
    this->ListFiles();
}
void FileSystemDisk::ListFiles() const {
    // Load filesystemName
    std::ifstream file(this->diskPath / this->filesystemName);
    if (file.fail()) {
        std::cerr << "Filesystem not found.";
        exit(1);
    }
    // Read file
    unsigned int currentBlock = 0;
    std::string line;
    std::string id, name, size;
    char tab;
    std::set<std::string> seenFiles;
    std::getline(file, line); // Ignore bitmap
    while (std::getline(file, line) && currentBlock <= MAX_FILES) {
        ++currentBlock;
        // Process current block info
        if (!this->fileBitmap.test(currentBlock)) { // Block not in use
            continue;
        } else {
            // Process file entry
            std::istringstream info(line);
            info >> id >> name >> size;
            if (seenFiles.find(id) == seenFiles.end()) {
                std::cout << name << '\n';
                seenFiles.insert(id);
            }
        }
    }
}
void FileSystemDisk::viewFile(std::string filename, int version) const {
    this->ViewFile(filename, version);
}
void FileSystemDisk::ViewFile(std::string filename, int version) const {
    std::string command;
    fs::path fileDir = this->diskPath / filename;
    if (this->FindBlock(filename) == -1) {
        std::cerr << "File does not exist.\n";
        exit(1);
    }
    if (version == 0) {
        // Output current
        command = "cat '" + (fileDir / "current").string() + "'";
        system(command.c_str());
    } else {
        int maxVersionNumber = 0;
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
        int versionCount = abs(version);
        if (versionCount > maxVersionNumber) {
            std::cerr << "Not an available version.\n";
            exit(1);
        }
        fs::path reconstructionFile = fileDir / "reconstruction";
        if (version < 0) {
            // Output n versions before current
            // Copy current file
            command = "cp '" + (fileDir / "current").string() + "' '" + reconstructionFile.string() + "'";
            system(command.c_str());
            // Apply patches
            for (int i = 0; i < versionCount; ++i) {
                // Patch from restoration to maxVersionCount - i
                command = "patch -s -R '" + reconstructionFile.string() + "' < '" + (fileDir / std::to_string(maxVersionNumber - i)).string() + "'";
                system(command.c_str());
            }
        } else {
            // Output n versions after initial
            // Copy initial file
            command = "cp '" + (fileDir / "0").string() + "' '" + reconstructionFile.string() + "'";
            system(command.c_str());
            // Apply patches
            for (int i = 1; i <= versionCount; ++i) {
                // Patch from restoration to i
                command = "patch -s '" + reconstructionFile.string() + "' '" + (fileDir / std::to_string(i)).string() + "'";
                system(command.c_str());
            }
        }
        // Output reconstructed file
        command = "cat '" + reconstructionFile.string() + "'";
        system(command.c_str());
        
        // Cleanup
        remove(reconstructionFile.c_str());
    }
}

void FileSystemDisk::viewAllVersions(std::string filename) const {
    fs::path fileDir = this->diskPath / filename;
    int maxVersionNumber = 0;
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
    std::string command;
    fs::path reconstructionFile = fileDir / "reconstruction";
    command = "cp '" + (fileDir / "0").string() + "' '" + reconstructionFile.string() + "'";
    system(command.c_str());
    // Apply patches
    for (int i = 1; i <= maxVersionNumber; ++i) {
        // Patch from restoration to i
        command = "patch -s '" + reconstructionFile.string() + "' '" + (fileDir / std::to_string(i)).string() + "'";
        system(command.c_str());
        // Output version and contents
        std::cout << i << ":" << std::endl;
        command = "cat '" + reconstructionFile.string() + "'";
        system(command.c_str());
    }

    // Cleanup
    remove(reconstructionFile.c_str());
}

void FileSystemDisk::restoreFile(std::string filename, int version) {
    this->RestoreFile(filename, version);
}
void FileSystemDisk::RestoreFile(std::string filename, int version) {
    std::string command;
    fs::path fileDir = this->diskPath / filename;
    int newLatestVersion;
    if (this->FindBlock(filename) == -1) {
        std::cerr << "File does not exist.\n";
        exit(1);
    }
    if (version == 0) {
        // Output current
        std::cout << "Same file version";
        return;
    } else {
        int maxVersionNumber = 0;
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

        int versionCount = abs(version);
        if (versionCount > maxVersionNumber) {
            std::cerr << "Not an available version.\n";
            exit(1);
        }
        
        fs::path reconstructionFile = fileDir / "reconstruction";
        if (version < 0) {
            // Output n versions before current
            // Copy current file
            command = "cp '" + (fileDir / "current").string() + "' '" + reconstructionFile.string() + "'";
            system(command.c_str());
            // Apply patches
            for (int i = 0; i < versionCount; ++i) {
                // Patch from restoration to maxVersionCount - i
                command = "patch -s -R '" + reconstructionFile.string() + "' < '" + (fileDir / std::to_string(maxVersionNumber - i)).string() + "'";
                system(command.c_str());
            }
            newLatestVersion = maxVersionNumber - versionCount;
        } else {
            // Output n versions after initial
            // Copy initial file
            command = "cp '" + (fileDir / "0").string() + "' '" + reconstructionFile.string() + "'";
            system(command.c_str());
            // Apply patches
            for (int i = 1; i <= versionCount; ++i) {
                // Patch from restoration to maxVersionCount - i
                command = "patch -s '" + reconstructionFile.string() + "' '" + (fileDir / std::to_string(i)).string() + "'";
                system(command.c_str());
            }
            newLatestVersion = version;
        }
        // Replace current with reconstructed file
        rename(reconstructionFile.c_str(), (fileDir / "current").c_str());
        
        // Remove all later versions
        for (int i = newLatestVersion + 1; i <= maxVersionNumber; ++i) {
            remove((fileDir / std::to_string(i)).c_str());
        }
    }
}

void FileSystemDisk::RemoveFile(FILE_INFO file) {
    // Find file
    fs::path fileDir = this->diskPath / file.name;
    unsigned int block = this->FindBlock(file.name);
    // Check for file
    if (block == -1) {
        std::cerr << "Failed to remove file.";
        exit(1);
    }

    // Update bitmap
    while (block != -1) {
        this->UpdateBitmap(block, false);
        block = this->FindBlock(file.name);
    }
    fs::remove_all(fileDir);
}

void FileSystemDisk::UpdateBitmap(unsigned int block, bool allocated) {
    this->fileBitmap[block] = allocated;
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
    rename(tempName.c_str(), originalName.c_str());
}
void FileSystemDisk::UpdateFileSystem(unsigned int block, FILE_INFO file) {
     // 1) Open original file (read) and create temporary file (write)
    std::string originalName = fs::path(this->diskPath / filesystemName).string();
    std::string tempName = fs::path(this->diskPath / "temp").string();

    std::ifstream original(originalName);
    std::ofstream temp(tempName);
    if (!original.is_open() || !temp.is_open()) {
        std::cerr << "Could not update filesystem.\n";
        exit(1);
    }

    // 2) Get to correct line
    int currentLine = 0;
    std::string line;
    while (std::getline(original, line) && currentLine < MAX_FILES) {
        ++currentLine;
        if (currentLine == (block + 1)) { // + 1 so it doesn't overwrite bitmap
                                          // Write identifier, name, size, block count
            temp << file.identifier << '\t' << file.name << '\t' << file.size << '\t' << file.blockCount << '\n';
        } else {
            // 3) Write everything else
            temp << line << '\n';
        }
    }
    if (block == MAX_FILES) {
        temp << file.identifier << '\t' << file.name << '\t' << file.size << '\t' << file.blockCount;
    } else {
        temp << line;
    }
    original.close();
    temp.close();
    // 4) Replace the original with the temporary file
    rename(tempName.c_str(), originalName.c_str());
}

FILE_INFO FileSystemDisk::GetFileInfo(const std::string& filename) const {
    fs::path fileDir = this->diskPath / filename;
    FILE_INFO file;
    // Update file info
    struct stat file_stat;
    if (stat(fileDir.c_str(), &file_stat) == 0) {
        file.name = fileDir.filename().string();
        file.identifier = file_stat.st_ino;
        #if defined(__APPLE__)
        // if (strcmp(SYSTEM_NAME, "macOS") == 0) {
            file.timestamp = file_stat.st_birthtimespec.tv_sec;
        #elif defined(_WIN32)
            file.timestamp = 0; // Not supported
        #else
        // } else if (strcmp(SYSTEM_NAME, "Linux") == 0) {
            file.timestamp = file_stat.st_ctime;
        // }
        #endif
        // Get total size of all files in directory
        std::string result = "";
        std::string command = "du -s '" + fileDir.string() + "'";
        FILE* pipe = popen(command.c_str(), "r"); // Open pipe for reading
        if (!pipe) {
            std::cerr << "Failed to get size.\n";
            exit(1);
        }

        char buffer[128]; // Buffer to read chunks of output
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }

        pclose(pipe); // Close the pipe
        std::istringstream size_info(result);
        size_info >> file.size;
        file.blockCount = 1;
    } else {
        std::cerr << "Error accessing file.\n";
        exit(1);
    }
    return file;
}
int FileSystemDisk::GetNextFreeBlock() const {
    for (unsigned int i = 1; i <= MAX_FILES; ++i) {
        if (!this->fileBitmap.test(i)) {
            return i;
        }
    }
    return -1; // No free blocks
}
int FileSystemDisk::FindBlock(const std::string& filename) const {
    // Load filesystemName
    std::ifstream file(this->diskPath / this->filesystemName);
    if (file.fail()) {
        std::cerr << "Filesystem not found.";
        exit(1);
    }
    // Read file
    unsigned int currentBlock = 0;
    std::string line;
    std::string id, name, size;
    char tab;
    std::getline(file, line); // Ignore bitmap
    while (std::getline(file, line) && currentBlock <= MAX_FILES) {
        ++currentBlock;
        // Process current block info
        if (!this->fileBitmap.test(currentBlock)) { // Block not in use
            continue;
        } else {
            // Process file entry
            std::istringstream info(line);
            info >> id >> name >> size;
            
            if (name == filename) {
                return currentBlock;
            }
        }
    }

    return -1; // File not found
}
FILE_INFO FileSystemDisk::ReadBlock(unsigned int block) const {
    // Load filesystemName
    std::ifstream file(this->diskPath / this->filesystemName);
    if (file.fail()) {
        std::cerr << "Filesystem not found.";
        exit(1);
    }
    FILE_INFO info;

    unsigned int currentBlock = 0;
    std::string line;
    unsigned long identifier;
    std::string name;
    size_t size;
    std::getline(file, line); // Ignore bitmap
    while (std::getline(file, line) && currentBlock <= MAX_FILES) {
        ++currentBlock;
        // Process current block info
        if (currentBlock == block) { // Return file info
            // Process file entry
            std::istringstream file_info(line);
            file_info >> info.identifier >> info.name >> info.size >> info.blockCount;
            break;
        }
    }
    return info;
}