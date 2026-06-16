using ESPER_Utau;
using libESPER_V2;
using libESPER_V2.Core;
using libESPER_V2.Effects;
using libESPER_V2.Transforms;
using MathNet.Numerics.Interpolation;
using MathNet.Numerics.LinearAlgebra;
using MathNet.Numerics.Statistics;
using NAudio.Wave;

namespace ESPER_Utau
{
    public class AudioProcessor
    {
        private readonly ConfigParser _configParser;
        private int _sampleRate;
        private EsperAudio? _esperAudio;
        
        public AudioProcessor(ConfigParser configParser)
        {
            _configParser = configParser;
        }
        
        public void ProcessAudio(ArgParser argParser)
        {
            // Load audio data
            (_esperAudio, _sampleRate) = EsperWrapper.LoadOrCreate(argParser.InputPath, _configParser);
            
            // Calculate audio segment parameters
            var length = (int)(argParser.Length * _sampleRate / _configParser.StepSize / 1000);
            var consonant = (int)(argParser.Consonant * _sampleRate / _configParser.StepSize / 1000);
            var offset = (int)(argParser.Offset * _sampleRate / _configParser.StepSize / 1000);
            var cutoff = (int)(argParser.Cutoff * _sampleRate / _configParser.StepSize / 1000);
            
            // Calculate vowel length with validation
            int vowel = CalculateVowelLength(offset, consonant, cutoff, _esperAudio.Length);
            
            // Validate and adjust parameters
            ValidateAndAdjustOto(ref length, consonant, ref offset, vowel);
            
            // Create parameter arrays
            var parameters = CreateParameters(argParser, length);
            
            // Process pitch
            var pitchData = ProcessPitch(argParser);
            
            // Process audio segments
            var outputAudio = ProcessAudioSegments(offset, consonant, vowel, length, parameters.Overlap);
            
            // Apply pitch modulation
            ApplyPitchModulation(outputAudio, pitchData, (float)argParser.Modulation);
            
            // Apply effects
            ApplyEffects(outputAudio, parameters);
            
            // Generate final output
            GenerateOutput(outputAudio, argParser.OutputPath, (float)argParser.Volume);
        }
        
        private int CalculateVowelLength(int offset, int consonant, int cutoff, int audioLength)
        {
            int vowel;
            if (cutoff < 0)
            {
                vowel = -cutoff;
            }
            else
            {
                vowel = audioLength - offset - cutoff;
            }
            vowel -= consonant;
            
            if (vowel <= 2)
            {
                Console.WriteLine("WARNING: oto.ini vowel length is below 3 frames. Extending to minimum viable size.");
                vowel = 3;
            }
            
            return vowel;
        }
        
        private void ValidateAndAdjustOto(ref int length, int consonant, ref int offset, int vowel)
        {
            if (length <= consonant + 2)
            {
                Console.WriteLine("WARNING: requested length is shorter than oto.ini consonant length. The resampled file will be longer than requested.");
                length = consonant + 3;
            }
            
            if (_esperAudio != null && offset + consonant + vowel > _esperAudio.Length)
            {
                Console.WriteLine("WARNING: oto.ini cutoff is beyond audio file end. Reducing offset to compensate.");
                offset -= _esperAudio.Length - offset - consonant - vowel;
            }
            
            if (offset < 0)
            {
                throw new Exception("Adjustment to invalid oto.ini parameters failed");
            }
        }
        
        private AudioProcessingParameters CreateParameters(ArgParser argParser, int length)
        {
            return new AudioProcessingParameters
            {
                Breathiness = ParameterUtils.MakeParamArray(argParser, "B", 0.0f, length),
                Brightness = ParameterUtils.MakeParamArray(argParser, "bri", 0.0f, length),
                Dynamic = ParameterUtils.MakeParamArray(argParser, "dyn", 0.0f, length),
                FormantShift = ParameterUtils.MakeParamArray(argParser, "g", 0.0f, length),
                Growl = ParameterUtils.MakeParamArray(argParser, "gro", 0.0f, length),
                Mouth = ParameterUtils.MakeParamArray(argParser, "m", 0.0f, length),
                Roughness = ParameterUtils.MakeParamArray(argParser, "rgh", 0.0f, length),
                Steadiness = ParameterUtils.MakeParamArray(argParser, "std", 0.0f, length),
                Overlap = argParser.Flags.TryGetValue("ovl", out var overlapFlag) ? (float)(overlapFlag / 100.0) : 0.0f
            };
        }
        
