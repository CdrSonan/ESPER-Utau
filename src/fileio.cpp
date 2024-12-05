#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <filesystem>
#include <vector>
#include <iomanip>
#include <cstring>

#include "AudioFile.h"
#include "esper.h"

#include "fileio.hpp"

//Read a WAV file and return the samples as a float array
//If the file contains multiple channels, only the first channel is read
float* readWavFile(const std::string& path, int* numSamples) {
    AudioFile<float> audioFile;
    audioFile.load(path);
    *numSamples = audioFile.getNumSamplesPerChannel();
    float* samples = new float[*numSamples];
    for (int i = 0; i < *numSamples; i++) {
        samples[i] = audioFile.samples[0][i];
    }
    return samples;
}

//Write a float array to a mono WAV file
void writeWavFile(const std::string& path, float* samples, int sampleRate, int numSamples) {
    std::vector<std::vector<float>> sampleVec;
    sampleVec.push_back(std::vector<float>(samples, samples + numSamples));
    AudioFile<float> audioFile;
    audioFile.setSampleRate(sampleRate);
    audioFile.setAudioBuffer(sampleVec);
    audioFile.save(path);
}

//Read an INI file and store the key-value pairs in a map
//Sections are ignored, so if a key is present in multiple sections, only thelast occurence will be saved
void readIniFile(const std::string& path, std::map<std::string, std::string>* iniMap) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        // Ignore comments, section headers and empty lines
        if (line.empty() || line[0] == ';' || line[0] == '[' || line[0] == '#') {
            continue;
        }
        // Find the position of the equals sign
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            // Extract the key and value
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            // Trim whitespace from key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            // Insert into the map
            (*iniMap)[key] = value;
        }
    }
}

//Read a .frq file following the UTAU standard and store the data in the provided frequency and amplitude vectors, and the avg_frq variable
int readFrqFile(const std::string& filename, double* avg_frq, std::vector<double>* frequencies, std::vector<double>* amplitudes) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Unable to open .frq file: " << filename << std::endl;
        return 1;
    }

    // Header
    char header_text[9] = {0};
    file.read(header_text, 8);
    std::cout << "Found .frq file. Header: " << header_text << std::endl;

    // Samples per frq value. Should always be 256.
    int samples_per_frq;
    file.read(reinterpret_cast<char*>(&samples_per_frq), sizeof(samples_per_frq));
    std::cout << "Samples per frq: " << samples_per_frq << std::endl;
    if (samples_per_frq != 256) {
        std::cerr << "Invalid samples per frq value: " << samples_per_frq << std::endl;
        return 1;
    }

    // Average frequency.
    file.read(reinterpret_cast<char*>(avg_frq), sizeof(avg_frq));

    // Empty space.
    file.seekg(16, std::ios::cur);

    // Number of chunks.
    int num_chunks;
    file.read(reinterpret_cast<char*>(&num_chunks), sizeof(num_chunks));

	// Frequency and amplitude pairs.
    double frequency, amplitude;
    for (int i = 0; i < num_chunks; ++i) {
        
        file.read(reinterpret_cast<char*>(&frequency), sizeof(frequency));
        file.read(reinterpret_cast<char*>(&amplitude), sizeof(amplitude));
		if (frequency <= 0 || frequency > 12000) {
			frequencies->push_back(*avg_frq);
		}
		else
        {
            frequencies->push_back(frequency);
		}
        amplitudes->push_back(amplitude);
    }

    std::cout << "Successfully parsed frq file!" << std::endl;
    return 0;
}

//Write pitch and amplitude data to a .frq file.
//samples_per_frq must be 256 to match the UTAU standard, and the header text must be exactly 8 characters long.
void writeFrqFile(const std::string& filename, const std::string& header_text, int samples_per_frq, double avg_frq, const std::vector<double>& frequencies, const std::vector<double>& amplitudes) {
    if (frequencies.size() != amplitudes.size()) {
        std::cerr << "Frequencies and amplitudes vectors must be of the same size." << std::endl;
        return;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return;
    }

    // Write header
    file.write(header_text.c_str(), 8);

    // Write samples per frq value
    file.write(reinterpret_cast<const char*>(&samples_per_frq), sizeof(samples_per_frq));

    // Write average frequency
    file.write(reinterpret_cast<const char*>(&avg_frq), sizeof(avg_frq));

    // Write empty space
    char empty_space[16] = {0};
    file.write(empty_space, 16);

    // Write number of chunks
    int num_chunks = frequencies.size();
    file.write(reinterpret_cast<const char*>(&num_chunks), sizeof(num_chunks));

    // Write frequency and amplitude pairs
    for (int i = 0; i < num_chunks; ++i) {
        file.write(reinterpret_cast<const char*>(&frequencies[i]), sizeof(frequencies[i]));
        file.write(reinterpret_cast<const char*>(&amplitudes[i]), sizeof(amplitudes[i]));
    }

    std::cout << "Successfully wrote frq file!" << std::endl;
}

