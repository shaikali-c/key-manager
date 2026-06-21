#include "wallet.h"
#include <sodium/crypto_sign.h>

void signHash() {
    std::string hashHex;
    std::string secretKeyHex;

    std::cout << "Hash hex: ";
    std::cin >> hashHex;
    std::cout << "\nPublic Key: ";
    std::cin >> secretKeyHex;
    Hash hash = getBytes<Hash>(hashHex);
    SecretKey secretKey = getBytes<SecretKey>(secretKeyHex);
    Signature signature{};
    crypto_sign_detached(signature.data(), nullptr, hash.data(), hash.size(), secretKey.data());
    std::cout << "\nSignature: " << getHex(signature) << "\n";
}

int main() {
    if (sodium_init() < 0) {
        std::cout << "Failed to initialize sodium\n";
        return 1;
    }

    Wallet& wallet = Wallet::getInstance();

    int choice;

    while (true) {
        std::cout << "\n";
        std::cout << "1. Create Keys\n";
        std::cout << "2. Load Keys\n";
        std::cout << "3. Save Keys\n";
        std::cout << "4. Print Keys\n";
        std::cout << "5. Create Transaction\n";
        std::cout << "6. Verify Signature\n";
        std::cout << "7. Sign hash\n";
        std::cout << "Choice: ";

        std::cin >> choice;

        switch (choice) {
        case 1:
            wallet.createKeys();
            std::cout << "Keys created\n";
            break;

        case 2:
            wallet.loadKeys();
            std::cout << "Keys loaded\n";
            break;

        case 3:
            wallet.saveKeys();
            std::cout << "Keys saved\n";
            break;

        case 4:
            wallet.printKeys();
            break;

        case 5:
            wallet.createTransaction();
            break;

        case 6: {
            std::string signatureHex;
            std::string hashHex;

            std::cout << "Signature: ";
            std::cin >> signatureHex;

            std::cout << "Hash: ";
            std::cin >> hashHex;

            bool valid = wallet.verifySignature(
                getBytes<Signature>(signatureHex),
                getBytes<Hash>(hashHex)
            );

            std::cout << (valid ? "Valid\n" : "Invalid\n");
            break;
        }

        case 7:
            signHash();
            break;
        default:
            std::cout << "Invalid choice\n";
            break;
        }
    }
}
