#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#include "../include/crypto++/cryptlib.h"
#include "../include/crypto++/blake2.h"
#include <string>

std::string my_blakes(std::string s);

#endif