#include <iostream>
#include <map>
#include <string>
#include <random>
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
    if (iniCfg["useEsperFiles"] == "true")
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
        std::string frqFilePath = args.inputPath.substr(0, args.inputPath.find_last_of(".")) + "_wav.frq";
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
                writeFrqFile(frqFilePath, "ESP-Utau", 256, cfg.sampleRate / sample.config.pitch, frequencies, amplitudes);
            }
        }
        specCalc(sample, cfg);
        if ((iniCfg["createEsperFiles"] == "true" && !std::filesystem::exists(args.inputPath + ".esp")) || iniCfg["overwriteEsperFiles"] == "true")
        {
            writeEspFile(args.inputPath + ".esp", sample, cfg);
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
    float loopOffset = 0.f;
    if (args.flags.find("loff") != args.flags.end())
    {
        loopOffset = (float)args.flags["loff"] / 100.f;
    }
	args.cutoff -= args.consonant;
    loopOffset *= args.cutoff / 4.f;
	args.cutoff -= 2. * loopOffset;
	args.consonant += loopOffset;
    int esperLength = args.length + (int)args.consonant;

    float* steadinessArr = (float*)malloc(esperLength * sizeof(float));
    float* breathinessArr = (float*)malloc(esperLength * sizeof(float));
    float* formantShiftArr = (float*)malloc(esperLength * sizeof(float));
    float steadiness = 0;
    float breathiness = 0;
	float formantShift = 0;
    if (args.flags.find("std") != args.flags.end())
        steadiness = (float)args.flags["std"] / 100.f;
    if (args.flags.find("bre") != args.flags.end())
        breathiness = (float)args.flags["bre"] / 101.f;
	if (args.flags.find("int") != args.flags.end())
		formantShift = (float)args.flags["int"] / 200.f;
    for (int i = 0; i < esperLength; i++)
    {
        steadinessArr[i] = steadiness;
        breathinessArr[i] = breathiness;
		formantShiftArr[i] = formantShift;
    }

    segmentTiming timings;
    timings.start1 = 0;
    timings.start2 = 0;
    timings.start3 = 0;
    if (args.length > 22)
    {
        timings.end1 = args.length - 21;
        timings.end2 = args.length - 11;
        timings.end3 = args.length - 1;
    }
    else
	{
		timings.end1 = args.length - 3;
		timings.end2 = args.length - 2;
		timings.end3 = args.length - 1;
	}
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
        for (unsigned int j = cfg.halfHarmonics; j < cfg.halfHarmonics + cfg.halfTripleBatchSize + 1; j++)
        {
            resampledSpecharm[i * cfg.frameSize + cfg.halfHarmonics + j] += sample.avgSpecharm[j];
        }
    }

	float* effAvgSpecharm = (float*)malloc((cfg.halfHarmonics + cfg.halfTripleBatchSize + 1) * sizeof(float));
	for (int i = 0; i < cfg.halfHarmonics + cfg.halfTripleBatchSize + 1; i++)
	{
		effAvgSpecharm[i] = 0.;
	}
	float* effSpecharm = (float*)malloc((int)args.cutoff * cfg.frameSize * sizeof(float));
	for (int i = 0; i < (int)(args.cutoff); i++)
	{
		for (unsigned int j = 0; j < cfg.halfHarmonics; j++)
		{
			effSpecharm[i * cfg.frameSize + j] = sample.specharm[((int)(args.offset + args.consonant) + i) * cfg.frameSize + j] + sample.avgSpecharm[j];
			effAvgSpecharm[j] += effSpecharm[i * cfg.frameSize + j];
		}
        for (unsigned int j = 0; j < cfg.halfHarmonics; j++)
        {
            effSpecharm[i * cfg.frameSize + cfg.halfHarmonics + j] = sample.specharm[((int)(args.offset + args.consonant) + i) * cfg.frameSize + cfg.halfHarmonics + j];
        }
		for (unsigned int j = 0; j < cfg.halfTripleBatchSize + 1; j++)
		{
			effSpecharm[i * cfg.frameSize + 2 * cfg.halfHarmonics + j] = sample.specharm[((int)(args.offset + args.consonant) + i) * cfg.frameSize + 2 * cfg.halfHarmonics + j] + sample.avgSpecharm[cfg.halfHarmonics + j];
			effAvgSpecharm[cfg.halfHarmonics + j] += effSpecharm[i * cfg.frameSize + 2 * cfg.halfHarmonics + j];
		}
	}
	for (int i = 0; i < cfg.halfHarmonics + cfg.halfTripleBatchSize + 1; i++)
	{
		effAvgSpecharm[i] /= (int)args.cutoff;
	}
	for (int i = 0; i < (int)(args.cutoff); i++)
	{
		for (unsigned int j = 0; j < cfg.halfHarmonics; j++)
		{
			effSpecharm[i * cfg.frameSize + j] -= effAvgSpecharm[j];
		}
		for (unsigned int j = 0; j < cfg.halfTripleBatchSize + 1; j++)
		{
			effSpecharm[i * cfg.frameSize + 2 * cfg.halfHarmonics + j] -= effAvgSpecharm[cfg.halfHarmonics + j];
		}
	}

	float loopOverlap = 0.5;
	if (args.flags.find("lovl") != args.flags.end())
	{
		loopOverlap = (float)args.flags["lovl"] / 101.f;
	}

    resampleSpecharm(effAvgSpecharm, effSpecharm, (int)args.cutoff, steadinessArr, loopOverlap, 0, 1, resampledSpecharm + (int)(args.consonant) * cfg.frameSize, timings, cfg);

	for (int i = sample.config.pitchLength; i < sample.config.batches; i++)
	{
		sample.pitchDeltas[i] = sample.pitchDeltas[sample.config.pitchLength - 1];
	}
	float meanPitch = 0;
	for (int i = (int)((args.offset) + args.consonant); i < (int)((args.offset) + args.consonant) + (int)args.cutoff; i++)
	{
		meanPitch += sample.pitchDeltas[i];
	}
	meanPitch /= (int)args.cutoff;
    float* resampledPitch = (float*)malloc(esperLength * sizeof(float));
    for (int i = 0; i < (int)(args.consonant); i++)
    {
        resampledPitch[i] = (float)(sample.pitchDeltas[(int)(args.offset) + i] - meanPitch);
    }
    resamplePitch(sample.pitchDeltas + (int)((args.offset) + args.consonant), (int)args.cutoff, meanPitch, loopOverlap, 0, 1, resampledPitch + (int)(args.consonant), args.length, timings);
    if (args.flags.find("pstb") != args.flags.end())
    {
        if (args.flags["pstb"] > 0)
        {
            for (int i = 0; i < esperLength; i++)
            {
				resampledPitch[i] *= 1.f - args.flags["pstb"] / 100.f;
            }
        }
    }
    float* srcPitch = (float*)malloc(esperLength * sizeof(float));
    for (int i = 0; i < (int)(esperLength); i++)
    {
        srcPitch[i] = resampledPitch[i] + meanPitch;
    }
    float* tgtPitch = (float*)malloc(esperLength * sizeof(float));
	float* tgtPitchMod = (float*)malloc(esperLength * sizeof(float));
    float pitchDeviation = 0;
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
        if (args.flags.find("pstb") != args.flags.end())
        {
            if (args.flags["pstb"] < 0)
            {
                float randNum = 0;
                for (int j = 0; j < 12; j++)
                {
                    randNum += (double)rand() / (double)RAND_MAX - 0.5;
                }
                pitchDeviation += randNum;
                pitchDeviation /= 1.1;
                tgtPitch[i] -= pitchDeviation * (float)args.flags["pstb"] / 100.;
            }
        }
		tgtPitchMod[i] = tgtPitch[i];
        if (args.flags.find("gen") != args.flags.end())
        {
			float flag = (float)args.flags["gen"] / 100.;
			if (flag > 0)
			{
				tgtPitchMod[i] *= 1. + flag;
			}
			else
			{
				tgtPitchMod[i] /= 1. - flag;
			}
        }
    }
    float* resampledExcitation = (float*)malloc(esperLength * (cfg.halfTripleBatchSize + 1) * 2 * sizeof(float));
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
    resampleExcitation(loopExcitationBase, (int)args.cutoff, 0, 1, resampledExcitation + (int)(args.consonant) * (cfg.halfTripleBatchSize + 1) * 2, timings, cfg);
    fuseConsecutiveExcitation(resampledExcitation, esperLength, (int)(args.consonant), cfg);

    pitchShift(resampledSpecharm, srcPitch, tgtPitchMod, formantShiftArr, breathinessArr, esperLength, cfg);

    float subharmonics = 1.f;
    if (args.flags.find("subh") != args.flags.end())
    {
        subharmonics = powf(1.f + (float)args.flags["subh"] / 100.f, 2.f);
    }
    for (int i = 0; i < esperLength; i++)
    {
        resampledSpecharm[i * cfg.frameSize] *= subharmonics;
    }
    float* paramArr = (float*)malloc(esperLength * sizeof(float));
    if (args.flags.find("bre") != args.flags.end())
    {
        applyBreathiness(resampledSpecharm, resampledExcitation, breathinessArr, esperLength, cfg);
	}
	if (args.flags.find("bri") != args.flags.end())
	{
		for (int i = 0; i < esperLength; i++)
		{
			paramArr[i] = (float)args.flags["bri"] / 100.f;
		}
        applyBrightness(resampledSpecharm, paramArr, esperLength, cfg);
	}
	if (args.flags.find("dyn") != args.flags.end())
	{
		for (int i = 0; i < esperLength; i++)
		{
			paramArr[i] = (float)args.flags["dyn"] / 100.f;
		}
		applyDynamics(resampledSpecharm, paramArr, tgtPitch, esperLength, cfg);
	}
    float phase = 0;
	if (args.flags.find("grwl") != args.flags.end())
	{
		for (int i = 0; i < esperLength; i++)
		{
			paramArr[i] = (float)args.flags["grwl"] / 100.f;
		}
        applyGrowl(resampledSpecharm, paramArr, &phase, esperLength, cfg);
	}
	if (args.flags.find("rgh") != args.flags.end())
	{
		for (int i = 0; i < esperLength; i++)
		{
			paramArr[i] = (float)args.flags["rgh"] / 100.f;
		}
        applyRoughness(resampledSpecharm, paramArr, esperLength, cfg);
	}

	if (args.flags.find("p") != args.flags.end())
	{
        float max = 0.f;
        for (int i = 0; i < esperLength; i++)
        {
            for (int j = 0; j < cfg.halfHarmonics; j++)
            {
                if (resampledSpecharm[i * cfg.frameSize + j] > max)
                {
                    max = resampledSpecharm[i * cfg.frameSize + j];
                }
            }
            for (int j = cfg.nHarmonics + 2; j < cfg.frameSize; j++)
            {
                if (resampledSpecharm[i * cfg.frameSize + j] > max)
                {
                    max = resampledSpecharm[i * cfg.frameSize + j];
                }
            }
        }
		max *= 1.f + 100.f * powf(1.f - (float)args.flags["p"] / 100, 4.f) * 2.f;
        for (int i = 0; i < esperLength; i++)
        {
            float localMax = 0.f;
            for (int j = 0; j < cfg.halfHarmonics; j++)
            {
                if (resampledSpecharm[i * cfg.frameSize + j] > localMax)
                {
                    localMax = resampledSpecharm[i * cfg.frameSize + j];
                }
            }
            for (int j = cfg.nHarmonics + 2; j < cfg.frameSize; j++)
            {
                if (resampledSpecharm[i * cfg.frameSize + j] > localMax)
                {
                    localMax = resampledSpecharm[i * cfg.frameSize + j];
                }
            }
            for (int j = 0; j < cfg.halfHarmonics; j++)
            {
				resampledSpecharm[i * cfg.frameSize + j] *= sin(3.14159 * localMax / max) * max;
            }
            for (int j = cfg.nHarmonics + 2; j < cfg.frameSize; j++)
            {
                resampledSpecharm[i * cfg.frameSize + j] *= sin(3.14159 * localMax / max) * max;
            }
		}
	}

    float* resampledWave = (float*)malloc(esperLength * cfg.batchSize * sizeof(float));
    renderUnvoiced(resampledSpecharm, resampledExcitation, 0, resampledWave, esperLength, cfg);
    phase = 0;
    renderVoiced(resampledSpecharm, tgtPitch, &phase, resampledWave, esperLength, cfg);
    writeWavFile(args.outputPath, resampledWave, cfg.sampleRate, esperLength * cfg.batchSize);
    return 0;
}
