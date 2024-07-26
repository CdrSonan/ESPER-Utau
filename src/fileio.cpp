#include <iostream>

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