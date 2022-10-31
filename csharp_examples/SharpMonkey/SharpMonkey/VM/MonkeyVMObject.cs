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
        public Instructions Instructions { get; }
        private HashKey? _hashVal = null;

        public MonkeyCompiledFunction(Instructions instructions)
        {
            Instructions = instructions;
        }

        public string Type()
        {
            return ObjType.CompiledFunctionObject;
        }

        public string Inspect()
        {
            throw new NotImplementedException();
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