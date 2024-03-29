﻿using System;
using System.Collections.Generic;

namespace SharpMonkey
{
    using TokenType = String;

    public static class Constants
    {
        public const string Illegal = "ILLEGAL";
        public const string Eof = "EOF";
        public const string Ident = "IDENT"; // Identifiers
        public const string Int = "INT"; // 123456
        public const string Double = "DOUBLE"; // 1.234, 1e23

        public const string Assign = "=";
        public const string Plus = "+";
        public const string Minus = "-";
        public const string Asterisk = "*";
        public const string Slash = "/";
        public const string Bang = "!";

        public const string Lt = "<";
        public const string Gt = ">";
        public const string Eq = "==";
        public const string NotEq = "!=";
        public const string Increment = "++";
        public const string Decrement = "--";

        public const string Comma = ",";
        public const string QuestionMark = "?";
        public const string Colon = ":";
        public const string Semicolon = ";";

        public const string LParen = "(";
        public const string RParen = ")";
        public const string LBrace = "{";
        public const string RBrace = "}";
        public const string LBracket = "[";
        public const string RBracket = "]";

        public const string And = "&&";
        public const string Or = "||";

        public const string String = "String";

        // language keywords
        public const string Function = "FUNCTION";
        public const string Let = "LET";
        public const string True = "TRUE ";
        public const string False = "FALSE";
        public const string If = "IF";
        public const string Else = "ELSE";
        public const string Return = "RETURN";
        public const string While = "WHILE";
        public const string Null = "NULL";

        // In C# there is noway to declare an compile-time constant Dictionary
        public static readonly Dictionary<string, TokenType> Keywords = new()
        {
            {"fn", Function},
            {"let", Let},
            {"true", True},
            {"false", False},
            {"if", If},
            {"else", Else},
            {"return", Return},
            {"while", While},
            {"null", Null},
        };

        public static TokenType LookupIdent(string identifier)
        {
            if (Keywords.TryGetValue(identifier, out var res))
            {
                return res;
            }

            return Ident;
        }
    }


    public class Token
    {
        public TokenType Type; // 用枚举可能更加紧凑，但是String更方便调试
        public String Literal;

        public Token()
        {
            Type = Constants.Illegal;
            Literal = "INIT";
        }

        public Token(TokenType type, string literal)
        {
            Type = type;
            Literal = literal;
        }
    }
}