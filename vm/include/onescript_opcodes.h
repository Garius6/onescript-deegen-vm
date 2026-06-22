// Generated from tools/opcodes.csv. DO NOT EDIT.
#pragma once
#include <cstdint>

namespace onescript {
enum class OpCode : uint8_t {
    Nop = 0,
    PushVar = 1,
    PushConst = 2,
    PushInt = 3,
    PushBool = 4,
    PushUndef = 5,
    PushNull = 6,
    PushLoc = 7,
    PushRef = 8,
    LoadVar = 9,
    LoadLoc = 10,
    AssignRef = 11,
    Add = 12,
    Sub = 13,
    Mul = 14,
    Div = 15,
    Mod = 16,
    Neg = 17,
    Equals = 18,
    Less = 19,
    Greater = 20,
    LessOrEqual = 21,
    GreaterOrEqual = 22,
    NotEqual = 23,
    Not = 24,
    And = 25,
    Or = 26,
    CallFunc = 27,
    CallProc = 28,
    ArgNum = 29,
    PushDefaultArg = 30,
    ResolveProp = 31,
    ResolveMethodProc = 32,
    ResolveMethodFunc = 33,
    Jmp = 34,
    JmpFalse = 35,
    PushIndexed = 36,
    Return = 37,
    JmpCounter = 38,
    Inc = 39,
    NewInstance = 40,
    NewFunc = 41,
    PushIterator = 42,
    IteratorNext = 43,
    StopIterator = 44,
    BeginTry = 45,
    EndTry = 46,
    RaiseException = 47,
    LineNum = 48,
    MakeRawValue = 49,
    MakeBool = 50,
    PushTmp = 51,
    PopTmp = 52,
    Execute = 53,
    AddHandler = 54,
    RemoveHandler = 55,
    ExitTry = 56,
    Eval = 57,
    Bool = 58,
    Number = 59,
    Str = 60,
    Date = 61,
    Type = 62,
    ValType = 63,
    StrLen = 64,
    TrimL = 65,
    TrimR = 66,
    TrimLR = 67,
    Left = 68,
    Right = 69,
    Mid = 70,
    StrPos = 71,
    UCase = 72,
    LCase = 73,
    TCase = 74,
    Chr = 75,
    ChrCode = 76,
    EmptyStr = 77,
    StrReplace = 78,
    StrGetLine = 79,
    StrLineCount = 80,
    StrEntryCount = 81,
    Year = 82,
    Month = 83,
    Day = 84,
    Hour = 85,
    Minute = 86,
    Second = 87,
    BegOfWeek = 88,
    BegOfYear = 89,
    BegOfMonth = 90,
    BegOfDay = 91,
    BegOfHour = 92,
    BegOfMinute = 93,
    BegOfQuarter = 94,
    EndOfWeek = 95,
    EndOfYear = 96,
    EndOfMonth = 97,
    EndOfDay = 98,
    EndOfHour = 99,
    EndOfMinute = 100,
    EndOfQuarter = 101,
    WeekOfYear = 102,
    DayOfYear = 103,
    DayOfWeek = 104,
    AddMonth = 105,
    CurrentDate = 106,
    Integer = 107,
    Round = 108,
    Log = 109,
    Log10 = 110,
    Sin = 111,
    Cos = 112,
    Tan = 113,
    ASin = 114,
    ACos = 115,
    ATan = 116,
    Exp = 117,
    Pow = 118,
    Sqrt = 119,
    Min = 120,
    Max = 121,
    Format = 122,
    ExceptionInfo = 123,
    ExceptionDescr = 124,
    ModuleInfo = 125,
};

constexpr const char* opcode_name(OpCode op) {
    switch (op) {
        case OpCode::Nop: return "Nop";
        case OpCode::PushVar: return "PushVar";
        case OpCode::PushConst: return "PushConst";
        case OpCode::PushInt: return "PushInt";
        case OpCode::PushBool: return "PushBool";
        case OpCode::PushUndef: return "PushUndef";
        case OpCode::PushNull: return "PushNull";
        case OpCode::PushLoc: return "PushLoc";
        case OpCode::PushRef: return "PushRef";
        case OpCode::LoadVar: return "LoadVar";
        case OpCode::LoadLoc: return "LoadLoc";
        case OpCode::AssignRef: return "AssignRef";
        case OpCode::Add: return "Add";
        case OpCode::Sub: return "Sub";
        case OpCode::Mul: return "Mul";
        case OpCode::Div: return "Div";
        case OpCode::Mod: return "Mod";
        case OpCode::Neg: return "Neg";
        case OpCode::Equals: return "Equals";
        case OpCode::Less: return "Less";
        case OpCode::Greater: return "Greater";
        case OpCode::LessOrEqual: return "LessOrEqual";
        case OpCode::GreaterOrEqual: return "GreaterOrEqual";
        case OpCode::NotEqual: return "NotEqual";
        case OpCode::Not: return "Not";
        case OpCode::And: return "And";
        case OpCode::Or: return "Or";
        case OpCode::CallFunc: return "CallFunc";
        case OpCode::CallProc: return "CallProc";
        case OpCode::ArgNum: return "ArgNum";
        case OpCode::PushDefaultArg: return "PushDefaultArg";
        case OpCode::ResolveProp: return "ResolveProp";
        case OpCode::ResolveMethodProc: return "ResolveMethodProc";
        case OpCode::ResolveMethodFunc: return "ResolveMethodFunc";
        case OpCode::Jmp: return "Jmp";
        case OpCode::JmpFalse: return "JmpFalse";
        case OpCode::PushIndexed: return "PushIndexed";
        case OpCode::Return: return "Return";
        case OpCode::JmpCounter: return "JmpCounter";
        case OpCode::Inc: return "Inc";
        case OpCode::NewInstance: return "NewInstance";
        case OpCode::NewFunc: return "NewFunc";
        case OpCode::PushIterator: return "PushIterator";
        case OpCode::IteratorNext: return "IteratorNext";
        case OpCode::StopIterator: return "StopIterator";
        case OpCode::BeginTry: return "BeginTry";
        case OpCode::EndTry: return "EndTry";
        case OpCode::RaiseException: return "RaiseException";
        case OpCode::LineNum: return "LineNum";
        case OpCode::MakeRawValue: return "MakeRawValue";
        case OpCode::MakeBool: return "MakeBool";
        case OpCode::PushTmp: return "PushTmp";
        case OpCode::PopTmp: return "PopTmp";
        case OpCode::Execute: return "Execute";
        case OpCode::AddHandler: return "AddHandler";
        case OpCode::RemoveHandler: return "RemoveHandler";
        case OpCode::ExitTry: return "ExitTry";
        case OpCode::Eval: return "Eval";
        case OpCode::Bool: return "Bool";
        case OpCode::Number: return "Number";
        case OpCode::Str: return "Str";
        case OpCode::Date: return "Date";
        case OpCode::Type: return "Type";
        case OpCode::ValType: return "ValType";
        case OpCode::StrLen: return "StrLen";
        case OpCode::TrimL: return "TrimL";
        case OpCode::TrimR: return "TrimR";
        case OpCode::TrimLR: return "TrimLR";
        case OpCode::Left: return "Left";
        case OpCode::Right: return "Right";
        case OpCode::Mid: return "Mid";
        case OpCode::StrPos: return "StrPos";
        case OpCode::UCase: return "UCase";
        case OpCode::LCase: return "LCase";
        case OpCode::TCase: return "TCase";
        case OpCode::Chr: return "Chr";
        case OpCode::ChrCode: return "ChrCode";
        case OpCode::EmptyStr: return "EmptyStr";
        case OpCode::StrReplace: return "StrReplace";
        case OpCode::StrGetLine: return "StrGetLine";
        case OpCode::StrLineCount: return "StrLineCount";
        case OpCode::StrEntryCount: return "StrEntryCount";
        case OpCode::Year: return "Year";
        case OpCode::Month: return "Month";
        case OpCode::Day: return "Day";
        case OpCode::Hour: return "Hour";
        case OpCode::Minute: return "Minute";
        case OpCode::Second: return "Second";
        case OpCode::BegOfWeek: return "BegOfWeek";
        case OpCode::BegOfYear: return "BegOfYear";
        case OpCode::BegOfMonth: return "BegOfMonth";
        case OpCode::BegOfDay: return "BegOfDay";
        case OpCode::BegOfHour: return "BegOfHour";
        case OpCode::BegOfMinute: return "BegOfMinute";
        case OpCode::BegOfQuarter: return "BegOfQuarter";
        case OpCode::EndOfWeek: return "EndOfWeek";
        case OpCode::EndOfYear: return "EndOfYear";
        case OpCode::EndOfMonth: return "EndOfMonth";
        case OpCode::EndOfDay: return "EndOfDay";
        case OpCode::EndOfHour: return "EndOfHour";
        case OpCode::EndOfMinute: return "EndOfMinute";
        case OpCode::EndOfQuarter: return "EndOfQuarter";
        case OpCode::WeekOfYear: return "WeekOfYear";
        case OpCode::DayOfYear: return "DayOfYear";
        case OpCode::DayOfWeek: return "DayOfWeek";
        case OpCode::AddMonth: return "AddMonth";
        case OpCode::CurrentDate: return "CurrentDate";
        case OpCode::Integer: return "Integer";
        case OpCode::Round: return "Round";
        case OpCode::Log: return "Log";
        case OpCode::Log10: return "Log10";
        case OpCode::Sin: return "Sin";
        case OpCode::Cos: return "Cos";
        case OpCode::Tan: return "Tan";
        case OpCode::ASin: return "ASin";
        case OpCode::ACos: return "ACos";
        case OpCode::ATan: return "ATan";
        case OpCode::Exp: return "Exp";
        case OpCode::Pow: return "Pow";
        case OpCode::Sqrt: return "Sqrt";
        case OpCode::Min: return "Min";
        case OpCode::Max: return "Max";
        case OpCode::Format: return "Format";
        case OpCode::ExceptionInfo: return "ExceptionInfo";
        case OpCode::ExceptionDescr: return "ExceptionDescr";
        case OpCode::ModuleInfo: return "ModuleInfo";
        default: return "<unknown>";
    }
}

constexpr uint32_t kOpCodeCount = 126;
}  // namespace onescript
