using System;
using System.Collections.Generic;

namespace SharpMonkey.VM
{
    using Instructions = List<byte>;
    using Opcode = Byte;

    public struct Bytecode
    {
        public Instructions Instructions;
        public List<IMonkeyObject> Constants;
    }

    public class Compiler
    {
        private Instructions _instructions;
        private List<IMonkeyObject> _constants;

        public Compiler()
        {
            _instructions = new Instructions();
            _constants = new List<IMonkeyObject>();
        }

        public void Compile(Ast.MonkeyProgram program)
        {
            // throw new Exception();
        }

        public Bytecode Bytecode()
        {
            return new Bytecode()
            {
                Instructions = _instructions,
                Constants = _constants
            };
        }
    }
}