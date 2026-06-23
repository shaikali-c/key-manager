#include "wallet.h"
#include <functional>
#include <unordered_map>

void clearScreen() {
    #ifdef _WIN32
    system("cls");
    #else
    system("clear");
    #endif
}

int main() {
    if (sodium_init() < 0) {
        std::cout << "Failed to initialize sodium\n";
        return 1;
    }


    Wallet& wallet = Wallet::getInstance();
    clearScreen();

    std::string choice;
    std::unordered_map<std::string, std::function<void()>> commands;
    commands["create-keys"] = [&](){
        clearScreen();
        wallet.createKeys();
        std::cout << "\n\t[SUCCESS] Keys created\n\n\n";
    };
    commands["load-keys"] = [&](){
        clearScreen();
        wallet.loadKeys();
        std::cout << "\n\t[SUCCESS] Keys loaded\n\n";

    };
    commands["save-keys"] = [&](){
        clearScreen();
        wallet.saveKeys();
        std::cout << "\n\t[SUCCESS] Keys saved\n\n";

    };
    commands["print-keys"] = [&](){
        clearScreen();
        wallet.printKeys();
    };
    commands["create-transaction"] = [&](){
        clearScreen();
        wallet.createTransaction();
    };
    commands["ct"] = [&](){
        clearScreen();
        wallet.createTransaction();
    };
    commands["balance"] = [&](){
        clearScreen();
        std::cout << "\n\tAvailable Balance: " << wallet.checkBalance() << "\n\n";
    };
    commands["verify-signature"] = [&](){
        clearScreen();

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
    };
    commands["exit"] = [&](){ exit(0);
    };

    while (true) {
        std::cout << "\nCommands:\n";
        std::cout << "\tcreate-keys\n";
        std::cout << "\tload-keys\n";
        std::cout << "\tsave-keys\n";
        std::cout << "\tprint-keys\n";
        std::cout << "\tcreate-transaction\n";
        std::cout << "\tverify-signature\n";
        std::cout << "\tbalance\n";
        std::cout << "\texit\n";
        std::cout << "wallet > ";

        std::cin >> choice;

        auto it = commands.find(choice);
        if(it != commands.end()) {
            it->second();
        } else {
            clearScreen();
            std::cout << "\n\t[BAD]: Invalid command\n\n";
        }

    }
}
