#include <iostream>
#include <map>
#include <vector>

void displayAsciiArt(char ch) {
    std::map<char, std::vector<std::string>> asciiArt = {
        {'A', {
            "  ##  ",
            " #  # ",
            " #### ",
            " #  # ",
            " #  # "
        }},
        {'B', {
            " ###  ",
            " #  # ",
            " ###  ",
            " #  # ",
            " ###  "
        }},
    };

    if (asciiArt.find(ch) != asciiArt.end()) {
        for (const auto& line : asciiArt[ch]) {
            std::cout << line << '\n';
        }
    } else {
        std::cout << "ASCII art not available for character: " << ch << '\n';
    }
}

int main() {
    char input;
    std::cout << "Enter a character: ";
    std::cin >> input;

    input = toupper(input);

    displayAsciiArt(input);

    return 0;
}
