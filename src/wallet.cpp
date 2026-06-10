#include "wallet.h"

// Utility function implementations

Hash getBytes(const std::string& hex) {
    Hash bytes{};
    if (sodium_hex2bin(bytes.data(), bytes.size(), hex.data(), hex.size(),
        nullptr, nullptr, nullptr) != 0) {
        throw std::runtime_error("Failed to convert hex to bytes");
    }
    return bytes;
}

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
    cpr::Response response = cpr::Get(cpr::Url{ DEFAULT_UTXO_URL });

    if (response.error.code != cpr::ErrorCode::OK) {
        throw std::runtime_error("Request failed: " + response.error.message);
    }

    json utxoResponse = json::parse(response.text);
    std::vector<Input> inputs;
    std::vector<UTXO> outputs;
    uint64_t totalCoins = 0;

    if (!utxoResponse.contains("utxos") || !utxoResponse["utxos"].is_array()) {
        throw std::runtime_error("Invalid UTXO response format");
    }

    for (const auto& utxo : utxoResponse["utxos"]) {
        if (!utxo.contains("utxoKey") || !utxo["utxoKey"].is_string()) {
            throw std::runtime_error("Missing or invalid utxoKey field");
        }

        std::string utxoKey = utxo["utxoKey"].get<std::string>();
        size_t pos = utxoKey.find(':');

        if (pos == std::string::npos) {
            throw std::runtime_error("Invalid utxoKey format: " + utxoKey);
        }

        std::string transactionHashHex = utxoKey.substr(0, pos);
        std::string indexStr = utxoKey.substr(pos + 1);

        if (transactionHashHex.empty()) {
            throw std::runtime_error("Empty transaction hash");
        }

        uint32_t outputIndex = static_cast<uint32_t>(std::stoul(indexStr));
        uint64_t coins = utxo["coins"].get<uint64_t>();
        totalCoins += coins;

        Hash transactionHash = getBytes(transactionHashHex);
        inputs.emplace_back(transactionHash, outputIndex);
    }

    std::cout << "Inputs: " << inputs.size() << ", Total coins: " << totalCoins << "\n";

    if (totalCoins < amount) {
        return { {}, {} };
    }

    outputs.emplace_back(receiver, amount);
    if (totalCoins > amount) {
        outputs.emplace_back(address, totalCoins - amount);
    }

    return { inputs, outputs };
}

void Wallet::createTransaction() {
    std::string receiverHex;
    uint64_t amount;

    std::cout << "Receiver address: ";
    std::cin >> receiverHex;
    std::cout << "Enter amount: ";
    std::cin >> amount;

    Addr receiver = getBytesAddr(receiverHex);
    auto [inputs, outputs] = prepareInputsOutputs(receiver, amount);

    if (inputs.empty() || outputs.empty()) {
        std::cout << "Insufficient funds for transaction\n";
        return;
    }

    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Calculate required size efficiently
    size_t bytesSize = address.size() + receiver.size() + sizeof(amount) + sizeof(timestamp);
    for (const auto& input : inputs) {
        bytesSize += input.transactionHash.size() + sizeof(input.outputIndex);
    }
    for (const auto& output : outputs) {
        bytesSize += output.owner.size() + sizeof(output.coins);
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(bytesSize);

    // Serialize data
    auto appendBytes = [&bytes](const auto& data, size_t size) {
        const auto* ptr = reinterpret_cast<const unsigned char*>(&data);
        bytes.insert(bytes.end(), ptr, ptr + size);
        };

    appendBytes(address, address.size());
    appendBytes(receiver, receiver.size());

    for (const auto& input : inputs) {
        appendBytes(input.transactionHash, input.transactionHash.size());
        appendBytes(input.outputIndex, sizeof(input.outputIndex));
    }

    for (const auto& output : outputs) {
        appendBytes(output.owner, output.owner.size());
        appendBytes(output.coins, sizeof(output.coins));
    }

    appendBytes(amount, sizeof(amount));
    appendBytes(timestamp, sizeof(timestamp));

    Hash transactionHash = hashBytesVector(bytes);

    json transactionJson;
    json inputsJson = nlohmann::json::array();
    json outputsJson = nlohmann::json::array();

    transactionJson["transactionHash"] = getHex(transactionHash);
    transactionJson["sender"] = getHex(address);
    transactionJson["receievr"] = receiverHex;
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