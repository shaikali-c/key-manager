#include "wallet.h"
#include <cstdlib>  // for system()

void printMenu() {
    std::cout << "\n====================================\n";
    std::cout << "         CRYPTO WALLET MENU           \n";
    std::cout << "======================================\n";
    std::cout << "  1. Create New Keys                  \n";
    std::cout << "  2. Load Existing Keys               \n";
    std::cout << "  3. Save Keys to Database            \n";
    std::cout << "  4. Print Keys & Address             \n";
    std::cout << "  5. Create Transaction               \n";
    std::cout << "  6. View Balance                     \n";
    std::cout << "  7. Exit                             \n";
    std::cout << "======================================\n";
    std::cout << "  Enter your choice (1-7): ";
}

void printHeader(const std::string& title) {
    std::cout << "\n-------------------------------------\n";
    std::cout << "  " << title << "\n";
    std::cout << "----------------------------------------\n";
}

void pauseForUser() {
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore();
    std::cin.get();
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void displayKeysInfo(const Wallet& wallet) {
    printHeader("WALLET INFORMATION");
    wallet.printKeys();
}

void displayTransactionPrompt() {
    printHeader("CREATE NEW TRANSACTION");
    std::cout << "Preparing to create transaction...\n";
}

int main() {
    std::cout << "=== PROGRAM START ===" << std::endl;

    // Check sodium initialization
    std::cout << "Initializing sodium..." << std::endl;
    if (sodium_init() < 0) {
        std::cerr << "Failed to initialize sodium library!\n";
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }
    std::cout << "Sodium initialized successfully!" << std::endl;

    try {
        std::cout << "Getting wallet instance..." << std::endl;
        Wallet& wallet = Wallet::getInstance();
        std::cout << "Wallet instance obtained!" << std::endl;

        int choice;
        bool running = true;

        clearScreen();
        std::cout << "  WELCOME TO CRYPTO WALLET SYSTEM\n";
        std::cout << "  ===============================\n";

        while (running) {
            printMenu();

            // Check if input is valid
            if (!(std::cin >> choice)) {
                std::cin.clear();  // Clear error flags
                std::cin.ignore(10000, '\n');  // Discard invalid input
                std::cout << "\n[ERROR] Invalid input! Please enter a number.\n";
                pauseForUser();
                clearScreen();
                continue;
            }

            try {  // Inner try-catch for each operation
                switch (choice) {
                case 1: {
                    clearScreen();
                    printHeader("CREATE NEW KEYS");
                    std::cout << "Generating new cryptographic keys...\n";
                    wallet.createKeys();
                    std::cout << "[SUCCESS] Keys created successfully!\n";
                    pauseForUser();
                    clearScreen();
                    break;
                }

                case 2: {
                    clearScreen();
                    printHeader("LOAD EXISTING KEYS");
                    std::cout << "Loading keys from database...\n";
                    wallet.loadKeys();
                    std::cout << "[SUCCESS] Keys loaded successfully!\n";
                    pauseForUser();
                    clearScreen();
                    break;
                }

                case 3: {
                    clearScreen();
                    printHeader("SAVE KEYS TO DATABASE");
                    std::cout << "Saving current keys to database...\n";
                    wallet.saveKeys();
                    std::cout << "[SUCCESS] Keys saved successfully!\n";
                    pauseForUser();
                    clearScreen();
                    break;
                }

                case 4: {
                    clearScreen();
                    displayKeysInfo(wallet);
                    pauseForUser();
                    clearScreen();
                    break;
                }

                case 5: {
                    clearScreen();
                    displayTransactionPrompt();
                    wallet.createTransaction();
                    std::cout << "\n[SUCCESS] Transaction created successfully!\n";
                    pauseForUser();
                    clearScreen();
                    break;
                }

                case 6: {
                    clearScreen();
                    printHeader("VIEW BALANCE");
                    std::cout << "Feature coming soon...\n";
                    std::cout << "Use UTXO endpoint to check balance.\n";
                    pauseForUser();
                    clearScreen();
                    break;
                }

                case 7: {
                    clearScreen();
                    std::cout << "\n  Thank you for using Crypto Wallet!\n";
                    std::cout << "  Goodbye!\n\n";
                    running = false;
                    break;
                }

                default: {
                    std::cout << "\n[ERROR] Invalid choice! Please enter a number between 1-7.\n";
                    pauseForUser();
                    clearScreen();
                    break;
                }
                }
            }
            catch (const std::runtime_error& e) {
                std::cerr << "\n[RUNTIME ERROR] " << e.what() << std::endl;
                pauseForUser();
                clearScreen();
            }
            catch (const std::logic_error& e) {
                std::cerr << "\n[LOGIC ERROR] " << e.what() << std::endl;
                pauseForUser();
                clearScreen();
            }
            catch (const std::exception& e) {
                std::cerr << "\n[STANDARD EXCEPTION] " << e.what() << std::endl;
                pauseForUser();
                clearScreen();
            }
            catch (...) {
                std::cerr << "\n[UNKNOWN EXCEPTION] Unknown exception caught!" << std::endl;
                std::cerr << "No exception object available" << std::endl;
                pauseForUser();
                clearScreen();
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\n=== FATAL ERROR in main setup ===" << std::endl;
        std::cerr << "Exception: " << e.what() << std::endl;
        std::cerr << "===================================" << std::endl;
        std::cout << "\nPress Enter to exit...";
        std::cin.ignore();
        std::cin.get();
        return 1;
    }
    catch (...) {
        std::cerr << "\n=== FATAL UNKNOWN ERROR in main setup ===" << std::endl;
        std::cerr << "Press Enter to exit...";
        std::cin.ignore();
        std::cin.get();
        return 1;
    }

    std::cout << "=== PROGRAM END ===" << std::endl;
    return 0;
}