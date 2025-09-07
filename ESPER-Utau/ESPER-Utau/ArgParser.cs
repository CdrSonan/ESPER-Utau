namespace ESPER_Utau;

/// <summary>
/// Parses command-line arguments passed to an UTAU resampler by the host application.
/// The expected arguments are:
/// 1. Path to the input audio file.
/// 2. Path to the output audio file.
/// 3. MIDI pitch (e.g., "C4").
/// 4. Velocity (0-127).
/// 5. Flags (a string containing compatible resampler flags and their respective values).
/// 6. offset as specified in oto.ini.
/// 7. desired length of the resampled audio in milliseconds.
/// 8. consonant length as specified in oto.ini.
/// 9. cutoff as specified in oto.ini.
/// 10. volume multiplier
/// 11. modulation strength (0.0-1.0).
/// 12. tempo multiplier (e.g., "x1.0" for no change).
/// 13. pitch bend data in UTAUs proprietary format.
/// </summary>
public class ArgParser
{
    public readonly string RsmpDir;
    public readonly string InputPath;
    public readonly string OutputPath;
    public readonly long Pitch;
    public readonly double Velocity;
    public readonly Dictionary<string, int> Flags;
    public readonly double Offset;
    public readonly long Length;
    public readonly double Consonant;
    public readonly double Cutoff;
    public readonly double Volume;
    public readonly double Modulation;
    public readonly double Tempo;
    public readonly int[] PitchBend;
    
    /// <summary>
    /// Initializes a new instance of the <see cref="ArgParser"/> class, and fills its attributes with the parsed command-line arguments.
    /// </summary>
    /// <param name="args">The command-line arguments passed to the resampler.</param>
    /// <exception cref="ArgumentException">Thrown when the number of arguments is not equal to the expected count.</exception>
    public ArgParser(string[] args)
    {
        if (args.Length != 14)
        {
            throw new ArgumentException("Expected 14 arguments, but received " + args.Length);
        }
        RsmpDir = Path.GetDirectoryName(args[0]) ?? throw new InvalidOperationException();
        InputPath = Path.Exists(args[1]) ? args[1] : throw new InvalidOperationException();
        OutputPath = Path.GetFullPath(args[2]);
        Pitch = NoteToMidiPitch(args[3]);
        Velocity = double.Parse(args[4], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture);
        Flags = ParseFlagString(args[5]);
        Offset = double.Parse(args[6], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture);
        Length = long.Parse(args[7]);
        Consonant = double.Parse(args[8], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture);
        Cutoff = double.Parse(args[9], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture);
        Volume = double.Parse(args[10], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture);
        Modulation = double.Parse(args[11], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture);
        Tempo = double.Parse(args[12][1..], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture);
        PitchBend = DecodePitchBend(args[13]);
    }

    /// <summary>
    /// Converts a note string (e.g., "C4") to a MIDI pitch value.
    /// </summary>
    private static long NoteToMidiPitch(string note)
    {
        // Convert note string to MIDI pitch (e.g., "C4" to 60)
        if (string.IsNullOrEmpty(note) || note.Length < 2)
        {
            throw new ArgumentException("Invalid note format.");
        }
        
        if (note[1] == '#' || note[1] == 'b')
        {
            // Handle sharps and flats
            var baseNote = note[0];
            var octave = int.Parse(note[2..]);
            var pitch = baseNote switch
            {
                'C' => 0,
                'D' => 2,
                'E' => 4,
                'F' => 5,
                'G' => 7,
                'A' => 9,
                'B' => 11,
                _ => throw new ArgumentException("Invalid note character.")
            };
            switch (note[1])
            {
                case '#':
                    pitch++;
                    break;
                case 'b':
                    pitch--;
                    break;
            }
            return pitch + (octave + 1) * 12; // MIDI pitch starts at C-1 (MIDI note 0)
        }
        else
        {
            // Handle natural notes
            var baseNote = note[0];
            var octave = int.Parse(note[1..]);
            return baseNote switch
            {
                'C' => octave * 12 + 0,
                'D' => octave * 12 + 2,
                'E' => octave * 12 + 4,
                'F' => octave * 12 + 5,
                'G' => octave * 12 + 7,
                'A' => octave * 12 + 9,
                'B' => octave * 12 + 11,
                _ => throw new ArgumentException("Invalid note character.")
            };
        }
    }

