using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using OneScript.Contexts;
using OneScript.Values;
using ScriptEngine.Machine;

namespace OsbcExport;

internal sealed class OsbinWriter
{
    private const ushort FormatVersion = 1;
    private const BindingFlags InstanceAccess =
        BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic;

    private readonly BinaryWriter _w;

    public OsbinWriter(Stream stream)
    {
        _w = new BinaryWriter(stream, Encoding.UTF8, leaveOpen: true);
    }

    public void WriteModule(StackRuntimeModule mod)
    {
        // Header
        _w.Write(new[] { (byte)'O', (byte)'S', (byte)'B', (byte)'N' });
        _w.Write(FormatVersion);
        _w.Write((ushort)0);                  // flags
        _w.Write(mod.EntryMethodIndex);

        WriteConstants(mod);
        WriteIdentifiers(mod);
        WriteBindings(GetInternalList<ModuleSymbolBinding>(mod, "VariableRefs"));
        WriteBindings(GetInternalList<ModuleSymbolBinding>(mod, "MethodRefs"));
        WriteMethods(mod);
        WriteCode(mod);

        _w.Write((uint)0);                    // CRC32 placeholder
    }

    private static IList<T> GetInternalList<T>(object owner, string name)
    {
        var prop = owner.GetType().GetProperty(name, InstanceAccess)
                   ?? throw new InvalidOperationException(
                          $"{owner.GetType().Name}.{name} not found via reflection");
        return (IList<T>)prop.GetValue(owner)!;
    }

    private void WriteConstants(StackRuntimeModule mod)
    {
        _w.Write((uint)mod.Constants.Count);
        foreach (var constant in mod.Constants)
        {
            var (type, presentation) = ClassifyPrimitive(constant);
            _w.Write((byte)type);
            WriteLString(presentation);
        }
    }

    private static (DataType, string) ClassifyPrimitive(BslPrimitiveValue value)
    {
        switch (value)
        {
            case BslUndefinedValue:
                return (DataType.Undefined, string.Empty);
            case BslNullValue:
                return (DataType.Null, string.Empty);
            case BslBooleanValue b:
                return (DataType.Boolean, ((bool)b) ? "True" : "False");
            case BslNumericValue n:
                return (DataType.Number,
                        ((decimal)n).ToString(System.Globalization.CultureInfo.InvariantCulture));
            case BslStringValue s:
                return (DataType.String, (string)s);
            case BslDateValue d:
                return (DataType.Date, ((DateTime)d).ToString("yyyy-MM-ddTHH:mm:ss"));
            default:
                throw new NotSupportedException(
                    $"Unsupported primitive constant: {value?.GetType().FullName ?? "null"}");
        }
    }

    private void WriteIdentifiers(StackRuntimeModule mod)
    {
        var ids = GetInternalList<string>(mod, "Identifiers");
        _w.Write((uint)ids.Count);
        foreach (var id in ids) WriteLString(id);
    }

    private void WriteBindings(IList<ModuleSymbolBinding> bindings)
    {
        _w.Write((uint)bindings.Count);
        foreach (var b in bindings)
        {
            _w.Write((byte)b.Kind);
            _w.Write(b.ScopeIndex);
            _w.Write(b.MemberNumber);
            WriteLString(b.Target?.ToString() ?? string.Empty);
        }
    }

    private void WriteMethods(StackRuntimeModule mod)
    {
        var methods = mod.Methods;
        _w.Write((uint)methods.Count);
        foreach (var info in methods)
        {
            var runtime = ExtractRuntimeMethod(info);
            var sig     = runtime.Signature;

            WriteLString(sig.Name ?? info.Name ?? string.Empty);
            _w.Write(sig.IsFunction);
            _w.Write(sig.ArgCount);

            var ps = sig.Params ?? Array.Empty<ParameterDefinition>();
            _w.Write((uint)ps.Length);
            foreach (var p in ps)
            {
                _w.Write(p.IsByValue);
                _w.Write(p.HasDefaultValue);
                _w.Write(p.HasDefaultValue
                    ? p.DefaultValueIndex
                    : ParameterDefinition.UNDEFINED_VALUE_INDEX);
            }

            _w.Write(runtime.EntryPoint);

            var locals = runtime.LocalVariables ?? Array.Empty<string>();
            _w.Write((uint)locals.Length);
            foreach (var v in locals) WriteLString(v ?? string.Empty);
        }
    }

    private static MachineMethod ExtractRuntimeMethod(BslScriptMethodInfo info)
    {
        var method = info.GetType().GetMethod("GetRuntimeMethod", InstanceAccess)
                     ?? throw new InvalidOperationException(
                            $"GetRuntimeMethod missing on {info.GetType().FullName}");
        return (MachineMethod)method.Invoke(info, null)!;
    }

    private void WriteCode(StackRuntimeModule mod)
    {
        _w.Write((uint)mod.Code.Count);
        foreach (var cmd in mod.Code)
        {
            var name = Enum.GetName(typeof(OperationCode), cmd.Code)
                       ?? throw new InvalidOperationException($"Unknown opcode {cmd.Code}");
            if (!OpCodeMapping.NameToValue.TryGetValue(name, out var encoded))
            {
                throw new InvalidOperationException(
                    $"Opcode {name} missing from generated mapping. " +
                    "Regenerate via scripts/gen_opcode_headers.sh.");
            }
            _w.Write(encoded);
            _w.Write(cmd.Argument);
        }
    }

    private void WriteLString(string s)
    {
        var bytes = Encoding.UTF8.GetBytes(s ?? string.Empty);
        _w.Write((uint)bytes.Length);
        _w.Write(bytes);
    }
}
