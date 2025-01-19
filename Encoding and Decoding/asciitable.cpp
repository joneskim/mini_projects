#include <iostream>
#include <iomanip>

int main() {
        // Print the header of the table
    std::cout << "Dec  Hex  Oct  Char\n";
    std::cout << "---------------------\n";

    for (int i = 0; i <= 127; ++i){
        std::cout << std::dec << std::setw(3) << i << "  ";
        std::cout << std::hex << std::setw(3) << i << "  ";
        std::cout << std::oct << std::setw(3) << i << "  ";

        if (i >= 32 && i <= 126){
            std::cout << static_cast<char>(i) << '\n';
        } else {
            std::cout << '\n';
        }

        std::cout << std::dec;
    }
    return 0;
}
