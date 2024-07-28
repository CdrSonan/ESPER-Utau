#include <string>
#include <map>

#pragma once

float* readWavFile(const std::string& path, int* numSamples);

void writeWavFile(const std::string& path, float* samples, int sampleRate, int numSamples);

void readIniFile(const std::string& path, std::map<std::string, std::string>* iniMap);
