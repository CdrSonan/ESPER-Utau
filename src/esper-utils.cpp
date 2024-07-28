#include <malloc.h>

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
    float* buffer = (float*)malloc((length - partLength) * (config.halfTripleBatchSize + 1) * sizeof(float));
    for (int i = 0; i < (length - partLength) * (config.halfTripleBatchSize + 1); i++) {
        buffer[i] = src[i + 2 * partLength * (config.halfTripleBatchSize + 1)];
    }
    for (int i = 0; i < partLength * (config.halfTripleBatchSize + 1); i++) {
        src[i + partLength * (config.halfTripleBatchSize + 1)] = src[i + length * (config.halfTripleBatchSize + 1)];
    }
}