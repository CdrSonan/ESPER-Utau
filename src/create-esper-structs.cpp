#include <string>
#include <map>
#include <math.h>

#include "esper.h"

#include "create-esper-structs.hpp"

#include "esper-utils.hpp"

// Creates an engineCfg struct from the values in an ini config file
engineCfg createEngineCfg(std::map<std::string, std::string> iniCfg)
{
    engineCfg cfg;

    // fill with default values
    cfg.sampleRate = 44100;
    cfg.tickRate = 230;
    cfg.batchSize = 192;
    cfg.tripleBatchSize = 576;
    cfg.halfTripleBatchSize = 288;
    cfg.nHarmonics = 64;
    cfg.halfHarmonics = 33;
    cfg.frameSize = 355;
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
    if (iniCfg.find("nHarmonics") != iniCfg.end()) {
        cfg.nHarmonics = std::stoi(iniCfg["nHarmonics"]);
    }
    cfg.halfHarmonics = cfg.nHarmonics / 2 + 1;
    cfg.frameSize = 2 * cfg.halfHarmonics + cfg.halfTripleBatchSize + 1;
    if (iniCfg.find("breCompPremul") != iniCfg.end()) {
        cfg.breCompPremul = std::stof(iniCfg["breCompPremul"]);
    }
    return cfg;
}

// Creates a cSampleCfg struct from an engineCfg object, and an ini config file
cSampleCfg createCSampleCfg(int numSamples, engineCfg cfg, std::map<std::string, std::string> iniCfg)
{
    cSampleCfg sampleCfg;
    sampleCfg.length = numSamples;
    sampleCfg.batches = numSamples / cfg.batchSize;
    sampleCfg.pitchLength = sampleCfg.batches;
    sampleCfg.pitch = 300;
    sampleCfg.isVoiced = 1;
    sampleCfg.isPlosive = 0;
    sampleCfg.useVariance = 1;
    sampleCfg.expectedPitch = 300.;
    if (iniCfg.find("expectedPitch") != iniCfg.end()) {
		if (iniCfg["expectedPitch"] == "0" || iniCfg["expectedPitch"] == "auto" || iniCfg["expectedPitch"] == "Auto")
        {
			sampleCfg.expectedPitch = 0;
		}
        else
        {
            int midiPitch = noteToMidiPitch(iniCfg["expectedPitch"]);
            if (midiPitch > 0)
            {
                sampleCfg.expectedPitch = 440. * pow(2., (midiPitch - 69) / 12.);
            }
            else
            {
                sampleCfg.expectedPitch = std::stof(iniCfg["expectedPitch"]);
            }
        }
    }
    sampleCfg.searchRange = 0.55;
    if (iniCfg.find("pitchSearchRange") != iniCfg.end()) {
        sampleCfg.searchRange = std::stof(iniCfg["pitchSearchRange"]);
    }
    sampleCfg.tempWidth = 15;
    if (iniCfg.find("tempWidth") != iniCfg.end()) {
        sampleCfg.tempWidth = std::stoi(iniCfg["tempWidth"]);
    }
    return sampleCfg;
}

// Creates a cSample struct from a waveform, its length, engineCfg object, and an ini config file
cSample createCSample(float* wave, int numSamples, engineCfg cfg, std::map<std::string, std::string> iniCfg)
{
    cSampleCfg sampleCfg = createCSampleCfg(numSamples, cfg, iniCfg);
    cSample sample;
    sample.waveform = wave;
    sample.pitchDeltas = (int*)malloc(sampleCfg.batches * sizeof(int));
	sample.pitchMarkers = (int*)malloc(sampleCfg.length * sizeof(int));
	sample.pitchMarkerValidity = (char*)malloc(sampleCfg.length * sizeof(char));
    sample.specharm = (float*)malloc(sampleCfg.batches * cfg.frameSize * sizeof(float));
    sample.avgSpecharm = (float*)malloc((cfg.halfHarmonics + cfg.halfTripleBatchSize + 1) * sizeof(float));
    sample.config = sampleCfg;
    return sample;
}
