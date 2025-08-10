namespace ESPER_Utau;

/// <summary>
/// Parses .ini configuration files for the ESPER-Utau application. Ignores section headers.
/// </summary>
public class ConfigParser
{
    public readonly bool UseFrq;
    public readonly bool UseEsp;
    public readonly bool CreateFrq;
    public readonly bool CreateEsp;
    public readonly bool OverwriteFrq;
    public readonly bool OverwriteEsp;

    public readonly long NVoiced;
    public readonly long NUnvoiced;
    public readonly long StepSize;
    public readonly double ExpPitch;
    public readonly double Smoothing;
    
    /// <summary>
    /// Initializes a new instance of the <see cref="ConfigParser"/> class, and fills its attributes with the read config values.
    /// </summary>
    /// <param name="path">The path to the configuration file.</param>
    /// <param name="fallbackPath">The fallback path if the configuration file is not found.</param>
    public ConfigParser(string path, string fallbackPath)
    {
        if (string.IsNullOrEmpty(path))
        {
            throw new ArgumentException("Path cannot be null or empty.", nameof(path));
        }

        if (!File.Exists(path))
        {
            if (string.IsNullOrEmpty(fallbackPath) || !Directory.Exists(fallbackPath))
            {
                throw new FileNotFoundException("Configuration file not found and no fallback path provided.", path);
            }
            path = fallbackPath;
        }
        
        var config = LoadConfig(path);
        foreach (var kvp in config)
        {
            var key = kvp.Key.ToLowerInvariant();
            var value = kvp.Value.ToLowerInvariant();
            switch (key)
            {
                case "usefrq":
                    UseFrq = value == "true";
                    break;
                case "useesp":
                    UseEsp = value == "true";
                    break;
                case "createfrq":
                    CreateFrq = value == "true";
                    break;
                case "createesp":
                    CreateEsp = value == "true";
                    break;
                case "overwritefrq":
                    OverwriteFrq = value == "true";
                    break;
                case "overwriteesp":
                    OverwriteEsp = value == "true";
                    break;
                case "nvoiced":
                    NVoiced = long.Parse(value);
                    break;
                case "nunvoiced":
                    NUnvoiced = long.Parse(value);
                    break;
                case "stepsize":
                    StepSize = long.Parse(value);
                    break;
                case "exppitch":
                    ExpPitch = double.Parse(value);
                    break;
                case "smoothing":
                    Smoothing = double.Parse(value);
                    break;
                default:
                    Console.WriteLine($"WARNING: Unknown configuration key: {key}");
                    break;
            }
        }
    }

    /// <summary>
    /// Raw .ini configuration file parser.
    /// </summary>
    /// <param name="path">The path to the configuration file.</param>
    /// <returns>A dictionary containing the configuration key-value pairs.</returns>
    private static Dictionary<string, string> LoadConfig(string path)
    {
        var config = new Dictionary<string, string>();
        path = Path.Join(path, "esper-config.ini");
        if (!File.Exists(path))
        {
            throw new FileNotFoundException("Configuration file not found.", path);
        }
        try
        {
            // Read the configuration file
            var lines = File.ReadAllLines(path);
            foreach (var line in lines)
            {
                if (string.IsNullOrWhiteSpace(line) ||
                    line.StartsWith('#') ||
                    line.StartsWith(';') ||
                    line.StartsWith('['))
                {
                    continue; // Skip empty lines and comments
                }

                var parts = line.Split('=');
                if (parts.Length != 2)
                {
                    throw new FormatException($"Invalid configuration line: {line}");
                }

                var key = parts[0].Trim();
                var value = parts[1].Trim();
                config.Add(key, value);

                // Process the key-value pair as needed
                // For example, you could store them in a dictionary or apply settings directly
            }
        }
        catch (Exception ex)
        {
            throw new IOException("Error reading configuration file.", ex);
        }
        return config;
    }
}