        private PitchProcessingData ProcessPitch(ArgParser argParser)
        {
            var pitchArr = new double[argParser.PitchBend.Length];
            var scale = Vector<double>.Build.Dense(argParser.PitchBend.Length, i => i);
            var newScale = Vector<double>.Build.Dense(_esperAudio?.Length ?? 0, i => double.Min(i * 1.6 * _configParser.StepSize * argParser.Tempo / _sampleRate, argParser.PitchBend.Length - 1));
            
            double basePitch = PitchUtils.MidiPitchToEsperPitch(argParser.Pitch, _sampleRate);
            
            if (argParser.Flags.TryGetValue("t", out var tFlag))
            {
                basePitch += double.Pow(2, (double)tFlag / 100);
            }
            
            for (var i = 0; i < argParser.PitchBend.Length; i++)
            {
                pitchArr[i] = basePitch * double.Pow(2, (double)argParser.PitchBend[i] / 1200);
            }
            
            var pitchBendInterpolator = new StepInterpolation(scale.ToArray(), pitchArr);
            var resampledPitch = Vector<float>.Build.Dense(_esperAudio?.Length ?? 0, i => (float)pitchBendInterpolator.Interpolate(newScale[i]));
            
            return new PitchProcessingData
            {
                ResampledPitch = resampledPitch,
                Length = _esperAudio?.Length ?? 0
            };
        }
        
        private EsperAudio ProcessAudioSegments(int offset, int consonant, int vowel, int length, float overlap)
        {
            if (_esperAudio == null)
                throw new InvalidOperationException("EsperAudio not initialized");
            
            var consonantAudio = CutCombine.Cut(_esperAudio, offset, offset + consonant);
            var vowelAudio = CutCombine.Cut(_esperAudio, offset + consonant, offset + consonant + vowel);
            
            var resampledVowelAudio = overlap == 0.0f 
                ? Stretch.StretchAudio(vowelAudio, length - consonant) 
                : Stretch.StretchLoopHybrid(vowelAudio, length - consonant, overlap);
            
            return CutCombine.Concat(consonantAudio, resampledVowelAudio);
        }
        
        private void ApplyPitchModulation(EsperAudio outputAudio, PitchProcessingData pitchData, float modulation)
        {
            var oldPitch = outputAudio.GetPitch();
            oldPitch -= oldPitch.Median();
            oldPitch *= modulation / 100.0f;
            
            // Resize pitch data if needed
            if (pitchData.ResampledPitch.Count != outputAudio.Length)
            {
                // Simple interpolation to match lengths
                var resizedPitch = Vector<float>.Build.Dense(outputAudio.Length);
                double ratio = (double)pitchData.ResampledPitch.Count / outputAudio.Length;
                for (int i = 0; i < outputAudio.Length; i++)
                {
                    int srcIndex = (int)(i * ratio);
                    if (srcIndex >= pitchData.ResampledPitch.Count)
                        srcIndex = pitchData.ResampledPitch.Count - 1;
                    resizedPitch[i] = pitchData.ResampledPitch[srcIndex];
                }
                pitchData.ResampledPitch = resizedPitch;
            }
            
            pitchData.ResampledPitch += oldPitch;
        }
        
        private void ApplyEffects(EsperAudio outputAudio, AudioProcessingParameters parameters)
        {
            Effects.FusedPitchFormantShift(outputAudio, parameters.ResampledPitch, parameters.FormantShift);
            Effects.Brightness(outputAudio, parameters.Brightness);
            Effects.Steadiness(outputAudio, parameters.Steadiness);
            Effects.Breathiness(outputAudio, parameters.Breathiness);
            Effects.Dynamics(outputAudio, parameters.Dynamic);
            Effects.Mouth(outputAudio, parameters.Mouth);
            Effects.Roughness(outputAudio, parameters.Roughness);
            Effects.Growl(outputAudio, parameters.Growl);
        }
        
        private void GenerateOutput(EsperAudio outputAudio, string outputPath, float volume)
        {
            var (outputWave, _) = EsperTransforms.Inverse(outputAudio);
            outputWave *= volume / 100;
            
            using var writer = new WaveFileWriter(outputPath, new WaveFormat(_sampleRate, 16, 1));
            writer.WriteSamples(outputWave.ToArray(), 0, outputWave.Count);
        }
        
        // Nested classes for parameter passing
        private class AudioProcessingParameters
        {
            public Vector<float> Breathiness { get; set; } = null!;
            public Vector<float> Brightness { get; set; } = null!;
            public Vector<float> Dynamic { get; set; } = null!;
            public Vector<float> FormantShift { get; set; } = null!;
            public Vector<float> Growl { get; set; } = null!;
            public Vector<float> Mouth { get; set; } = null!;
            public Vector<float> Roughness { get; set; } = null!;
            public Vector<float> Steadiness { get; set; } = null!;
            public float Overlap { get; set; }
            public Vector<float> ResampledPitch { get; set; } = null!;
        }
        
        private class PitchProcessingData
        {
            public Vector<float> ResampledPitch { get; set; } = null!;
            public int Length { get; set; }
        }
    }
}