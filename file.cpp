// #include <cryptopp/sha.h>
// #include <cryptopp/hex.h>
#include <utility>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "file.h"

// std::string sha256_cryptopp(const std::string& input) {
//     CryptoPP::SHA256 hash;
//     std::string digest;

//      CryptoPP::StringSource s(input, true,
//         new CryptoPP::HashFilter(hash,
//             new CryptoPP::HexEncoder(
//                 new CryptoPP::StringSink(digest)
//             ) // HexEncoder
//         ) // HashFilter
//     ); // StringSource

//     return digest;
// }

// FILE_INFO::FILE_INFO(std::string name, std::time_t timestamp) {
//     this->name = name;
//     this->timestamp = timestamp;
//     this->size = 1024;

//     // Format timestamp to "yyyy-mm-dd-HH-MM"
//     std::ostringstream oss;
//     oss << std::put_time(localtime(&timestamp), "%Y-%m-%d-%H-%M");
//     std::string timestampStr = oss.str();
//     std::string name_timestamp = this->name + timestampStr;

//     size_t h = std::hash<std::string>{}(name_timestamp);
//     this->identifier = h;
//     std::cout << this->identifier << '\n';
//     // this->identifier = sha256_cryptopp(name_timestamp);

//     this->prev = nullptr;
//     this->next = nullptr;
// }