using System;
using System.Collections.Generic;

namespace SharpMonkey
{
    using Instructions = List<byte>;
    using Opcode = Byte;

    public static partial class ObjType
    {
        public const string CompiledFunctionObject = "CompiledFunctionObject";
    }

    public class MonkeyCompiledFunction : IMonkeyObject, IMonkeyHash
    {
#if DEBUG
        public string Source;
#endif
        public Instructions Instructions { get; }
        public int NumLocals;
        private HashKey? _hashVal = null;

        public MonkeyCompiledFunction(Instructions instructions)
        {
            Instructions = instructions;
            NumLocals = 0;
        }

        public string Type()
        {
            return ObjType.CompiledFunctionObject;
        }

        public string Inspect()
        {
#if DEBUG
            return Source;
#endif
            return "CompiledFunctionObject";
        }

        public HashKey HashKey()
        {
            if (_hashVal.HasValue) return _hashVal.Value;

            _hashVal = new HashKey()
            {
                KeyObjType = Type(),
                // TODO: verify the implementation
                HashValue = Instructions.GetHashCode()
            };
            return _hashVal.Value;
        }
    }
}