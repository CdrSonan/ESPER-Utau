using libESPER_V2.Core;
using libESPER_V2.Transforms;
using MathNet.Numerics.Interpolation;
using MathNet.Numerics.LinearAlgebra;
using MathNet.Numerics.Statistics;
using NAudio.Wave;

namespace ESPER_Utau;

public static class EsperWrapper
{
    public static (EsperAudio, int) LoadOrCreate(string filename, ConfigParser config)
    {
        var espFilename = Path.ChangeExtension(filename, ".esp");
        var frqFilename = filename[..^4] + "_wav.frq";
        
        using var reader = new WaveFileReader(filename);
        var sampleRate = reader.WaveFormat.SampleRate;
        
        // Attempt to load .esp file
        if (config.UseEsp)
        {
            if (File.Exists(espFilename))
            {
                var espBytes = File.ReadAllBytes(espFilename);
                try
                {
                    var fetchedEsperAudio =  Serialization.Deserialize(espBytes);
                    return (fetchedEsperAudio, sampleRate);
                }
                catch (Exception e)
                {
                    Console.WriteLine(e);
                }
                
            }
        }

        var expectedPitch = config.ExpPitch;
        // .esp file not found - run forward transform
        if (config.UseFrq && File.Exists(frqFilename))
        {
            var frq = new FrqParser(frqFilename);
            expectedPitch = sampleRate / frq.F0Mean;
        }
        
        // read audio file
        var waveform = new float[reader.SampleCount];
        
        for (var i = 0; i < waveform.Length; i++)
        {
            var frame = reader.ReadNextSampleFrame();
            //if (frame == null) break;
            var sample = frame[0];
            waveform[i] = sample;
        }

        var sampleConfig = new EsperAudioConfig((ushort)config.NVoiced, (ushort)config.NUnvoiced, (int)config.StepSize);
        var forwardConfig = new EsperForwardConfig(config.Smoothing, expectedPitch);
        
        var esperAudio = EsperTransforms.Forward(
            Vector<float>.Build.DenseOfArray(waveform),
            sampleConfig,
            forwardConfig);
        
        if (config.OverwriteFrq || (config.CreateFrq && !File.Exists(frqFilename)))
        {
            // Write FRQ file
            var f0 = esperAudio.GetPitch().Map(i => sampleRate / i);
            var amplitudes = esperAudio.GetVoicedAmps().Row(1);
            var f0Mean = f0.Mean();
            if (config.StepSize == 256)
            {
                FrqWriter.Write(frqFilename, f0.ToDouble().ToArray(), amplitudes.ToDouble().ToArray(), f0Mean);
            }
            else
            {
                var newCount = (int)Math.Ceiling(f0.Count * (256.0 / config.StepSize));
                var scale = Vector<double>.Build.Dense(f0.Count, i => i);
                var newScale = Vector<double>.Build.Dense(newCount, i => i * (256.0 / config.StepSize));
                
                var f0Interpolator = new StepInterpolation(scale.ToArray(), f0.ToDouble().ToArray());
                var ampsInterpolator = new StepInterpolation(scale.ToArray(), amplitudes.ToDouble().ToArray());
                var resampledF0 = new double[newCount];
                var resampledAmps = new double[newCount];
                for (int i = 0; i < newCount; i++)
                {
                    resampledF0[i] = f0Interpolator.Interpolate(newScale[i]);
                    resampledAmps[i] = ampsInterpolator.Interpolate(newScale[i]);
                }
                FrqWriter.Write(frqFilename, resampledF0, resampledAmps, f0Mean);
            }
        }
        
        if (config.OverwriteEsp || (config.CreateEsp && !File.Exists(espFilename)))
        {
            // Write ESP file
            var espBytes = Serialization.Serialize(esperAudio);
            try
            {
                File.WriteAllBytes(espFilename, espBytes);
            }
            catch (Exception e)
            {
                Console.WriteLine("Error while writing ESP file:");
                Console.WriteLine(e);
                Console.WriteLine("Continuing...");
            }
            
        }
        
        return (esperAudio, sampleRate);
    }
}