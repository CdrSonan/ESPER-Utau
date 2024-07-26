#include <iostream>
#include <map>
#include <string>

#include <windows.h>

#include "argparse.hpp"

int main(int argc, char* argv[]) {
    resamplerArgs args = parseArguments(argc, argv);
    // write args to cout
    std::cout << "Resampler path: " << args.rsmpDir << std::endl;
    Sleep(100);
    std::cout << "Input path: " << args.inputPath << std::endl;
    Sleep(100);
    std::cout << "Output path: " << args.outputPath << std::endl;
    Sleep(100);
    std::cout << "Flags: " << std::endl;
    Sleep(100);
    for (auto const& flag : args.flags) {
        std::cout << flag.first << ": " << flag.second << std::endl;
        Sleep(100);
    }
    std::cout << "Pitch bend: " << std::endl;
    Sleep(100);
    for (int i = 0; i < args.pitchBend.size(); i++) {
        std::cout << args.pitchBend[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}