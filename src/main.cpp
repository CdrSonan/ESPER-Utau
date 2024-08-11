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
    cSample sample;
    int espReadSuccess = 1;
    if (iniCfg["useEspFiles"] == "true")
    {
        std::string espFilePath = args.inputPath + ".esp";
        espReadSuccess = readEspFile(espFilePath, sample, cfg);
    }
    if (espReadSuccess != 0)
    {
        int numSamples;
        float* wave = readWavFile(args.inputPath, &numSamples);
        sample = createCSample(wave, numSamples, cfg, iniCfg);
        int frqReadSuccess = 1;
        double avg_frq;
        std::vector<double> frequencies;
        std::vector<double> amplitudes;
        std::string frqFilePath = args.inputPath + ".frq";
        if (iniCfg["useFrqFiles"] == "true")
        {
            frqReadSuccess = readFrqFile(frqFilePath, &avg_frq, &frequencies, &amplitudes);
            if (frqReadSuccess == 0)
            {
                applyFrqToSample(sample, avg_frq, frequencies, cfg);
            }
        }
        if (frqReadSuccess != 0)
        {
            pitchCalcFallback(&sample, cfg);
            if ((iniCfg["createFrqFiles"] == "true" && !std::filesystem::exists(frqFilePath)) || iniCfg["overwriteFrqFiles"] == "true")
            {
                getFrqFromSample(sample, frequencies, amplitudes, cfg);
                writeFrqFile(frqFilePath, "Created by ESPER-Utau resampler", 256, cfg.sampleRate / sample.config.pitch, frequencies, amplitudes);
            }
        }
        specCalc(sample, cfg);
        if ((iniCfg["createEspFiles"] == "true" && !std::filesystem::exists(args.outputPath + ".esp")) || iniCfg["overwriteEspFiles"] == "true")
        {
            writeEspFile(args.outputPath + ".esp", sample, cfg);
        }
    }

    args.length *= (float)cfg.sampleRate / (float)cfg.batchSize / 1000.f;
    args.consonant *= (float)cfg.sampleRate / (float)cfg.batchSize / 1000.f;
    args.offset *= (float)cfg.sampleRate / (float)cfg.batchSize / 1000.f;
    args.cutoff *= (float)cfg.sampleRate / (float)cfg.batchSize / 1000.f;
    if (args.cutoff < 0)
    {
        args.cutoff *= -1;
    }
    else
    {
        args.cutoff = sample.config.batches - args.offset - args.cutoff;
    }
    int esperLength = args.length + (int)args.consonant;

    float* steadinessArr = (float*)malloc(esperLength * sizeof(float));
    float* breathinessArr = (float*)malloc(esperLength * sizeof(float));//TODO: fill arrays with values from args if present
    float* formantShift = (float*)malloc(esperLength * sizeof(float));
    float steadiness = 0;
    float breathiness = 0;
    if (args.flags.find("std") != args.flags.end())
        steadiness = (float)args.flags["std"] / 100.f;
    if (args.flags.find("bre") != args.flags.end())
		breathiness = (float)args.flags["bre"] / 100.f;
    for (int i = 0; i < esperLength; i++)
	{
		steadinessArr[i] = steadiness;
		breathinessArr[i] = breathiness;
		formantShift[i] = 0;
	}

    segmentTiming timings;
    timings.start1 = 0;
    timings.start2 = 0;
    timings.start3 = 0;
    timings.end1 = args.length - 1;
    timings.end2 = args.length - 1;
    timings.end3 = args.length - 1;
    timings.windowStart = 0;
    timings.windowEnd = args.length - 1;
    timings.offset = 0;

    float* resampledSpecharm = (float*)malloc(esperLength * cfg.frameSize * sizeof(float));
    memcpy(resampledSpecharm, sample.specharm + (int)(args.offset) * cfg.frameSize, (int)(args.consonant) * cfg.frameSize * sizeof(float));
    for (int i = 0; i < (int)(args.consonant); i++)
    {
        for (unsigned int j = 0; j < cfg.halfHarmonics; j++)
        {
            resampledSpecharm[i * cfg.frameSize + j] += sample.avgSpecharm[j];
        }
        for (unsigned int j = cfg.halfHarmonics; j < cfg.frameSize; j++)
        {
            resampledSpecharm[i * cfg.frameSize + cfg.halfHarmonics + j] += sample.avgSpecharm[j];
        }
    }
    resampleSpecharm(sample.avgSpecharm, sample.specharm + (int)((args.offset) + args.consonant) * cfg.frameSize, (int)args.cutoff, steadinessArr, 0.5, 0, 0, resampledSpecharm + (int)(args.consonant) * cfg.frameSize, timings, cfg);

    float* resampledPitch = (float*)malloc(esperLength * sizeof(float));
    for (int i = 0; i < (int)(args.consonant); i++)
    {
        resampledPitch[i] = (float)(sample.pitchDeltas[(int)(args.offset) + i] - sample.config.pitch);
    }
    resamplePitch(sample.pitchDeltas + (int)((args.offset) + args.consonant), (int)args.cutoff, (float)sample.config.pitch, 0.5, 0, 0, resampledPitch + (int)(args.consonant), args.length, timings);

    float* srcPitch = (float*)malloc(esperLength * sizeof(float));
    for (int i = 0; i < (int)(esperLength); i++)
	{
		srcPitch[i] = resampledPitch[i] + sample.config.pitch;
    }
    float* tgtPitch = (float*)malloc(esperLength * sizeof(float));
    for (int i = 0; i < (int)(esperLength); i++)
    {
        float pitchBendPos = (float)i / (float)esperLength * (float)args.pitchBend.size();
        int pitchBendIndex = (int)pitchBendPos;
        float pitchBendWeight = pitchBendPos - pitchBendIndex;
        float pitchBend;
        if (pitchBendIndex >= args.pitchBend.size() - 1)
		{
			pitchBend = args.pitchBend[args.pitchBend.size() - 1];
		}
        else
        {
            pitchBend = args.pitchBend[pitchBendIndex] * (1 - pitchBendWeight) + args.pitchBend[pitchBendIndex + 1] * pitchBendWeight;
        }
        if (args.flags.find("t") != args.flags.end())
		{
            tgtPitch[i] = resampledPitch[i] + midiPitchToEsperPitch((float)args.pitch, cfg) * powf(2, pitchBend / 1200) * powf(2, (float)args.flags["t"] / 100); 
		}
        else
        {
            tgtPitch[i] = resampledPitch[i] + midiPitchToEsperPitch((float)args.pitch, cfg) * powf(2, pitchBend / 1200);
        }
	}
    std::cout << "sanity check: sample length:" << sample.config.batches << "offset:" << (int)(args.offset) << "consonant:" << (int)(args.consonant) << "cutoff:" << args.cutoff << std::endl;
    float* resampledExcitation = (float*)malloc(esperLength * (cfg.halfTripleBatchSize + 1) * 2 * sizeof(float));//TODO' fix excitation memory allocation, part passed to resampler, etc.
    float* loopExcitationBase = (float*)malloc((int)args.cutoff * (cfg.halfTripleBatchSize + 1) * 2 * sizeof(float));
    memcpy(resampledExcitation,
        sample.excitation + (int)(args.offset) * (cfg.halfTripleBatchSize + 1),
        (int)(args.consonant) * (cfg.halfTripleBatchSize + 1) * sizeof(float));
    memcpy(resampledExcitation + (int)(args.consonant) * (cfg.halfTripleBatchSize + 1),
        sample.excitation + (int)(args.offset + sample.config.batches) * (cfg.halfTripleBatchSize + 1),
        (int)(args.consonant) * (cfg.halfTripleBatchSize + 1) * sizeof(float));
    memcpy(loopExcitationBase,
        sample.excitation + (int)(args.offset + args.consonant) * (cfg.halfTripleBatchSize + 1),
		(int)args.cutoff * (cfg.halfTripleBatchSize + 1) * sizeof(float));
    memcpy(loopExcitationBase + (int)args.cutoff * (cfg.halfTripleBatchSize + 1),
		sample.excitation + (int)(args.offset + args.consonant + args.length) * (cfg.halfTripleBatchSize + 1),
		(int)args.cutoff * (cfg.halfTripleBatchSize + 1) * sizeof(float));
    resampleExcitation(loopExcitationBase, (int)args.cutoff, 0, 0, resampledExcitation + (int)(args.consonant) * (cfg.halfTripleBatchSize + 1) * 2, timings, cfg);
    fuseConsecutiveExcitation(resampledExcitation, esperLength, (int)(args.consonant), cfg);

    pitchShift(resampledSpecharm, srcPitch, tgtPitch, formantShift, breathinessArr, esperLength, cfg);

    float* resampledWave = (float*)malloc(esperLength * cfg.batchSize * sizeof(float));
    renderUnvoiced(resampledSpecharm, resampledExcitation, 1, resampledWave, esperLength, cfg);
    float phase = 0;
    renderVoiced(resampledSpecharm, resampledPitch, &phase, resampledWave, esperLength, cfg);
    writeWavFile(args.outputPath, resampledWave, cfg.sampleRate, esperLength * cfg.batchSize);
    return 0;
}