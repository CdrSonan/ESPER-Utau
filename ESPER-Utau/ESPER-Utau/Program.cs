
using ESPER_Utau;
using libESPER_V2.Effects;
using libESPER_V2.Transforms;
using MathNet.Numerics.Interpolation;
using MathNet.Numerics.LinearAlgebra;
using MathNet.Numerics.Statistics;
using NAudio.Wave;

var arguments = Environment.GetCommandLineArgs();
var argParser = new ArgParser(arguments);
var configParser = new ConfigParser(Path.GetDirectoryName(argParser.InputPath), argParser.RsmpDir);
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

if (vowel <= 2)
{
    Console.WriteLine("WARNING: oto.ini vowel length is below 3 frames. Extending to minimum viable size.");
    vowel = 3;
}
if (length <= consonant + 2)
{
    Console.WriteLine("WARNING: requested length is shorter than oto.ini consonant length. The resampled file will be longer than requested.");
    length = consonant + 3;
}

if (offset + consonant + vowel > esperAudio.Length)
{
    Console.WriteLine("WARNING: oto.ini cutoff is beyond audio file end. Reducing offset to compensate.");
    offset -= esperAudio.Length - offset - consonant - vowel;
}

if (offset < 0)
{
    throw new Exception("Adjustment to invalid oto.ini parameters failed");
}

// Create parameter arrays
var breathiness = MakeParamArray(argParser, "B", 0.0f, length);
var brightness = MakeParamArray(argParser, "bri", 0.0f, length);
var dynamic = MakeParamArray(argParser, "dyn", 0.0f, length);
var formantShift = MakeParamArray(argParser, "g", 0.0f, length);
var growl = MakeParamArray(argParser, "gro", 0.0f, length);
var mouth = MakeParamArray(argParser, "m", 0.0f, length);
var roughness = MakeParamArray(argParser, "rgh", 0.0f, length);
var steadiness = MakeParamArray(argParser, "std", 0.0f, length);
var overlap = argParser.Flags.TryGetValue("ovl", out var overlapFlag) ? (float)(overlapFlag / 100.0) : 0.0f;

var pitchArr = new double[argParser.PitchBend.Length];
var scale = Vector<double>.Build.Dense(argParser.PitchBend.Length, i => i);
var newScale = Vector<double>.Build.Dense(length, i => double.Min(i * 1.6 * configParser.StepSize * argParser.Tempo / sampleRate, argParser.PitchBend.Length - 1));
double basePitch;
if (argParser.Flags.TryGetValue("t", out var tFlag))
{
    basePitch = MidiPitchToEsperPitch(argParser.Pitch, sampleRate) + double.Pow(2, (double)tFlag / 100);
}
else
{
    basePitch = MidiPitchToEsperPitch(argParser.Pitch, sampleRate);
}
for (var i = 0; i < argParser.PitchBend.Length; i++)
{
    pitchArr[i] = basePitch * double.Pow(2, (double)argParser.PitchBend[i] / 1200);
}
var pitchBendInterpolator = new StepInterpolation(scale.ToArray(), pitchArr);
var resampledPitch = Vector<float>.Build.Dense(length, i => (float)pitchBendInterpolator.Interpolate(newScale[i]));

var consonantAudio = CutCombine.Cut(esperAudio, offset, offset + consonant);
var vowelAudio = CutCombine.Cut(esperAudio, offset + consonant, offset + consonant + vowel);
var resampledVowelAudio = overlap == 0.0f ?
    Stretch.StretchAudio(vowelAudio, length - consonant) :
    Stretch.StretchLoopHybrid(vowelAudio, length - consonant, overlap);
var outputAudio = CutCombine.Concat(consonantAudio, resampledVowelAudio);

var oldPitch = outputAudio.GetPitch();
oldPitch -= oldPitch.Median();
oldPitch *= (float)argParser.Modulation / 100.0f;
resampledPitch += oldPitch;

// Apply effects
Effects.FusedPitchFormantShift(outputAudio, resampledPitch, formantShift);
Effects.Brightness(outputAudio, brightness);
Effects.Steadiness(outputAudio, steadiness);
Effects.Breathiness(outputAudio, breathiness);
Effects.Dynamics(outputAudio, dynamic);
Effects.Mouth(outputAudio, mouth);
Effects.Roughness(outputAudio, roughness);
Effects.Growl(outputAudio, growl);

var (outputWave, _) = EsperTransforms.Inverse(outputAudio);

outputWave *= (float)argParser.Volume / 100;

using var writer = new WaveFileWriter(argParser.OutputPath, new WaveFormat(sampleRate, 16, 1));
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