    private static readonly HashSet<string> SupportedFlags =
        ["ovl", "stb", "std", "dyn", "bri", "rgh", "gro", "B", "m", "t", "g"];

    /// <summary>
    /// Converts a dense flag string into a dictionary of the supported flags and their respective values.
    /// </summary>
    private static Dictionary<string, int> ParseFlagString(string flagString)
    {
        var flags = new Dictionary<string, int>();
        var warning = false;
        var index = 0;

        while (index < flagString.Length)
        {
            var flagFound = false;
            foreach (var flag in SupportedFlags.Where(flag => flagString.Length >= index + flag.Length &&
                                                              flagString.Substring(index, flag.Length) == flag))
            {
                flagFound = true;
                var startIndex = index + flag.Length;
                var endIndex = startIndex;

                // Find the end of the number (first non-digit and non-minus character)
                while (endIndex < flagString.Length &&
                       (char.IsDigit(flagString[endIndex]) || flagString[endIndex] == '-'))
                {
                    endIndex++;
                }

                string numberStr = flagString.Substring(startIndex, endIndex - startIndex);
                if (numberStr.Length > 0 && int.TryParse(numberStr, out var value))
                {
                    flags[flag] = value;
                }
                else
                {
                    flags[flag] = 0; // default value if parsing fails or no number is present
                }

                index = endIndex;
                break;
            }

            if (!flagFound)
            {
                index++;
                if (!warning)
                {
                    Console.WriteLine("Warning: Unsupported flag(s) found. Other flags may be misinterpreted.");
                    warning = true;
                }
            }
        }

        return flags;
    }

    private const string Base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    /// <summary>
    /// Decodes a Base64-like string into an array of integers.
    /// The string is expected to be in UTAU's specific format where each pair of characters represents a signed integer.
    /// Each character is mapped to a value in the range of -2048 to 2047.
    /// </summary>
    private static int[] DecodeBase64(string base64String)
    {
        if (base64String.Length % 2 != 0)
        {
            throw new ArgumentException("Base64 string length must be even.");
        }

        var result = new int[base64String.Length / 2];

        for (var i = 0; i < base64String.Length; i += 2)
        {
            var firstIndex = Base64Chars.IndexOf(base64String[i]);
            var secondIndex = Base64Chars.IndexOf(base64String[i + 1]);

            if (firstIndex == -1 || secondIndex == -1)
            {
                throw new ArgumentException("Invalid Base64 character encountered.");
            }

            var value = firstIndex * 64 + secondIndex;
            if (value >= 2048)
            {
                value -= 4096;
            }
            result[i / 2] = -value;
        }
        return result;
    }

    /// <summary>
    /// Decodes an UTAU pitch bend string into an array of integers.
    /// </summary>
    private static int[] DecodePitchBend(string pitchBendString)
    {
        var pitchBend = Array.Empty<int>();
        var substrings = new List<string>();

        // Split the string into substrings at the '#' character
        while (pitchBendString.Length > 0)
        {
            var splitIndex = pitchBendString.IndexOf('#');
            if (splitIndex == -1)
            {
                substrings.Add(pitchBendString);
                break;
            }
            substrings.Add(pitchBendString[..splitIndex]);
            pitchBendString = pitchBendString[(splitIndex + 1)..];
        }

        for (var i = 0; i < substrings.Count; i += 2)
        {
            // Decode the first substring (Base64 data)
            var base64Data = substrings[i];
            var decoded = DecodeBase64(base64Data);
            pitchBend = pitchBend.Concat(decoded).ToArray();

            // Process the repetition count if it exists
            if (i + 1 >= substrings.Count) continue;
            if (int.TryParse(substrings[i + 1], out var repetitions))
            {
                if (pitchBend.Length <= 0) continue;
                for (var j = 0; j < repetitions; j++)
                {
                    pitchBend = pitchBend.Append(pitchBend[^1]).ToArray();
                }
            }
            else
            {
                throw new ArgumentException("Invalid repetition count.");
            }
        }
        return pitchBend;
    }
}