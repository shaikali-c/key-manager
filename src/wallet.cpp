#include "wallet.h"
#include <cmath>
#include <cpr/callback.h>

// Utility function implementations
//
Addr getBytesAddr(const std::string& hex) {
    Addr bytes{};
    if (sodium_hex2bin(bytes.data(), bytes.size(), hex.data(), hex.size(),
        nullptr, nullptr, nullptr) != 0) {
        throw std::runtime_error("Failed to convert address hex to bytes");
    }
    return bytes;
}

Hash hashBytesVector(const std::vector<unsigned char>& bytes) {
    Hash hash{};
    crypto_generichash(hash.data(), hash.size(), bytes.data(), bytes.size(), nullptr, 0);
    return hash;
}

// Wallet implementation
void Wallet::initCrypto() {
    if (sodium_init() < 0) {
        throw std::runtime_error("Failed to initialize libsodium");
    }
}

void Wallet::initDatabase() {
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::DB* db = nullptr;
    leveldb::Status status = leveldb::DB::Open(options, "wallet_data", &db);

    if (!status.ok()) {
        throw std::runtime_error("Failed to open LevelDB: " + status.ToString());
    }

    keysDatabase.reset(db);
}

void Wallet::deriveAddress() {
    crypto_generichash(address.data(), address.size(),
        publicKey.data(), publicKey.size(),
        nullptr, 0);
}

void Wallet::createKeys() {
    crypto_sign_keypair(publicKey.data(), secretKey.data());
    deriveAddress();
}

Wallet::Wallet() {
    initCrypto();
    createKeys();
    initDatabase();
}

void Wallet::saveKeys() {
    std::string keys;
    keys.reserve(publicKey.size() + secretKey.size());
    keys.append(reinterpret_cast<const char*>(publicKey.data()), publicKey.size());
    keys.append(reinterpret_cast<const char*>(secretKey.data()), secretKey.size());

    std::string ownerName;
    std::cout << "Save keys as: ";
    std::cin >> ownerName;

    leveldb::Status status = keysDatabase->Put(leveldb::WriteOptions(), ownerName, keys);
    if (!status.ok()) {
        throw std::runtime_error("Failed to save keys: " + status.ToString());
    }

    std::cout << "\nKeys saved successfully\n";
}

void Wallet::loadKeys() {
    std::string rawBytes, key;
    std::cout << "Enter keys name: ";
    std::cin >> key;

    leveldb::Status status = keysDatabase->Get(leveldb::ReadOptions(), key, &rawBytes);
    if (!status.ok()) {
        throw std::runtime_error("No keys found for: " + key);
    }

    if (rawBytes.size() != publicKey.size() + secretKey.size()) {
        throw std::runtime_error("Invalid key data size");
    }

    std::memcpy(publicKey.data(), rawBytes.data(), publicKey.size());
    std::memcpy(secretKey.data(), rawBytes.data() + publicKey.size(), secretKey.size());
    deriveAddress();
}

void Wallet::printKeys() const {
    std::cout << "Public key: " << getHex(publicKey) << "\n"
        << "Secret Key: " << getHex(secretKey) << "\n"
        << "Address: " << getHex(address) << "\n";
}

std::pair<std::vector<Input>, std::vector<UTXO>>
Wallet::prepareInputsOutputs(const Addr& receiver, uint64_t amount) {

    nlohmann::json requestUTXO{
        {"address", getHex(address)},
        {"coins", amount}
    };

    std::string url = "http://127.0.0.1:18080/utxo";
    cpr::Response response = cpr::Post(
        cpr::Url{url},
        cpr::Body{requestUTXO.dump()},
        cpr::Header{{"Content-Type", "application/json"}}
    );

    if (response.error.code != cpr::ErrorCode::OK) {
        throw std::runtime_error("Request failed: " + response.error.message);
    }

    json utxoResponse = json::parse(response.text);
    std::vector<Input> inputs;
    std::vector<UTXO> outputs;

    // Check if utxos exists and is an array
    if (!utxoResponse.contains("utxos") || !utxoResponse["utxos"].is_array()) {
        throw std::runtime_error("Invalid UTXO response format: missing or invalid utxos field");
    }

    // Check if utxos array is empty
    if (utxoResponse["utxos"].empty()) {
        throw std::runtime_error("No UTXOs available");
    }

    for (const auto& utxo : utxoResponse["utxos"]) {
        // Check utxoKey field
        if (!utxo.contains("utxoKey") ||
            !utxo["utxoKey"].is_string() ||
            utxo["utxoKey"].get<std::string>().empty()) {
            throw std::runtime_error("Missing or invalid utxoKey field in UTXO entry");
        }

        // Check outputIndex field
        if (!utxo.contains("outputIndex")) {
            throw std::runtime_error("Missing outputIndex field in UTXO entry");
        }

        if (!utxo["outputIndex"].is_number()) {
            throw std::runtime_error("outputIndex is not a number");
        }

        // Check coins field
        if (!utxoResponse.contains("coins")) {
            throw std::runtime_error("Missing coins field in UTXO entry");
        }

        if (!utxoResponse["coins"].is_number()) {
            throw std::runtime_error("coins is not a number");
        }

        std::string utxoKey = utxo["utxoKey"].get<std::string>();
        uint32_t outputIndex = utxo["outputIndex"].get<uint32_t>();

        inputs.emplace_back(getBytes<Hash>(utxoKey), outputIndex);
    }
    uint64_t totalCoins = utxoResponse["coins"].get<uint64_t>();

    if (totalCoins < amount) {
        std::cout << "Insufficient funds: have " << totalCoins << ", need " << amount << "\n";
        return { {}, {} };
    }

    outputs.emplace_back(receiver, amount);
    if (totalCoins > amount) {
        outputs.emplace_back(address, totalCoins - amount);
    }

    return { inputs, outputs };
}

