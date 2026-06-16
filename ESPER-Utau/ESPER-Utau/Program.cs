using ESPER_Utau;

var arguments = Environment.GetCommandLineArgs();

try
{
    var argParser = new ArgParser(arguments);
    var configParser = new ConfigParser(Path.GetDirectoryName(argParser.InputPath) ?? string.Empty, argParser.RsmpDir);
    
    var processor = new AudioProcessor(configParser);
    processor.ProcessAudio(argParser);
}
catch (Exception ex)
{
    Console.WriteLine($"Error processing audio: {ex.Message}");
    Environment.Exit(1);
}
