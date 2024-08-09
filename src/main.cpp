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

    int esperLength = args.length / 4 + args.consonant / 4;//TODO: adjust following length and consonant references to the correct format !!!important!!!

    float* steadinessArr = (float*)malloc(args.length / 4 * sizeof(float));
    float* breathinessArr = (float*)malloc(args.length / 4 * sizeof(float));
    segmentTiming timings;
    timings.start1 = 0;
    timings.start2 = 1;
    timings.start3 = 2;
    timings.end1 = args.length / 4 - (int)(args.consonant / 4) - 3;
    timings.end2 = args.length / 4 - (int)(args.consonant / 4) - 2;
    timings.end3 = args.length / 4 - (int)(args.consonant / 4) - 1;
    timings.windowStart = 0;
    timings.windowEnd = args.length / 4 - (int)(args.consonant / 4) - 1;
    timings.offset = 0;

    float* resampledSpecharm = (float*)malloc(args.length / 4 * cfg.frameSize * sizeof(float));
    memcpy(resampledSpecharm, sample.specharm + (int)(args.offset / 4) * cfg.frameSize, (int)(args.consonant / 4) * cfg.frameSize * sizeof(float));
    for (int i = 0; i < (int)(args.consonant / 4); i++)
    {
        for (int j = 0; j < cfg.halfHarmonics; j++)
        {
            resampledSpecharm[i * cfg.frameSize + j] += sample.avgSpecharm[j];
        }
        for (int j = cfg.halfHarmonics; j < cfg.frameSize; j++)
        {
            resampledSpecharm[i * cfg.frameSize + cfg.halfHarmonics + j] += sample.avgSpecharm[j];
        }
    }
    resampleSpecharm(sample.avgSpecharm, sample.specharm, sample.config.length, steadinessArr, 0.5, 1, 1, resampledSpecharm + (int)(args.consonant / 4) * cfg.frameSize, timings, cfg);

    float* resampledPitch = (float*)malloc(args.length / 4 * sizeof(float));
    for (int i = 0; i < (int)(args.consonant / 4); i++)
    {
        resampledPitch[i] = (float)(sample.pitchDeltas[(int)(args.offset / 4) + i] - sample.config.pitch);
    }
    resamplePitch(sample.pitchDeltas, sample.config.pitchLength, sample.config.pitch, 0.5, 1, 1, resampledPitch + (int)(args.consonant / 4), args.length / 4 - (int)(args.consonant / 4), timings);

    float* srcPitch = (float*)malloc(args.length / 4 * sizeof(float));
    for (int i = 0; i < (int)(args.length / 4); i++)
	{
		srcPitch[i] = resampledPitch[i] + sample.config.pitch;
    }
    float* tgtPitch = (float*)malloc(args.length / 4 * sizeof(float));
    for (int i = 0; i < (int)(args.length / 4); i++)
    {
        float pitchBendPos = (float)i / (args.length / 4) * args.pitchBend.size();
        int pitchBendIndex = (int)pitchBendPos;
        float pitchBendWeight = pitchBendPos - pitchBendIndex;
        float pitchBend = args.pitchBend[pitchBendIndex] * (1 - pitchBendWeight) + args.pitchBend[pitchBendIndex + 1] * pitchBendWeight;
        if (args.flags.find("t") != args.flags.end())
		{
            tgtPitch[i] = resampledPitch[i] + midiPitchToEsperPitch(args.pitch, cfg) * powf(2, pitchBend / 1200) * powf(2, args.flags["t"] / 100); 
		}
        else
        {
            tgtPitch[i] = resampledPitch[i] + midiPitchToEsperPitch(args.pitch, cfg) * powf(2, pitchBend / 1200);
        }
	}

    float* resampledExcitation = (float*)malloc(args.length / 4 * (cfg.halfTripleBatchSize + 1) * 2 * sizeof(float));
    memcpy(resampledExcitation, sample.excitation + (int)(args.offset / 4) * (cfg.halfTripleBatchSize + 1), (int)(args.consonant / 4) * (cfg.halfTripleBatchSize + 1) * sizeof(float));
    memcpy(resampledExcitation + (int)(args.consonant / 4) * (cfg.halfTripleBatchSize + 1), sample.excitation + (int)(args.offset / 4 + sample.config.length) * (cfg.halfTripleBatchSize + 1) * 2, (int)(args.consonant / 4) * (cfg.halfTripleBatchSize + 1) * sizeof(float));
    resampleExcitation(sample.excitation, sample.config.batches, 1, 1, resampledExcitation + (int)(args.consonant / 4) * (cfg.halfTripleBatchSize + 1) * 2, timings, cfg);
    fuseConsecutiveExcitation(resampledExcitation, args.length / 4, (int)(args.consonant / 4), cfg);

    //pitchShift(resampledSpecharm, ...) TODO: figure out correct srcPitch and tgtPitch
    std::cout << "test0" << std::endl; Sleep(200);
    float* resampledWave = (float*)malloc(args.length / 4 * cfg.batchSize * sizeof(float));
    renderUnvoiced(resampledSpecharm, resampledExcitation, 1, resampledWave, args.length / 4, cfg);
    float phase = 0;
    renderVoiced(resampledSpecharm, resampledPitch, &phase, resampledWave, args.length / 4, cfg);

    return 0;
}