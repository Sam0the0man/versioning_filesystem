#include <iostream>
#include <unistd.h>
#include <sstream>
#include <sys/wait.h>
#include "diskLibrary.h"
// Main interface for interacting with filesystem
void init(std::string filesystemName) {
    FileSystemDisk disk(filesystemName);
}

int main(int argc, char **argv) {
    char* fName = getenv("VFS_FILESYSTEM");
    char* fPath = getenv("VFS_FILESYSTEM_PATH");
    if (fName == nullptr || fPath == nullptr) {
        std::cerr << "Setup not complete. Please source: " << "init.sh" << '\n';
        exit(-1);
    }

    std::string filesystemName(fName);

    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <command> ... \n";
        exit(1);
    }

    FileSystemDisk disk(filesystemName);

    // Get command to execute
    std::string command(argv[1]);
    
    if (command == "list") {
        disk.listFiles();
    } else {
        if (argc < 3 || argc > 5) {
            std::cerr << "usage: " << argv[0] << " <command> <filename> [options]\n";
            exit(1);
        }
        std::string filename(argv[2]);
        if (command == "create") {
            for (int i = 2; i < argc; ++i) {
                filename = std::string(argv[i]);
                disk.createFile(filename);
            }                
        } else if (command == "edit") {
            disk.editFile(filename);
        } else if (command == "remove") {
            for (int i = 2; i < argc; ++i) {
                filename = std::string(argv[i]);
                disk.deleteFile(filename);
            }
        } else {
            // Check for version
            std::string option;
            int version;
            if (command == "view") {
                if (argc == 3) {
                    disk.viewFile(filename);
                } else if (argc == 4) {
                    std::cerr << "usage: " << argv[0] << " " << command << " " << filename << " <-n/-p> <version number>\n";
                    exit(1);
                } else if (argc == 5) {
                    option = argv[3];
                    try {
                        version = std::stoi(argv[4]);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "usage: " << argv[0] << " " << command << " " << filename << " <-n/-p> <version number>\n";
                        exit(1);
                    }
                    if (option == "-p") {
                        version *= -1;
                    } else if (option == "-n") {
                        version *= 1;
                    } else {
                        std::cerr << "usage: " << argv[0] << " " << command << " " << filename << " <-n/-p> <version number>\n";
                        exit(1);
                    }
                    disk.viewFile(filename, version);
                }
            } else if (command == "restore") {
                if (argc == 3) {
                    // Output all versions and ask which version to output
                    int fd[2];
                    pipe(fd);

                    pid_t pid = fork();

                    if (pid < 0) {
                        std::cerr << "Failed to list all file versions.\n";
                        exit(1);
                    } else if (pid == 0) {
                        // Write to pager
                        close(fd[0]); // Close read end
                        dup2(fd[1], STDOUT_FILENO); // Redirect STDOUT to pipe
                        dup2(fd[1], STDERR_FILENO); // Redirect STDERR to pipe
                        close(fd[1]);

                        disk.viewAllVersions(filename);

                        exit(0);
                    } else {
                        // Run pager, reading from pipe
                        close(fd[1]); // Close write end
                        FILE* pager = popen("less -R", "w");
                        
                        char buffer[BLOCK_SIZE];
                        char ch;
                        while ((ch = read(fd[0], buffer, BLOCK_SIZE)) > 0) {
                            fwrite(buffer, 1, ch, pager);
                        } // Write child output to pager

                        close(fd[0]);
                        pclose(pager);
                        waitpid(pid, nullptr, 0);
                    }
                    // Request which file version to restore
                    std::cout << "Version number to restore: ";
                    std::cin >> version;
                    disk.restoreFile(filename, version);
                } else if (argc == 4) {
                   try {
                        version = std::stoi(argv[4]);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "usage: " << argv[0] << " " << command << " " << filename << " [-p] <version number>\n";
                        exit(1);
                    } 
                    disk.restoreFile(filename, version);
                } else if (argc == 5) {
                    option = argv[3];
                    try {
                        version = std::stoi(argv[4]);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "usage: " << argv[0] << " " << command << " " << filename << " <-n/-p> <version number>\n";
                        exit(1);
                    }
                    if (option == "-p") {
                        version *= -1;
                    } else {
                        std::cerr << "usage: " << argv[0] << " " << command << " " << filename << " <-n/-p> <version number>\n";
                        exit(1);
                    }
                    disk.restoreFile(filename, version);
                }
            }
        }
    }

}

// Commands
// vfs list
// vfs create file_name
// vfs edit file_name
// vfs remove file_name
// vfs view file_name [-n/-p version]
// vfs restore file_name [[-p] version]