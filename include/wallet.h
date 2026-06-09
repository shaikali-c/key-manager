#pragma once

#include <array>
#include <sodium.h>
#include <stdexcept>
#include <leveldb/db.h>

using PublicKey = std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>;
using SecretKey = std::array<unsigned char, crypto_sign_SECRETKEYBYTES>;

class Wallet {
    PublicKey publicKey{};
    SecretKey secretKey{};

    Wallet();
    ~Wallet() = default;

    public:
    static Wallet& getInstance() {
        static Wallet wallet;
        return wallet;
    }
    void createKeys();

    Wallet& operator=(const Wallet&) = delete;
    Wallet& operator=(Wallet&&) = delete;
    Wallet(const Wallet&) = delete;
    Wallet(Wallet&&) = delete;
};
