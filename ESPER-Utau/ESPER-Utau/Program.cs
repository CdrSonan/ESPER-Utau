
using ESPER_Utau;
using libESPER_V2.Effects;
using libESPER_V2.Transforms;
using MathNet.Numerics.Interpolation;
using MathNet.Numerics.LinearAlgebra;
using NAudio.Wave;

var arguments = Environment.GetCommandLineArgs();
var argParser = new ArgParser(arguments);
var configParser = new ConfigParser(argParser.InputPath, argParser.RsmpDir);
var (esperAudio, sampleRate) = EsperWrapper.LoadOrCreate(argParser.InputPath, configParser);

var length = (int)(argParser.Length * sampleRate / configParser.StepSize / 1000);
var consonant = (int)(argParser.Consonant * sampleRate / configParser.StepSize / 1000);
var offset = (int)(argParser.Offset * sampleRate / configParser.StepSize / 1000);
var cutoff = (int)(argParser.Cutoff * sampleRate / configParser.StepSize / 1000);

int vowel;
if (cutoff < 0)
{
    vowel = -cutoff;
}
else
{
    vowel = esperAudio.Length - offset - cutoff;
}
vowel -= consonant;
consonant += offset;
if (cutoff < 1)
{
    consonant -= 1 - cutoff;
    vowel = 1;
}

if (length <= consonant + 1)
{
    length = consonant + 2;
}

// Create parameter arrays
var breathiness = MakeParamArray(argParser, "bre", 0.0f, length);
var brightness = MakeParamArray(argParser, "bri", 0.0f, length);
var dynamic = MakeParamArray(argParser, "dyn", 0.0f, length);
var formantShift = MakeParamArray(argParser, "g", 0.0f, length);
var growl = MakeParamArray(argParser, "gro", 0.0f, length);
var mouth = MakeParamArray(argParser, "m", 0.0f, length);
var roughness = MakeParamArray(argParser, "rgh", 0.0f, length);

var pitchArr = new double[length];
var scale = Vector<double>.Build.Dense(argParser.PitchBend.Length, i => i);
var newScale = Vector<double>.Build.Dense(length, i => i * ((double)length / argParser.PitchBend.Length));
double basePitch;
if (argParser.Flags.TryGetValue("t", out var tFlag))
{
    basePitch = MidiPitchToEsperPitch(argParser.Pitch, sampleRate);
}
else
{
    basePitch = MidiPitchToEsperPitch(argParser.Pitch, sampleRate) + Double.Pow(2, (double)tFlag / 100);
}
for (var i = 0; i < length; i++)
{
    pitchArr[i] = argParser.PitchBend[i] + basePitch;
}
var pitchBendInterpolator = new StepInterpolation(scale.ToArray(), pitchArr);
var resampledPitch = Vector<float>.Build.Dense(length, i => (float)pitchBendInterpolator.Interpolate(newScale[i]));

var consonantAudio = CutCombine.Cut(esperAudio, offset, offset + consonant);
var vowelAudio = CutCombine.Cut(esperAudio, offset + consonant, offset + consonant + vowel);
var resampledVowelAudio = vowelAudio; // TODO: Apply resampling logic once implemented in libESPER_V2
var outputAudio = CutCombine.Concat(consonantAudio, resampledVowelAudio);

// Apply effects
Effects.FusedPitchFormantShift(outputAudio, resampledPitch, formantShift);
Effects.Brightness(outputAudio, brightness);
Effects.Breathiness(outputAudio, breathiness);
Effects.Dynamics(outputAudio, dynamic);
Effects.Mouth(outputAudio, mouth);
Effects.Roughness(outputAudio, roughness);
Effects.Growl(outputAudio, growl);

var (outputWave, _) = EsperTransforms.Inverse(outputAudio);

using var writer = new WaveFileWriter(argParser.OutputPath, WaveFormat.CreateIeeeFloatWaveFormat(sampleRate, 1));
writer.WriteSamples(outputWave.ToArray(), 0, outputWave.Count);

return;

Vector<float> MakeParamArray(ArgParser args, string paramName, float defaultValue, long arrLength)
{
    if (!args.Flags.TryGetValue(paramName, out var value))
    {
        return Vector<float>.Build.DenseOfEnumerable(Enumerable.Repeat(defaultValue, (int)arrLength));
    }
    var val = value / 100.0f;
    return Vector<float>.Build.DenseOfEnumerable(Enumerable.Repeat(val, (int)arrLength));
}

float MidiPitchToEsperPitch(float pitch, int waveSampleRate)
{
    return waveSampleRate / (440 * float.Pow(2, (pitch - 69 + 12) / 12));
}