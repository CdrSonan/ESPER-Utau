#include <iostream>
#include <map>
#include <string>

std::map<std::string, std::string> saveCommandLineArguments(int argc, char* argv[]) {
    std::map<std::string, std::string> arguments;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        std::string key, value;

        // Check if the argument has a value
        size_t pos = arg.find('=');
        if (pos != std::string::npos) {
            key = arg.substr(0, pos);
            value = arg.substr(pos + 1);
        } else {
            key = arg;
        }

        arguments[key] = value;
    }

    return arguments;
}

float* parseFloatArg(std::map<std::string, std::string>& arguments, const std::string& key, int length) {
    if (arguments.find(key) != arguments.end()) {
        try {
            //figure out whether the argument is numeric or curve-type
            if (arguments[key].find(',') != std::string::npos) {
                //parse curve-type argument
                float* curve = new float[length];
                std::string value = arguments[key];
                size_t pos = 0;
                int i = 0;
                while ((pos = value.find(',')) != std::string::npos) {
                    curve[i] = std::stof(value.substr(0, pos));
                    value.erase(0, pos + 1);
                    i++;
                }
                curve[i] = std::stof(value);
                return curve;
            }
            //parse numeric argument
            return new float(std::stof(arguments[key]));
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid argument for " << key << ": " << arguments[key] << ". Using default value." << std::endl;
        }
    }

    return nullptr;
}
