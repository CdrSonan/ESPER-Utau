#include <string>
#include <map>

#pragma once

float* readWavFile(const std::string& path, int* numSamples);

void writeWavFile(const std::string& path, float* samples, int sampleRate, int numSamples);

void readIniFile(const std::string& path, std::map<std::string, std::string>* iniMap);

int readFrqFile(const std::string& filename, double* avg_frq, std::vector<double>* frequencies, std::vector<double>* amplitudes);

void writeFrqFile(const std::string& filename, const std::string& header_text, int samples_per_frq, double avg_frq, const std::vector<double>& frequencies, const std::vector<double>& amplitudes);
