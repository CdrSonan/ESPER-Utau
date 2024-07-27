#include <iostream>
#include <string>
#include <map>
#include "fileio.hpp"

#include "AudioFile.h"

float* readWavFile(const std::string& path, int& sampleRate, int& numSamples) {
    AudioFile<float> audioFile;
    audioFile.load(path);
    sampleRate = audioFile.getSampleRate();
    numSamples = audioFile.getNumSamplesPerChannel();
    float* samples = new float[numSamples];
    for (int i = 0; i < numSamples; i++) {
        samples[i] = audioFile.samples[0][i];
    }
    return samples;
}

void writeWavFile(const std::string& path, float* samples, int sampleRate, int numSamples) {
    std::vector<std::vector<float>> sampleVec;
    sampleVec.push_back(std::vector<float>(samples, samples + numSamples));
    AudioFile<float> audioFile;
    audioFile.setSampleRate(sampleRate);
    audioFile.setAudioBuffer(sampleVec);
    audioFile.save(path);
}

std::map<std::string, std::string> readIniFile(const std::string& path) {
    std::map<std::string, std::string> iniMap;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        // Ignore comments, section headers and empty lines
        if (line.empty() || line[0] == ';' || line[0] == '[' || line[0] == '#') {
            continue;
        }
        // Find the position of the equals sign
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            // Extract the key and value
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            // Trim whitespace from key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            // Insert into the map
            iniMap[key] = value;
        }
    }
    return iniMap;
}