//Struct representing the header of an ESP file. Contains the required metadata to read the file and verify its compatibility with the current engine version and configuration.
struct espFileHeader
{
    unsigned int filestd;
    unsigned int sampleRate;
    unsigned short tickRate;
    unsigned int batchSize;
    unsigned int nHarmonics;
    unsigned int batches;
    unsigned int pitchLength;
	unsigned int markerLength;
    unsigned int pitch;
    int isVoiced;
    int isPlosive;
};

//Read an ESP file and store the data in the provided cSample object
//the filestd parameter is used to verify the compatibility of the file with the current engine version
int readEspFile(std::string path, cSample& sample, unsigned int filestd, engineCfg config)
{
    //check if the file exists
    if (!std::filesystem::exists(path))
    {
        std::cerr << "ESP File not found: " << path << std::endl;
        return 1;
    }
    FILE* file = fopen(path.c_str(), "rb");
    espFileHeader header;
    fread(&header, sizeof(espFileHeader), 1, file);
    
    // check if the file is valid under the current engine configuration
	if (header.filestd != filestd ||
        header.sampleRate != config.sampleRate ||
        header.tickRate != config.tickRate ||
        header.batchSize != config.batchSize ||
        header.nHarmonics != config.nHarmonics)
    {
        std::cerr << "Invalid ESP file for current config: " << path << std::endl;
        fclose(file);
        return 1;
    }
    sample.config.batches = header.batches;
    sample.config.pitchLength = header.pitchLength;
	sample.config.markerLength = header.markerLength;
    sample.config.pitch = header.pitch;
    sample.config.isVoiced = header.isVoiced;
    sample.config.isPlosive = header.isPlosive;
    sample.pitchDeltas = (int*)malloc(sample.config.pitchLength * sizeof(int));
	sample.pitchMarkers = (int*)malloc(sample.config.markerLength * sizeof(int));
    sample.specharm = (float*)malloc(sample.config.batches * config.frameSize * sizeof(float));
    sample.avgSpecharm = (float*)malloc((config.halfHarmonics + config.halfTripleBatchSize + 1) * sizeof(float));
    fread(sample.pitchDeltas, sizeof(int), sample.config.pitchLength, file);
	fread(sample.pitchMarkers, sizeof(int), sample.config.markerLength, file);
    fread(sample.specharm, sizeof(float), sample.config.batches * config.frameSize, file);
    fread(sample.avgSpecharm, sizeof(float), config.halfHarmonics + config.halfTripleBatchSize + 1, file);
    fclose(file);
    return 0;
}

//writes the contents of a cSample object to an ESP file
void writeEspFile(std::string path, cSample& sample, unsigned int filestd, engineCfg config)
{
    FILE* file = fopen(path.c_str(), "wb");
    espFileHeader header;
	header.filestd = filestd;
    header.sampleRate = config.sampleRate;
    header.tickRate = config.tickRate;
    header.batchSize = config.batchSize;
    header.nHarmonics = config.nHarmonics;
    header.batches = sample.config.batches;
    header.pitchLength = sample.config.pitchLength;
	header.markerLength = sample.config.markerLength;
    header.pitch = sample.config.pitch;
    header.isVoiced = sample.config.isVoiced;
    header.isPlosive = sample.config.isPlosive;
    fwrite(&header, sizeof(espFileHeader), 1, file);
    fwrite(sample.pitchDeltas, sizeof(int), sample.config.pitchLength, file);
	fwrite(sample.pitchMarkers, sizeof(int), sample.config.markerLength, file);
    fwrite(sample.specharm, sizeof(float), sample.config.batches * config.frameSize, file);
    fwrite(sample.avgSpecharm, sizeof(float), config.halfHarmonics + config.halfTripleBatchSize + 1, file);
    fclose(file);
}