void Wallet::createTransaction() {
    std::string receiverHex{};
    uint64_t amount{};
    double rawAmount{};

    std::cout << "Receiver address: ";
    std::cin >> receiverHex;
    std::cout << "Enter amount: ";
    std::cin >> rawAmount;

    amount = static_cast<uint64_t>(std::round(rawAmount * UNITS));

    Addr receiver = getBytesAddr(receiverHex);
    auto [inputs, outputs] = prepareInputsOutputs(receiver, amount);

    if (inputs.empty() || outputs.empty()) {
        std::cout << "Insufficient funds for transaction\n";
        return;
    }

    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    size_t bytesSize = address.size() + receiver.size() + sizeof(amount) + sizeof(timestamp);
    for (const auto& input : inputs) {
        bytesSize += input.transactionHash.size() + sizeof(input.outputIndex);
    }
    for (const auto& output : outputs) {
        bytesSize += output.owner.size() + sizeof(output.coins);
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(bytesSize);

    auto appendBytes = [&bytes](const auto& data) {
        const auto* ptr = reinterpret_cast<const unsigned char*>(&data);
        bytes.insert(bytes.end(), ptr, ptr + sizeof(data));
    };

    appendBytes(address);
    appendBytes(receiver);

    for (const auto& input : inputs) {
        appendBytes(input.transactionHash);
        appendBytes(input.outputIndex);
        std::cout << "\n Input: " << getHex(input.transactionHash) << "\nOutput Index: " << input.outputIndex << "\n";
    }

    for (const auto& output : outputs) {
        appendBytes(output.owner);
        appendBytes((output.coins));
        std::cout << "\n Output: " << getHex(output.owner) << "\nOutput Index: " << output.coins << "\n";
    }

    appendBytes(amount);
    appendBytes(timestamp);

    std::cout << "Timestamp: " << timestamp << "\n";

    Hash transactionHash = hashBytesVector(bytes);

    json transactionJson;
    json inputsJson = nlohmann::json::array();
    json outputsJson = nlohmann::json::array();

    transactionJson["transactionHash"] = getHex(transactionHash);
    transactionJson["sender"] = getHex(publicKey);
    transactionJson["receiver"] = receiverHex;
    transactionJson["timestamp"] = timestamp;
    transactionJson["amount"] = amount;
    transactionJson["signature"] = getHex(sign(transactionHash));

    for (const auto& i : inputs) {
        json input;
        inputsJson.push_back(getHex(i.transactionHash) + ":" + std::to_string(i.outputIndex));
    }

    for (const auto& o : outputs) {
        json utxo;
        utxo["address"] = getHex(o.owner);
        utxo["coins"] = o.coins;
        outputsJson.push_back(utxo);
    }

    transactionJson["inputs"] = inputsJson;
    transactionJson["outputs"] = outputsJson;


    cpr::Response r = cpr::Post(
            cpr::Url{"http://127.0.0.1:18080/createTransaction"},
            cpr::Body{transactionJson.dump()},
            cpr::Header{{"Content-Type", "application/json"}}
    );
    std::cout << r.text << "\n";

}

Signature Wallet::sign(Hash& hash) {
    Signature signature{};
    crypto_sign_detached(signature.data(), nullptr,
        hash.data(), hash.size(),
        secretKey.data());
    return signature;
}

bool Wallet::verifySignature(const Signature& signature, const Hash& hash) {
    return crypto_sign_verify_detached(signature.data(), hash.data(),
        hash.size(), publicKey.data()) == 0;
}
