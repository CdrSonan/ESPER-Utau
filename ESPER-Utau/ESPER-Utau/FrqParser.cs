namespace ESPER_Utau;

public class FrqParser
{
    public readonly float[] F0;
    public readonly float F0Mean;

    public FrqParser(string filename)
    {
        if (string.IsNullOrWhiteSpace(filename))
            throw new ArgumentException("Filename is null or empty.", nameof(filename));
        if (!File.Exists(filename))
            throw new FileNotFoundException("FRQ file not found.", filename);

        using var fs = File.OpenRead(filename);
        using var br = new BinaryReader(fs);

        // Header (8 bytes, ignored content here)
        var headerChars = br.ReadChars(8);
        if (headerChars.Length < 8)
            throw new InvalidDataException("FRQ header too short.");

        var samplesPerFrq = br.ReadInt32();
        if (samplesPerFrq != 256)
            throw new InvalidDataException($"Unexpected samples per frq: {samplesPerFrq}");

        var avg = br.ReadDouble(); // Average frequency
        F0Mean = (float)avg;

        // Skip 16 bytes padding
        br.BaseStream.Seek(16, SeekOrigin.Current);

        var numChunks = br.ReadInt32();
        if (numChunks is < 0 or > 10_000_000)
            throw new InvalidDataException($"Unreasonable chunk count: {numChunks}");

        F0 = new float[numChunks];

        for (var i = 0; i < numChunks; i++)
        {
            var freq = br.ReadDouble();
            br.ReadDouble(); // amplitude (ignored)

            if (freq is < 0 or > 12000)
                freq = 0;

            F0[i] = (float)freq;
        }
    }
}

public class FrqWriter
{
    public static void Write(string filename, double[] f0, double[] amps, double f0Mean)
    {
        if (string.IsNullOrWhiteSpace(filename))
            throw new ArgumentException("Filename is null or empty.", nameof(filename));

        using var fs = File.Create(filename);
        using var bw = new BinaryWriter(fs);

        try
        {
            // Write header
            bw.Write("FREQ0003".ToCharArray());
            bw.Write(256); // samples per frq
            bw.Write(f0Mean); // average frequency

            // Padding
            bw.Write(new byte[16]);

            bw.Write(f0.Length); // number of chunks

            foreach (var (freq, amp) in f0.Zip(amps, (f, a) => (f, a)))
            {
                bw.Write(freq);
                bw.Write(amp);
            }
        }
        catch (Exception e)
        {
            Console.WriteLine("Error while writing FRQ file:");
            Console.WriteLine(e);
            Console.WriteLine("Continuing...");
        }
    }
}