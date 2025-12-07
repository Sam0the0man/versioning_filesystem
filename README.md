# versioning_filesystem
Versioning File System as a final project for Operating Systems (CISC3595)

Installation:
1. Download folder
2. Enter: `make install`
3. Follow the instructions given by step 2's output

Deactivation:
1. Enter `source ./bin/deinit.sh` with the installed folder's path

Removal:
1. Enter `make clean`
2. Follow the instructions given by step 1's output

Commands:
-Lists all file names-
ls, or vfs list
-Creates a file-
touch file_name, or vfs create file_name
-Edits a file if it exists; if it doesn't exist, creates, then allows you to edit a file-
edit file_name, or vfs edit file_name
-Removes a file if it exists; if it doesn't exist, creates then deletes the file-
rm file_name, or vfs remove file_name
-Shows a file's contents-
cat file_name [-n/-p version], or vfs view file_name [-n/-p version]
-Allows you to restore a file to a previous version-
res file_name [[-p] version], or vfs restore file_name [[-p] version]

Notes:
-If a file's size is bigger than 4096, a new block will be created for it.
-You can create a maximum number of 255 files (or take up to a maximum of 255 file spaces in the bitmap).
-To remove multiple files at once, just enter the file names in succession: rm file_name file2_name
-To create multiple files at once, just enter the file names in succession: touch file_name file2_name