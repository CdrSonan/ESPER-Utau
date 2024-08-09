#include <malloc.h>
#include <vector>

#include "esper.h"

#include "esper-utils.hpp"

float* splitExcitation(float* src, int length, int partLength, engineCfg config)
{
    float* buffer = (float*)malloc((length - partLength) * (config.halfTripleBatchSize + 1) * sizeof(float));
    for (int i = partLength * (config.halfTripleBatchSize + 1); i < length * (config.halfTripleBatchSize + 1); i++) {
        buffer[i - partLength * (config.halfTripleBatchSize + 1)] = src[i];
    }
    for (int i = 0; i < partLength * (config.halfTripleBatchSize + 1); i++) {
        src[i + partLength * (config.halfTripleBatchSize + 1)] = src[i + length * (config.halfTripleBatchSize + 1)];
    }
    for (int i = 0; i < (length - partLength) * (config.halfTripleBatchSize + 1); i++) {
        src[i + 2 * partLength * (config.halfTripleBatchSize + 1)] = buffer[i];
    }
    free(buffer);
    return src + partLength * (config.halfTripleBatchSize + 1);
}

void fuseConsecutiveExcitation(float* src, int length, int partLength, engineCfg config)
{
    float* buffer = (float*)malloc(partLength * (config.halfTripleBatchSize + 1) * sizeof(float));
    for (int i = 0; i < partLength * (config.halfTripleBatchSize + 1); i++) {
        buffer[i] = src[i + partLength * (config.halfTripleBatchSize + 1)];
    }
    for (int i = 0; i < (length - partLength) * (config.halfTripleBatchSize + 1); i++) {
        src[i + partLength * (config.halfTripleBatchSize + 1)] = src[i + 2 * partLength * (config.halfTripleBatchSize + 1)];
    }
    for (int i = 0; i < partLength * (config.halfTripleBatchSize + 1); i++) {
        src[i + length * (config.halfTripleBatchSize + 1)] = buffer[i];
    }
}

void applyFrqToSample(cSample& sample, double avg_frq, std::vector<double> frequencies, engineCfg config)
{
    for (int i = 0; i < frequencies.size(); i++) {
        frequencies[i] = config.sampleRate / frequencies[i];
    }
    sample.config.pitchLength = sample.config.batches;
    int srcLength = frequencies.size();
    int tgtLength = sample.config.pitchLength;
    //perform linear interpolation
    for (int i = 0; i < tgtLength; i++) {
        double tgtIndex = (double)i / tgtLength * srcLength;
        int srcIndex = (int)tgtIndex;
        double srcWeight = tgtIndex - srcIndex;
        sample.pitchDeltas[i] = (1 - srcWeight) * frequencies[srcIndex] + srcWeight * frequencies[srcIndex + 1];
    }
    sample.config.pitch = config.sampleRate / avg_frq;
}

void getFrqFromSample(cSample& sample, std::vector<double>& frequencies, std::vector<double>& amplitudes, engineCfg config)
{
    frequencies.clear();
    amplitudes.clear();
    for (int i = 0; i < sample.config.pitchLength; i++) {
        frequencies.push_back(config.sampleRate / sample.pitchDeltas[i]);
        float amplitude = 0;
        for (int j = 0; j < 256; j++) {
            if (abs(sample.waveform[i * 256 + j]) > amplitude) {
                amplitude = abs(sample.waveform[i * 256 + j]);
            }
        }
        amplitudes.push_back(amplitude);
    }
}

float midiPitchToEsperPitch(float pitch, engineCfg config)
{
	return (float)config.sampleRate / (440 * pow(2, (pitch - 69) / 12));
}