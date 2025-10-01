# ESPER-Utau

ESPER-Utau is a new resampler for the UTAU and OpenUTAU engines.
It is based on the ESPER vocal synthesis framework designed for Nova-Vox, and puts the focus on a smooth sound, low engine noise and clarity.

# Installation and usage
You can download ESPER-Utau from the releases section of this repository.
Download the executable matching your platform (Windows/Mac/Linux etc.), esper-config.ini, and ESPER-Utau.yaml.
esper-config.ini is required for the resampler to function, and ESPER-Utau.yaml provides expression definitions for OpenUTAU.

## UTAU - Windows
To use the resampler with UTAU on Windows, put ESPER-Utau windows.exe and esper-config.ini into the same folder.
Then, as with other resamplers, select ESPER-Utau windows.exe as the resampler executable in UTAU.

## OpenUTAU - All platforms
To use the resampler with OpenUTAU, navigate to OpenUTAU's data directory and open the folder named "Resamplers".
Inside the resamplers folder, create a new folder named ESPER-Utau (or any other name of your choosing), and place the executable matching your platform, esper-config.ini, and ESPER-Utau.yaml into it.
The resampler will then be able for selection in the settings dialogue of all tracks.
In order for OpenUTAU to automatically recognise the supported expressions, rename the executable to ESPER-Utau.exe if you are on Windows, or just ESPER-Utau on all other platforms.

# Flags/Expressions
ESPER-Utau supports a variety of mostly non-standard flags (called Expressions in OpenUTAU):

| Name | UTAU flag | OpenUTAU | range | description |
| :--- | :-------: | :------: | :---: | :---------- | 
| breath | B | bre | 0 - 100 | controls the ratio of voiced and unvoiced singing, making the voice sound more, or less breathy |
| gender/formant shift | g | gen | -100 - 100 | Shifts the spectrum of voice, resulting in a higher, or lower-pitched sound |
| dynamics | dyn | dyn | -100 - 100 | Controls the amount of force or strength the singer sings with. Also affects volume |
| brightness | bri | bri | -100 - 100 |  Modifies the spectrum to create a brighter or more muffled sound |
| roughness | rgh | rgh | 0 - 100 | Introduces phase variations into the voice, making it sound more coarse |
| growl | gro | gro | 0 - 100 | Growl effect. Uses larger and longer variations compared to roughness |
| pitch stability | stb | stb | -100 - 100 | Increases or decreases the amount of pitch variation within the note |
| steadiness | std | std | -100 - 100 | Adjusts the amount of variation in the vocal spectrum, making the voice sound more, or less steady |
| mouth | m | mou | -100 - 100 | Simulates the singer singing in a more, or less open way |
| t | t | t | -100 - 100 | Standard UTAU t flag. Adjusts note pitch in units of cents |
| loop overlap | lovl | Elov | 0 - 100 | Controls the amount of overlap used when looping the voiced portion of a sample to fill a long note. Set to 0 to stretch the sample instead of looping it.|

Additional, planned flags:
| Name | UTAU flag | OpenUTAU | range | description |
| :--- | :-------: | :------: | :---: | :---------- |
| subharmonics | subh | Esub | -100 - 100 | Controls the amount of subharmonics, which can either smooth out or amplify consonants, vocal cracks, etc. |
| peak compressor | p | Ecmp | 0 - 100 | Selectively lowers the volume of loud sections of audio, balancing it out overall |

# Configuration options
The resampler can be further configured by editing the esper-config.ini file. The available options are explained within the file itself, but there are a few especially important ones worth mentioning:

## Caching
ESPER-Utau works in two stages: It first analyses an audio file and translates it to an intermediate representation, then, using the oto.ini file and data provided by UTAU/OpenUTAU, selects the appropriate section of this intermediate representation and resamples it to form the final audio.
Creating the intermediate representation is several times more time-consuming than the resampling step. Therefore, by default, this is only done once per audio file, and the intermediate representation is saved to a .esp file alongside the audio file.
This allows it to be reused during later resampling operations, which drastically speeds up the process.
However, .esp file use a significant amount of space - typically about 1.5 to 2 times the size of the original audio. If that is a concern for you, it is recommended to disable .esp file creation by setting "createEsperFiles" to "false" in the config file.
Another caveat is that, since OpenUTAU currently does not recognise .esp as an auxillary file type, .esp files are only saved to OpenUTAU's cache directory, and can neither be written to nor read from Voicebank directories when using OpenUTAU.

## .frq files
.frq files are a standardized way to store pitch and volume information about audio files.
ESPER-Utau is capable of creating .frq files, and will, by default, do so for any audio files it processes that do not already have a matching .frq file.
However, in the current version, it can only use .frq file to calculate an average pitch of the entire sample to base its calculations on. It can not use the actual pitch curve contained in the file.
This is planned for a future update.

# Building from source

## Windows

To build ESPER-Utau, you will need an IDE capable of reading .sln files. Tested are:
- Microsoft Visual Studio 2019 and later
- Jetbrains Rider 2025.1 and later

Additionally, you will need a working .NET 8 installation. If you are using Jetbrains Rider, this is installed along with the IDE. If you are using Visual Studio, it needs to be selected during the IDE installation. 

- Clone or download this repository into a folder of your choice.
- Open the downloaded .sln file in your IDE.
- Download the NuGet packages required for the project. Your IDE should normally do this automatically.
- Publish the project to a folder using .NET Publich. Depending on your IDE, this feature will be found in different places. Make sure to select your target platform.
