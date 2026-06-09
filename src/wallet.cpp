#include "wallet.h"
#include <sodium/crypto_sign.h>

Wallet::Wallet() {
    if(sodium_init() != 0)
        throw std::runtime_error("Failed to initialize crypto library");
    createKeys();
}

void Wallet::createKeys() {
    crypto_sign_keypair(publicKey.data(), secretKey.data());
}
