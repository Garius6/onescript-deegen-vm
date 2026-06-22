using System;
using OneScript.StandardLibrary;
using OneScript.StandardLibrary.Binary;
using ScriptEngine.Compiler;
using ScriptEngine.HostedScript;
using ScriptEngine.HostedScript.Extensions;
using ScriptEngine.Hosting;
using ScriptEngine.Machine;

namespace OsbcExport;

internal static class Compilation
{
    public static StackRuntimeModule CompileFromFile(string path)
    {
        var builder = DefaultEngineBuilder.Create()
            .SetupConfiguration(p => p
                .UseSystemConfigFile()
                .UseEntrypointConfigFile(path)
                .UseEnvironmentVariableConfig("OSCRIPT_CONFIG"));

        builder.SetDefaultOptions()
               .UseImports()
               .UseDefaultHosting()
               .UseNativeRuntime()
               .UseEventHandlers();

        builder.SetupEnvironment(env =>
        {
            env.AddStandardLibrary()
               .UseTemplateFactory(new DefaultTemplatesFactory(env.Services.Resolve<IBinaryDataMemoryLimit>()));
        });

        var hosted = new HostedScriptEngine(builder.Build());
        hosted.Initialize();

        var source   = hosted.Loader.FromFile(path);
        var compiler = hosted.GetCompilerService();
        hosted.SetGlobalEnvironment(new SilentHost(), source);

        var compiled = compiler.Compile(source, hosted.Engine.NewProcess());
        if (compiled is not StackRuntimeModule stack)
        {
            throw new InvalidOperationException(
                $"Expected StackRuntimeModule, got {compiled?.GetType().FullName ?? "null"}. " +
                "Ensure the engine is configured for the stack runtime backend.");
        }
        return stack;
    }
}
