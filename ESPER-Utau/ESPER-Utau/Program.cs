
using ESPER_Utau;

var arguments = Environment.GetCommandLineArgs();
var argParser = new ArgParser(arguments);
var configParser = new ConfigParser(argParser.InputPath, argParser.RsmpDir);
var esperAudio = EsperWrapper.LoadOrCreate(argParser.InputPath, configParser);