#include <iostream>
#include <map>
#include <string>
#include <fstream>

#include "argparse.hpp"

int main(int argc, char* argv[]) {
    std::map<std::string, std::string> arguments = saveCommandLineArguments(argc, argv);

    // Print the saved arguments
    std::ofstream file("arguments.txt", std::ios::app); // Initialize the file directly
    for (const auto& argument : arguments) {
        std::cout << argument.first << " = " << argument.second << std::endl;
        //write the arguments to a file
        file << argument.first << " = " << argument.second << std::endl;
    }
    file.close();

    return 0;
}