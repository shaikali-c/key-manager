#pragma once

using json = nlohmann::json;
using Hash = std::array<unsigned char, crypto_generichash_BYTES>;
using Addr = std::array<unsigned char, 20>;
using Signature = std::array<unsigned char, crypto_sign_BYTES>;
using PublicKey = std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>;
using SecretKey = std::array<unsigned char, crypto_sign_SECRETKEYBYTES>;

struct UTXO {
    Addr owner;
    uint64_t coins;
    UTXO(const Addr& owner, uint64_t coins) : owner(owner), coins(coins) {}
};

struct Input {
    Hash transactionHash;
    uint32_t outputIndex;
    Input(const Hash& hash, uint32_t index) : transactionHash(hash), outputIndex(index) {}
};

class Wallet {
public:
    static Wallet& getInstance() {
        static Wallet instance;
        return instance;
    }

   	static constexpr uint64_t UNITS = 1000000;

    // Disable copy
    Wallet(const Wallet&) = delete;
    Wallet& operator=(const Wallet&) = delete;

    // Disable move
    Wallet(Wallet&&) = delete;
    Wallet& operator=(Wallet&&) = delete;

    bool status = false;
    void saveKeys();
    void loadKeys();
    void printKeys() const;
    void createTransaction();
    void createKeys();
    uint64_t checkBalance();
    bool verifySignature(const Signature& signature, const Hash& hash);

private:
    Wallet();
    ~Wallet() = default;

    void initCrypto();
    void initDatabase();
    void deriveAddress();

    std::pair<std::vector<Input>, std::vector<UTXO>> prepareInputsOutputs(const Addr& receiver, uint64_t amount);
    Signature sign(Hash& hash);

    std::unique_ptr<leveldb::DB> keysDatabase;
    PublicKey publicKey{};
    SecretKey secretKey{};
    Addr address{};
};

// Utility functions
template<typename T>
std::string getHex(const T& bytes) {
    std::string hex(bytes.size() * 2 + 1, '\0');
    sodium_bin2hex(hex.data(), hex.size(), bytes.data(), bytes.size());
    hex.pop_back(); // Remove null terminator
    return hex;
}
template <typename T>
T getBytes(const std::string& hex) {
    T bytes{};
    if (sodium_hex2bin(bytes.data(), bytes.size(), hex.data(), hex.size(),
        nullptr, nullptr, nullptr) != 0) {
        throw std::runtime_error("Failed to convert hex to bytes");
    }
    return bytes;
}
Addr getBytesAddr(const std::string& hex);
Hash hashBytesVector(const std::vector<unsigned char>& bytes);
