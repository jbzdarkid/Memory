#include <iostream>
#include <string>

struct Foo {
    int k = 1;
    int j = 2;
    std::string s = "Secret value: 123456";
} _foo;

int a = 1;
int b = 1;

int main(int argc, char** argv) {
    std::string input;
    while (input != "0") {
        std::cout << "(1) Set a" << std::endl;
        std::cout << "(2) Set b" << std::endl;
        std::cout << "(3) Check a == b" << std::endl;
        std::cout << "(4) Print secret" << std::endl;
        std::cout << "(0) Exit" << std::endl;
        std::cout << ">";
        std::cin >> input;
        if (input == "1") {
            std::cout << "a=";
            std::cin >> input;
            a = std::stoi(input);
        } else if (input == "2") {
            std::cout << "b=";
            std::cin >> input;
            b = std::stoi(input);
        } else if (input == "3") {
            std::cout << (a == b ? "true" : "false") << std::endl;
        } else if (input == "4") {
            std::cout << _foo.s << std::endl;
        }
    }
    return 0;
}
