#include <iostream>
#include <map>
#include <string>
#include <filesystem>
#include <windows.h>

#include "argparse.hpp"
#include "fileio.hpp"
#include "create-esper-structs.hpp"
#include "esper-utils.hpp"
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
    int frqReadSuccess = 1;
    double avg_frq;
    std::vector<double> frequencies;
    std::vector<double> amplitudes;
    std::string frqFilePath = args.inputPath + ".frq";
    if (iniCfg["useFrqFiles"] == "true") {
        frqReadSuccess = readFrqFile(frqFilePath, &avg_frq, &frequencies, &amplitudes);
        if (frqReadSuccess == 0)
        {
            applyFrqToSample(sample, avg_frq, frequencies, cfg);
        }
    }
    if (frqReadSuccess != 0)
    {
        pitchCalcFallback(sample, cfg);
        if ((iniCfg["createFrqFiles"] == "true" && !std::filesystem::exists(frqFilePath)) || iniCfg["overwriteFrqFiles"] == "true"){
            getFrqFromSample(sample, frequencies, amplitudes, cfg);
            writeFrqFile(frqFilePath, "Created by ESPER-Utau resampler", 256, cfg.sampleRate / sample.config.pitch, frequencies, amplitudes);
    }
    }
    specCalc(sample, cfg);
    return 0;
}