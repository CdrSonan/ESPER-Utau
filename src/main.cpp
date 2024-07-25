#include <iostream>
#include <map>
#include <string>
#include <fstream>

#include "argparse.hpp"

int main(int argc, char* argv[]) {
    resamplerArgs args = parseArguments(argc, argv);
    // write args to cout
    std::cout << "Input path: " << args.inputPath << std::endl;
    std::cout << "Output path: " << args.outputPath << std::endl;
    std::cout << "Pitch: " << args.pitch << std::endl;
    std::cout << "Velocity: " << args.velocity << std::endl;
    std::cout << "Flags: " << std::endl;
    for (auto const& flag : args.flags) {
        std::cout << flag.first << ": " << flag.second << std::endl;
    }
    std::cout << "Offset: " << args.offset << std::endl;
    std::cout << "Length: " << args.length << std::endl;
    std::cout << "Consonant: " << args.consonant << std::endl;
    std::cout << "Cutoff: " << args.cutoff << std::endl;
    std::cout << "Volume: " << args.volume << std::endl;
    std::cout << "Modulation: " << args.modulation << std::endl;
    std::cout << "Tempo: " << args.tempo << std::endl;
    std::cout << "Pitch bend: " << std::endl;
    for (int i = 0; i < args.pitchBend.size(); i++) {
        std::cout << args.pitchBend[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}