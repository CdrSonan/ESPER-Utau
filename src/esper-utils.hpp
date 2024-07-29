#include <vector>

#include "esper.h"

#pragma once

float* splitExcitation(float* src, int length, int partLength, engineCfg config);

void fuseConsecutiveExcitation(float* src, int length, int partLength, engineCfg config);

void applyFrqToSample(cSample& sample, double avg_frq, std::vector<double> frequencies, engineCfg config);

void getFrqFromSample(cSample& sample, std::vector<double>& frequencies, std::vector<double>& amplitudes, engineCfg config);
