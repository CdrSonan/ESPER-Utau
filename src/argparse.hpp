#include <string>
#include <vector>

#pragma once

//struct holding all arguments passed to the resampler by Utau.
//members:
// - rsmpDir: the directory containing the resampler.
// - inputPath: the path to the input audio file.
// - outputPath: the path to the output audio file.
// - pitch: the desired MIDI pitch of the output.
// - velocity: unused.
// - flags: flag string decoded into a map of flag names and corresponding values.
// - offset: the offset of the input sample start from the input audio file start in milliseconds.
// - length: the desired length of the output sample in milliseconds.
// - consonant: the length of the consonant section in the input sample in milliseconds.
// - cutoff: the end point of the sample in the input audio file in milliseconds. Negative inputs are converted to their positive counterparts.
// - volume: unused.
// - modulation: unused.
// - tempo: unused.
// - pitchBend: the pitch bend data for the output sample, decoded into a vector of integers covering a millisecond of audio.
struct resamplerArgs
{
    std::string rsmpDir;
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
