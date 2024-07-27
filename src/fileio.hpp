#include <string>
#include <map>

#pragma once

float* readWavFile(const std::string& path, int& sampleRate, int& numSamples);

void writeWavFile(const std::string& path, float* samples, int sampleRate, int numSamples);

std::map<std::string, std::string> readIniFile(const std::string& path);
