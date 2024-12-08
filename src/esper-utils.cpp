#include <malloc.h>
#include <math.h>
#include <vector>
#include <string>

#include "esper.h"

#include "esper-utils.hpp"

//applies the contents of a .frq pitch curve file to a sample, overwriting its pitchDeltas, expectedPitch, and pitch attributes
//Note that the pitchMarkers attribute is NOT overwritten, meaning this method is unsuitable as a replacement for a full pitch calculation as done in libESPER.
void applyFrqToSample(cSample& sample, double avg_frq, std::vector<double> frequencies, engineCfg config)
{
    for (int i = 0; i < frequencies.size(); i++) {
        frequencies[i] = config.sampleRate / frequencies[i];
    }
    sample.config.pitchLength = sample.config.batches;
    int srcLength = frequencies.size();
    int tgtLength = sample.config.pitchLength;
    //perform linear interpolation
    for (int i = 0; i < tgtLength; i++) {
        double tgtIndex = (double)i / tgtLength * srcLength;
        int srcIndex = (int)tgtIndex;
        double srcWeight = tgtIndex - srcIndex;
        if (srcIndex + 1 >= srcLength)
		{
			sample.pitchDeltas[i] = frequencies[srcIndex];
		}
        else
        {
            sample.pitchDeltas[i] = (1 - srcWeight) * frequencies[srcIndex] + srcWeight * frequencies[srcIndex + 1];
        }
    }
	sample.config.expectedPitch = avg_frq;
    sample.config.pitch = config.sampleRate / avg_frq;
}

//generates frequency and amplitude arrays from a sample's pitchDeltas and waveform attributes
//The result is intended to be written to a .frq file.
void getFrqFromSample(cSample& sample, std::vector<double>& frequencies, std::vector<double>& amplitudes, engineCfg config)
{
    frequencies.clear();
    amplitudes.clear();
	int srcLength = sample.config.pitchLength;  
	int tgtLength = config.sampleRate / 256;
	for (int i = 0; i < tgtLength; i++) {
		double tgtIndex = (double)i / tgtLength * srcLength;
		int srcIndex = (int)tgtIndex;
		double srcWeight = tgtIndex - srcIndex;
		if (srcIndex + 1 >= srcLength)
		{
			frequencies.push_back(sample.pitchDeltas[srcIndex]);
		}
		else
		{
			frequencies.push_back((1 - srcWeight) * sample.pitchDeltas[srcIndex] + srcWeight * sample.pitchDeltas[srcIndex + 1]);
		}
        float amplitude = 0;
        for (int j = 0; j < 256; j++) {
            if (abs(sample.waveform[i * 256 + j]) > amplitude) {
                amplitude = abs(sample.waveform[i * 256 + j]);
            }
        }
        amplitudes.push_back(amplitude);
    }
}

//converts a note string to a MIDI pitch value
//Assumes American note names written in uppercase, and followed by accidentals (if present), then an octave number, even if an alternative notation exists.
//Valid examples: "C4", "D#5", "Fb3". Invalid examples: "c4", "D5#", "Fb", "H3".
int noteToMidiPitch(std::string note) {
    std::string noteStr = "";
    std::string octaveStr = "";
    for (int i = 0; i < note.length(); i++) {
        if (isdigit(note[i])) {
            noteStr = note.substr(0, i);
            octaveStr = note.substr(i, note.length());
            break;
        }
    }
    int octave = std::stoi(octaveStr);
    int halftone = 0;
    if (noteStr.length() > 1) {
        switch (noteStr[1]) {
        case '#':
            halftone = 1;
            break;
        case 'b':
            halftone = -1;
            break;
        default:
            halftone = 0;
            break;
        }
    }
    switch (noteStr[0]) {
    case 'C':
        return 12 * octave + halftone;
    case 'D':
        return 12 * octave + 2 + halftone;
    case 'E':
        return 12 * octave + 4 + halftone;
    case 'F':
        return 12 * octave + 5 + halftone;
    case 'G':
        return 12 * octave + 7 + halftone;
    case 'A':
        return 12 * octave + 9 + halftone;
    case 'B':
        return 12 * octave + 11 + halftone;
    default:
        return 0;
    }
}

//converts a MIDI pitch value to an ESPER wavelength format pitch value
float midiPitchToEsperPitch(float pitch, engineCfg config)
{
	return (float)config.sampleRate / (440 * pow(2, (pitch - 69 + 12) / 12));
}
