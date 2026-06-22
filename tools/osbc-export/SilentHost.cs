using System;
using OneScript.StandardLibrary;
using ScriptEngine.HostedScript;

namespace OsbcExport;

internal sealed class SilentHost : IHostApplication
{
    public void Echo(string str, MessageStatusEnum status = MessageStatusEnum.Ordinary) { }
    public void ShowExceptionInfo(Exception exc) => Console.Error.WriteLine(exc.Message);
    public bool InputString(out string result, string prompt, int maxLen, bool multiline)
    {
        result = string.Empty;
        return false;
    }
    public string[] GetCommandLineArguments() => Array.Empty<string>();
}
