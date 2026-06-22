using System;
using System.IO;

namespace OsbcExport;

internal static class Program
{
    private const int ExitOk    = 0;
    private const int ExitUsage = 2;
    private const int ExitError = 1;

    private static int Main(string[] args)
    {
        if (args.Length < 2)
        {
            Console.Error.WriteLine("usage: osbc-export <input.os> <output.osbin>");
            return ExitUsage;
        }

        var inputPath  = args[0];
        var outputPath = args[1];

        if (!File.Exists(inputPath))
        {
            Console.Error.WriteLine($"input not found: {inputPath}");
            return ExitError;
        }

        try
        {
            var module = Compilation.CompileFromFile(inputPath);
            using var stream = File.Create(outputPath);
            var writer = new OsbinWriter(stream);
            writer.WriteModule(module);
            Console.Error.WriteLine($"wrote {outputPath}");
            return ExitOk;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"error: {ex.Message}");
            Console.Error.WriteLine(ex.StackTrace);
            return ExitError;
        }
    }
}
