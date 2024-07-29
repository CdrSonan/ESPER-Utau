#include <string>
#include <map>
#include <math.h>

#include "esper.h"

#include "create-esper-structs.hpp"

engineCfg createEngineCfg(std::map<std::string, std::string> iniCfg)
{
    engineCfg cfg;

    // fill with default values
    cfg.sampleRate = 44100;
    cfg.tickRate = 230;
    cfg.batchSize = 192;
    cfg.tripleBatchSize = 576;
    cfg.halfTripleBatchSize = 288;
    cfg.filterBSMult = 4;
    cfg.DIOBias = 0.4;
    cfg.DIOBias2 = 0.2;
    cfg.DIOTolerance = 0.2;
    cfg.DIOLastWinTolerance = 0.9;
    cfg.filterTEEMult = 32;
    cfg.filterHRSSMult = 4;
    cfg.nHarmonics = 64;
    cfg.halfHarmonics = 33;
    cfg.frameSize = 355;
    cfg.ampContThreshold = 10;
    cfg.spectralRolloff1 = 144;
    cfg.spectralRolloff2 = 192;
    cfg.breCompPremul = 0.05;

    // overwrite with values from ini file if present
    if (iniCfg.find("sampleRate") != iniCfg.end()) {
        cfg.sampleRate = std::stoi(iniCfg["sampleRate"]);
    }
    if (iniCfg.find("tickRate") != iniCfg.end()) {
        cfg.tickRate = std::stoi(iniCfg["tickRate"]);
    }
    if (iniCfg.find("batchSize") != iniCfg.end()) {
        cfg.batchSize = std::stoi(iniCfg["batchSize"]);
    }
    cfg.tripleBatchSize = cfg.batchSize * 3;
    cfg.halfTripleBatchSize = cfg.tripleBatchSize / 2;
    if (iniCfg.find("filterBSMult") != iniCfg.end()) {
        cfg.filterBSMult = std::stoi(iniCfg["filterBSMult"]);
    }
    if (iniCfg.find("DIOBias") != iniCfg.end()) {
        cfg.DIOBias = std::stof(iniCfg["DIOBias"]);
    }
    if (iniCfg.find("DIOBias2") != iniCfg.end()) {
        cfg.DIOBias2 = std::stof(iniCfg["DIOBias2"]);
    }
    if (iniCfg.find("DIOTolerance") != iniCfg.end()) {
        cfg.DIOTolerance = std::stof(iniCfg["DIOTolerance"]);
    }
    if (iniCfg.find("DIOLastWinTolerance") != iniCfg.end()) {
        cfg.DIOLastWinTolerance = std::stof(iniCfg["DIOLastWinTolerance"]);
    }
    if (iniCfg.find("filterTEEMult") != iniCfg.end()) {
        cfg.filterTEEMult = std::stoi(iniCfg["filterTEEMult"]);
    }
    if (iniCfg.find("filterHRSSMult") != iniCfg.end()) {
        cfg.filterHRSSMult = std::stoi(iniCfg["filterHRSSMult"]);
    }
    if (iniCfg.find("nHarmonics") != iniCfg.end()) {
        cfg.nHarmonics = std::stoi(iniCfg["nHarmonics"]);
    }
    cfg.halfHarmonics = cfg.nHarmonics / 2 + 1;
    cfg.frameSize = 2 * cfg.halfHarmonics + cfg.halfTripleBatchSize + 1;
    if (iniCfg.find("ampContThreshold") != iniCfg.end()) {
        cfg.ampContThreshold = std::stoi(iniCfg["ampContThreshold"]);
    }
    if (iniCfg.find("spectralRolloff1") != iniCfg.end()) {
        cfg.spectralRolloff1 = std::stoi(iniCfg["spectralRolloff1"]);
    }
    if (iniCfg.find("spectralRolloff2") != iniCfg.end()) {
        cfg.spectralRolloff2 = std::stoi(iniCfg["spectralRolloff2"]);
    }
    if (iniCfg.find("breCompPremul") != iniCfg.end()) {
        cfg.breCompPremul = std::stof(iniCfg["breCompPremul"]);
    }
    return cfg;
}

cSampleCfg createCSampleCfg(int numSamples, engineCfg cfg, std::map<std::string, std::string> iniCfg)
{
    cSampleCfg sampleCfg;
    sampleCfg.length = numSamples;
    sampleCfg.batches = floor(numSamples / cfg.batchSize);
    sampleCfg.pitchLength = sampleCfg.batches;
    sampleCfg.pitch = 300;
    sampleCfg.isVoiced = 1;
    sampleCfg.isPlosive = 0;
    sampleCfg.useVariance = 0;
    sampleCfg.expectedPitch = 300.;
    if (iniCfg.find("expectedPitch") != iniCfg.end()) {
        sampleCfg.expectedPitch = std::stof(iniCfg["expectedPitch"]);
    }
    sampleCfg.searchRange = 0.55;
    if (iniCfg.find("pitchSearchRange") != iniCfg.end()) {
        sampleCfg.searchRange = std::stof(iniCfg["pitchSearchRange"]);
    }
    sampleCfg.voicedThrh = 1.;
    if (iniCfg.find("voicedThreshold") != iniCfg.end()) {
        sampleCfg.voicedThrh = std::stof(iniCfg["voicedThreshold"]);
    }
    sampleCfg.specWidth = 2;
    if (iniCfg.find("specWidth") != iniCfg.end()) {
        sampleCfg.specWidth = std::stoi(iniCfg["specWidth"]);
    }
    sampleCfg.specDepth = 30;
    if (iniCfg.find("specDepth") != iniCfg.end()) {
        sampleCfg.specDepth = std::stoi(iniCfg["specDepth"]);
    }
    sampleCfg.tempWidth = 2;
    if (iniCfg.find("tempWidth") != iniCfg.end()) {
        sampleCfg.tempWidth = std::stoi(iniCfg["tempWidth"]);
    }
    sampleCfg.tempDepth = 10;
    if (iniCfg.find("tempDepth") != iniCfg.end()) {
        sampleCfg.tempDepth = std::stoi(iniCfg["tempDepth"]);
    }
    return sampleCfg;
}

cSample createCSample(float* wave, int numSamples, engineCfg cfg, std::map<std::string, std::string> iniCfg)
{
    cSampleCfg sampleCfg = createCSampleCfg(numSamples, cfg, iniCfg);
    cSample sample;
    sample.waveform = wave;
    sample.pitchDeltas = (int*)malloc(sampleCfg.batches * sizeof(float));
    sample.specharm = (float*)malloc(sampleCfg.batches * cfg.frameSize * sizeof(float));
    sample.avgSpecharm = (float*)malloc((cfg.halfHarmonics + cfg.halfTripleBatchSize + 1) * sizeof(float));
    sample.excitation = (float*)malloc(sampleCfg.batches * (cfg.halfTripleBatchSize + 1) * 2 * sizeof(float));
    sample.config = sampleCfg;
    return sample;
}
