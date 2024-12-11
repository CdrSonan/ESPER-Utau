#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <random>
#include <filesystem>

#include "argparse.hpp"
#include "fileio.hpp"
#include "create-esper-structs.hpp"
#include "esper-utils.hpp"
#include "esper.h"

int main(int argc, char* argv[]) {
    //declare ESP file standard of the current engine version
	unsigned int espFileStd = 5;
	//parse command line arguments
    resamplerArgs args = parseArguments(argc, argv);

	//read configuration files in order of priority
    std::map<std::string, std::string> iniCfg;
    readIniFile(args.rsmpDir + "\\esper-config.ini", &iniCfg);
    readIniFile(args.inputPath.substr(0, args.rsmpDir.find_last_of("/\\")) + "\\resampler-config.ini", &iniCfg);
    engineCfg cfg = createEngineCfg(iniCfg);

    //create sample object
    cSample sample;

	//read ESP file if necessary and it exists
    int espReadSuccess = 1;
    if (iniCfg["useEsperFiles"] == "true")
    {
        std::string espFilePath = args.inputPath + ".esp";
        espReadSuccess = readEspFile(espFilePath, sample, espFileStd, cfg);
    }

	if (espReadSuccess != 0) //no ESP file found or reading failed -> perform analysis to get the same data
    {
		//read WAV file
        int numSamples;
        float* wave = readWavFile(args.inputPath, &numSamples);

		//load into sample object
        sample = createCSample(wave, numSamples, cfg, iniCfg);

		//attempt to read .frq file
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

		//regardless of whether the .frq file was read, perform pitch calculation to get pitch markers
		//if an .frq file was read, only its average pitch will be used as the expected pitch, all other data is discarded
		pitchCalcFallback(&sample, cfg);
		sample.pitchMarkers = (int*)realloc(sample.pitchMarkers, sample.config.markerLength * sizeof(int));

		//write pitch data to a new .frq file if necessary
        if (frqReadSuccess != 0)
        {
            if ((iniCfg["createFrqFiles"] == "true" && !std::filesystem::exists(frqFilePath)) || iniCfg["overwriteFrqFiles"] == "true")
            {
                getFrqFromSample(sample, frequencies, amplitudes, cfg);
                writeFrqFile(frqFilePath, "FREQ0003", 256, cfg.sampleRate / sample.config.pitch, frequencies, amplitudes);
            }
        }

		//perform spectral analysis
		specCalc(sample, cfg);

		//write sample data to a new ESP file if necessary
		if ((iniCfg["createEsperFiles"] == "true" && !std::filesystem::exists(args.inputPath + ".esp")) || iniCfg["overwriteEsperFiles"] == "true")
        {
            writeEspFile(args.inputPath + ".esp", sample, espFileStd, cfg);
        }
    }


	//analysis complete, now perform resampling

    //convert milliseconds to ESPER frames
	args.length *= (float)cfg.sampleRate / (float)cfg.batchSize / 1000.f;
    args.consonant *= (float)cfg.sampleRate / (float)cfg.batchSize / 1000.f;
    args.offset *= (float)cfg.sampleRate / (float)cfg.batchSize / 1000.f;
    args.cutoff *= (float)cfg.sampleRate / (float)cfg.batchSize / 1000.f;

	//handle negative cutoff values
    if (args.cutoff < 0)
    {
        args.cutoff *= -1;
    }
    else
    {
        args.cutoff = sample.config.batches - args.offset - args.cutoff;
    }

    //read resampling flags
    float loopOffset = 0.f;
    if (args.flags.find("loff") != args.flags.end())
    {
        loopOffset = (float)args.flags["loff"] / 100.f;
    }
	args.cutoff -= args.consonant;
    loopOffset *= args.cutoff / 4.f;
	args.cutoff -= 2. * loopOffset;
	args.consonant += loopOffset;
    int esperLength = args.length;// +(int)args.consonant;
    if (esperLength <= (int)args.consonant + 1)
	{
		esperLength = (int)args.consonant + 2;
	}
	args.length = esperLength - (int)args.consonant;

    //create arrays for curve parameters given as flags
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

    //create generic timing object
    segmentTiming timings;
    timings.start1 = 0;
    timings.start2 = 0;
    timings.start3 = 0;
    if (args.length > 40)
    {
        timings.end1 = args.length - 21;
        timings.end2 = args.length - 11;
        timings.end3 = args.length - 1;
    }
    else
	{
		timings.end1 = args.length - 2;
		timings.end2 = args.length - 1;
		timings.end3 = args.length;
	}
    timings.windowStart = 0;
    timings.windowEnd = args.length;
    timings.offset = 0;

	//resample specharm
    float* resampledSpecharm = (float*)malloc(esperLength * cfg.frameSize * sizeof(float));

    //copy consonant part wwithout resampling
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

    //calculate correct average of vowel part for resampling
	float* effAvgSpecharm = (float*)malloc((cfg.halfHarmonics + cfg.halfTripleBatchSize + 1) * sizeof(float));
	for (int i = 0; i < cfg.halfHarmonics + cfg.halfTripleBatchSize + 1; i++)
	{
		effAvgSpecharm[i] = 0.;
	}
	float* effSpecharm = (float*)malloc((int)(args.cutoff) * cfg.frameSize * sizeof(float));
	for (int i = 0; i < (int)(args.cutoff); i++)
	{
		for (unsigned int j = 0; j < cfg.halfHarmonics; j++)
		{
			effSpecharm[i * cfg.frameSize + j] = sample.specharm[((int)(args.offset + args.consonant) + i) * cfg.frameSize + j] + sample.avgSpecharm[j];
			if (i < args.length)
			{
				effAvgSpecharm[j] += effSpecharm[i * cfg.frameSize + j];
			}
		}
        for (unsigned int j = 0; j < cfg.halfHarmonics; j++)
        {
            effSpecharm[i * cfg.frameSize + cfg.halfHarmonics + j] = sample.specharm[((int)(args.offset + args.consonant) + i) * cfg.frameSize + cfg.halfHarmonics + j];
        }
		for (unsigned int j = 0; j < cfg.halfTripleBatchSize + 1; j++)
		{
			effSpecharm[i * cfg.frameSize + 2 * cfg.halfHarmonics + j] = sample.specharm[((int)(args.offset + args.consonant) + i) * cfg.frameSize + 2 * cfg.halfHarmonics + j] + sample.avgSpecharm[cfg.halfHarmonics + j];
			if (i < args.length)
			{
				effAvgSpecharm[cfg.halfHarmonics + j] += effSpecharm[i * cfg.frameSize + 2 * cfg.halfHarmonics + j];
			}
		}
	}

    int loopLength = (int)(args.cutoff);
    if (loopLength > args.length)
    {
        loopLength = args.length;
    }
    for (int i = 0; i < cfg.halfHarmonics + cfg.halfTripleBatchSize + 1; i++)
    {
        effAvgSpecharm[i] /= loopLength;
    }
	float totalAmplitude = 0.;
	for (int i = 0; i < cfg.halfHarmonics; i++)
	{
		totalAmplitude += effAvgSpecharm[i];
	}
    
    
    float targetAmplitude = std::stof(iniCfg["targetAmplitude"]);
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

    //loop overlap flag
	float loopOverlap = 0.5;
	if (args.flags.find("lovl") != args.flags.end())
	{
		loopOverlap = (float)args.flags["lovl"] / 101.f;
	}

	//resample vowel part
    resampleSpecharm(effAvgSpecharm, effSpecharm, (int)args.cutoff, steadinessArr, loopOverlap, 0, 1, resampledSpecharm + (int)(args.consonant) * cfg.frameSize, timings, cfg);

    if (targetAmplitude > 0 && totalAmplitude > 0)
    {
        for (int i = 0; i < esperLength; i++)
        {
            for (int j = 0; j < cfg.halfHarmonics; j++)
            {
                resampledSpecharm[i * cfg.frameSize + j] *= targetAmplitude / totalAmplitude;
            }
            for (int j = cfg.nHarmonics + 2; j < cfg.frameSize; j++)
            {
                resampledSpecharm[i * cfg.frameSize + j] *= targetAmplitude / totalAmplitude;
            }
        }
    }
    
    //resample pitch
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

	//modify pitch with stability flag
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

    //derive source, target and modified target pitch arrays for pitch shifting
    //modified target pitch array is used by the pitch shift function, target pitch array is used for rendering
    float* srcPitch = (float*)malloc(esperLength * sizeof(float));
    for (int i = 0; i < (int)(esperLength); i++)
    {
        srcPitch[i] = resampledPitch[i] + meanPitch;
    }
    float* tgtPitch = (float*)malloc(esperLength * sizeof(float));
	float* tgtPitchMod = (float*)malloc(esperLength * sizeof(float));
    float pitchDeviation = 0;
	float timeMultiplier = 1.6 * args.tempo * cfg.batchSize / cfg.sampleRate;
    for (int i = 0; i < (int)(esperLength); i++)
    {
        float pitchBendPos = (float)i * timeMultiplier;
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
		if (tgtPitch[i] < 10.)
		{
            tgtPitch[i] = 10.;
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

	//apply pitch shift
    pitchShift(resampledSpecharm, srcPitch, tgtPitchMod, formantShiftArr, breathinessArr, esperLength, cfg);

    //apply other effects
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
        applyBreathiness(resampledSpecharm, breathinessArr, esperLength, cfg);
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

    //final rendering
    float* resampledWave = (float*)malloc(esperLength * cfg.batchSize * sizeof(float));
	for (int i = 0; i < esperLength * cfg.batchSize; i++)
	{
		resampledWave[i] = 0;
	}
    phase = 0;
    render(resampledSpecharm, tgtPitch, &phase, resampledWave, esperLength, cfg);

	//write output to file
    writeWavFile(args.outputPath, resampledWave, cfg.sampleRate, esperLength * cfg.batchSize);

    return 0;
}
