#include <vector>

#include "esper.h"

#pragma once

void applyFrqToSample(cSample& sample, double avg_frq, std::vector<double> frequencies, engineCfg config);

void getFrqFromSample(cSample& sample, std::vector<double>& frequencies, std::vector<double>& amplitudes, engineCfg config);

int noteToMidiPitch(std::string note);

float midiPitchToEsperPitch(float pitch, engineCfg config);
