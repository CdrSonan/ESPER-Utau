#include <string>
#include <vector>

#pragma once

struct resamplerArgs
{
    std::string inputPath;
    std::string outputPath;
    int pitch;
    float velocity;
    std::map<std::string, int> flags;
    float offset;
    int length;
    float consonant;
    float cutoff;
    float volume;
    float modulation;
    float tempo;
    std::vector<int> pitchBend;
};

resamplerArgs parseArguments(int argc, char* argv[]);