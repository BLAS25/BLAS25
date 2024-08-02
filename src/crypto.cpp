
#include "../include/crypto.h"
#include <iostream>

using namespace CryptoPP;

std::string my_blakes(std::string s) {
    std::string digest;
    BLAKE2s hash;
    hash.Update((const byte *)s.data(), s.size());
    digest.resize(hash.DigestSize());
    hash.Final((byte *)&digest[0]);
    return digest;
}
