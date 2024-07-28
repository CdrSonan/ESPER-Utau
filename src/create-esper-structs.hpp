#include <string>
#include <map>
#include <math.h>

#include "esper.h"

#pragma once

engineCfg createEngineCfg(std::map<std::string, std::string> iniCfg);

cSampleCfg createCSampleCfg(int numSamples, engineCfg cfg, std::map<std::string, std::string> iniCfg);

cSample createCSample(float* wave, int numSamples, engineCfg cfg, std::map<std::string, std::string> iniCfg);
