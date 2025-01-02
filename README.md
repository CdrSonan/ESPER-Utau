# ESPER-Utau

ESPER-Utau is a new resampler for the UTAU and OpenUTAU engines.
It is based on the ESPER vocal synthesis framework designed for Nova-Vox, and puts the focus on a smooth sound, low engine noise and clarity.

# Installation and usage - Windows
You can download the Windows version of ESPER-Utau from the Nova-Vox website, or from the releases section of this repository.
The resampler comes in a zipped folder, and you will need to extract the all files to use it.

## UTAU
To use the resampler with UTAU, put all files included in the zipped folder into the same, shared folder.
Then, as with other resamplers, select ESPER-Utau.exe as the resampler executable in UTAU.

## OpenUTAU
To use the resampler with OpenUTAU, navigate to OpenUTAU's data directory and open the folder named "Resamplers".
Inside the resamplers folder, create a new folder named ESPER-Utau (or any other name of your choosing), and extract the contents of the .zip download into it.
The resampler will then be able for selection in the settings dialogue of all tracks.

# Installation and usage - Linux & OpenUTAU
Like the windows version, you can also download the Linux version of the resampler from either the Nova-Vox website, or the releases section of this repository.
Then, follow the same procedure and create a new folder inside the "resamplers" folder in your OpenUTAU install directory, and extract the .tar archive into it.
You can then use the resampler by selecting it in the settings dialogue of a track.

# Flags/Expressions
ESPER-Utau supports a variety of mostly non-standard flags (called Expressions in OpenUTAU):

| Name | UTAU flag | OpenUTAU | range | description |
| :--- | :-------: | :------: | :---: | :---------- | 
| breathiness | bre | Ebre | -100 - 100 | controls the ratio of voiced and unvoiced singing, making the voice sound more, or less breathy |
| gender/formant shift | g | Egen | -100 - 100 | Shifts the spectrum of voice, resulting in a higher, or lower-pitched sound |
| intonation | int | Eint | 0 - 100 | Controls the amount of "effort" the singer uses to reach very high or low notes. Very subtle effect |
| subharmonics | subh | Esub | -100 - 100 | Controls the amount of subharmonics, which can either smooth out or amplify consonants, vocal cracks, etc. |
| peak compressor | p | Ecmp | 0 - 100 | Selectively lowers the volume of loud sections of audio, balancing it out overall |
| dynamics | dyn | Edyn | -100 - 100 | Controls the amount of force or strength the singer sings with. Also affects volume |
| brightness | bri | Ebri | -100 - 100 |  Modifies the spectrum to create a brighter or more muffled sound |
| roughness | rgh | Ergh | 0 - 100 | Introduces phase variations into the voice, making it sound more coarse |
| growl | grwl | Egrw | 0 - 100 | Growl effect. Uses larger and longer variations compared to roughness |
| pitch stability | pstb | Epst | -100 - 100 | Increases or decreases the amount of pitch variation within the note |
| steadiness | std | Estd | -100 - 100 | Adjusts the amount of variation in the vocal spectrum, making the voice sound more, or less steady |
| t | t | t | -100 - 100 | Standard UTAU t flag. Adjusts note pitch in units of cents |
| loop overlap | lovl | Elov | 0 - 100 | Controls the amount of overlap used when looping a sample to fill a long note |
| loop offset | loff | Elof | 0 - 100 | Adjusts the side of the "head" and "tail" sections of the vowel, which are not used for looping |

# Configuration options
The resampler can be further configured by editing the esper-config.ini file. The available options are explained within the file itself, but there are a few especially important ones worth mentioning:

## Caching
ESPER-Utau works in two stages: It first analyses an audio file and translates it to an intermediate representation, then, using the oto.ini file and data provided by UTAU/OpenUTAU, selects the appropriate section of this intermediate representation and resamples it to form the final audio.
Creating the intermediate representation is several times more time-consuming than the resampling step. Therefore, by default, this is only done once per audio file, and the intermediate representation is saved to a .esp file alongside the audio file.
This allows it to be reused during later resampling operations, which drastically speeds up the process.
However, .esp file use a significant amount of space - typically about 3 times the size of the original audio. If that is a concern for you, it is recommended to disable .esp file creation by setting "createEsperFiles" to "false" in the config file.
Another caveat is that, since OpenUTAU currently does not recognise .esp as an auxillary file type, .esp files are only saved to OpenUTAU's cache directory, and can neither be written to nor read from Voicebank directories when using OpenUTAU.

## .frq files
.frq files are a standardized way to store pitch and volume information about audio files.
ESPER-Utau is capable of creating .frq files, and will, by default, do so for any audio files it processes that do not already have a matching .frq file.
However, in the current version, it can only use .frq file to calculate an average pitch of the entire sample to base its calculations on. It can not use the actual pitch curve contained in the file.
However, this is planned for a future update.

## target amplitude
The targetAmplitude setting controls the volume normalisation applied to the audio before processing. This is done to keep volume levels between different samples and voices consistent, while still allowing volume variations between phonemes.
Higher values mean a higher output volume, but also pose an increased risk of clipping. Setting it to 0 disables normalisation, making the resampled audio match the original volume of the samples it is based on.

# Building from source

## Windows

To build ESPER-Utau on Windows, you are going to need the following software:
- Microsoft Visual Studio 2019 or later (earlier versions may also work, but are untested)
- CMake version 22 or later
- Git

First, use Git to clone this repository into a folder of your choice. Then create a folder named "build" in the repository.
Next run CMake. This can either be done from the console, or by using CMake GUI and pointing it at the repository root and build folders.
After CMake has run, the build folder is going to contain a .sln file that you can open with Visual Studio. This is going to be your main entry point for editing and compiling the project.
To compile, open the .sln file with Visual Studio, right-click the main solution in the project explorer on the right, and select "build solution".
All binary file required to run the resampler are going to be placed in build/bundled. To get a working copy, also add the esper-config.ini file found in the repository root folder to these files.

## Linux
All packages required for building on Linux can be installed via the buildessentials meta-package.
After installing it, navigate to a folder of your choice, and clone the repository. Next, create a folder named "build" inside of the repository.
Open a console window and navigate into this new build folder. Run
cmake ..
to create a makefile for the project, and then run 
make
to compile and build the project.
Like on Windows, the resultant binary files are placed in build/bundled, and you need to add esper-config.ini to them to get a working copy.
Compiling on Linux takes significantly longer than on Windows, since it includes a download and full build of the 3rd-party libraries libfftw and libnfft.

# Known bugs
- nfft built through the Linux build process has a misconfigured rpath attribute, preventing the resampler from starting if it isn't corrected first
- build steps on Windows may happen out of order, because of which several subsequent build attempts may be required to build all files
- certain oto.ini configurations may cause a crash
- certain samples may experience an unexpected formant shift
- end breaths and certain other samples may be normalised to an incorrect, too high volume
