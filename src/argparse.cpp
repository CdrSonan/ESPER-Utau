#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "argparse.hpp"

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
            case 'b':
                halftone = -1;
            default:
                halftone = 0;
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

std::map<std::string, int> parseFlagString(std::string flagString) {
    std::string supportedFlags[] =
    {
        "lovl",
        "loff",
        "pstb",
        "std",
        "bre",
        "g",
        "int",
        "subh",
        "p",
        "dyn",
        "bri",
        "rgh",
        "grwl"
    };
    std::map<std::string, int> flags;
    while (flagString.length() > 0) {
        for (int i = 0; i < supportedFlags->length(); i++) {
            if (flagString.find(supportedFlags[i]) == 0) {
                for (size_t j = i + supportedFlags[i].length(); j < flagString.length(); j++) {
                    if (j == flagString.length() || !isdigit(flagString[j])) {
                        flags[supportedFlags[i]] = std::stoi(flagString.substr(supportedFlags[i].length(), j - supportedFlags[i].length() - i));
                        flagString = flagString.substr(j, flagString.length() - j);
                        break;
                    }
                }
                break;
            }
        }
    }
    return flags;
}

std::vector<int> decodeBase64(std::string base64String)
{
    std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> result;
    // Decode pairs of Base64 characters
    for (int i = 0; i < base64String.length(); i += 2)
    {
        //unsigned 12-bit integer
        int value = (int)base64Chars.find(base64String[i]) * 64 + (int)base64Chars.find(base64String[i + 1]);
        // Convert to signed 12-bit integer
        if (value >= 2048)
        {
            value -= 4096;
        }
        result.push_back(value);
    }
    return result;
}

std::vector<int> decodePitchBend(std::string pitchBendString)
{
    std::vector<int> pitchBend;
    std::vector<std::string> substrings;
    // Split the string into substrings at the '#' character
    while (pitchBendString.length() > 0) {
        size_t splitIndex = pitchBendString.find('#');
        if (splitIndex == std::string::npos)
        {
            splitIndex = pitchBendString.length();
        }
        substrings.push_back(pitchBendString.substr(0, splitIndex));
        pitchBendString = pitchBendString.substr(splitIndex, pitchBendString.length() - splitIndex);
    }
    // Decode pairs of substrings
    for (int i = 0; i < substrings.size(); i += 2)
    {
        //first substring is the base64 encoded pitch bend data
        std::vector<int> decoded = decodeBase64(substrings[i]);
        for (int j = 0; j < decoded.size(); j++)
        {
            pitchBend.push_back(decoded[j]);
        }
        //second substring is the number of repetitions, in plain text
        if (i + 1 < substrings.size())
        {
            for (int j = 0; j < std::stoi(substrings[i + 1]); j++)
            {
                for (int k = 0; k < decoded.size(); k++)
                {
                    pitchBend.push_back(decoded[k]);
                }
            }
        }
    }
    return pitchBend;
}

resamplerArgs parseArguments(int argc, char* argv[]) {
    resamplerArgs args;
    args.inputPath = argv[0];
    args.outputPath = argv[1];
    args.pitch = noteToMidiPitch(argv[2]);
    args.velocity = std::stof(argv[3]);
    args.flags = parseFlagString(argv[4]);
    args.offset = std::stof(argv[5]);
    args.length = std::stoi(argv[6]);
    args.consonant = std::stof(argv[7]);
    args.cutoff = std::stof(argv[8]);
    args.volume = std::stof(argv[9]);
    args.modulation = std::stof(argv[10]);
    std::string tempoString = argv[11];
    args.tempo = std::stof(tempoString.substr(1, tempoString.length() - 1));
    args.pitchBend = decodePitchBend(argv[12]);
    return args;
}

