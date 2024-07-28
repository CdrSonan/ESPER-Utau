#include <iostream>
#include <map>
#include <string>
#include <windows.h>

#include "argparse.hpp"
#include "fileio.hpp"
#include "create-esper-structs.hpp"
#include "esper.h"

int main(int argc, char* argv[]) {
    resamplerArgs args = parseArguments(argc, argv);
    std::map<std::string, std::string> iniCfg;
    readIniFile(args.rsmpDir + "\\esper-config.ini", &iniCfg);
    readIniFile(args.inputPath.substr(0, args.rsmpDir.find_last_of("/\\")) + "\\resampler-config.ini", &iniCfg);
    engineCfg cfg = createEngineCfg(iniCfg);

    int numSamples;
    float* wave = readWavFile(args.inputPath, &numSamples);
    cSample sample = createCSample(wave, numSamples, cfg, iniCfg);
    pitchCalcFallback(sample, cfg);
    specCalc(sample, cfg);
    return 0;
}