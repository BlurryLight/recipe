using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

namespace SharpMonkey.VM
{
    using Instructions = List<byte>;
    using Opcode = Byte;

    public enum OpConstants : byte
    {
        OpConstant,
    }

    public class Definition
    {
        public string Name;

        // 一条指令可能包含多个操作数，每个操作数有不同的字节表示
        public List<int> OperandWidths;

        public Definition(string name, List<int> operandWidths)
        {
            Name = name;
            OperandWidths = operandWidths;
        }
    }

    public static class OpcodeUtils
    {
        public static readonly Dictionary<Opcode, Definition> Definitions = new()
        {
            {(Opcode) OpConstants.OpConstant, new Definition(OpConstants.OpConstant.ToString(), new List<int> {2})}
        };

        public static Definition Lookup(Opcode code)
        {
            if (Definitions.TryGetValue(code, out Definition def))
            {
                return def;
            }

            string msg = $"Opcode {((OpConstants) code).ToString()} undefined";
            Debug.Fail(msg);
            Console.WriteLine(msg);
            return null;
        }

        /// <summary>
        /// 输入一个可读的Opcode表示，输出其字节码表示
        /// </summary>
        /// <returns></returns>
        public static List<byte> MakeBytes(Opcode op, params int[] operands)
        {
            var def = Lookup(op);

            var instructionLen = 1;
            foreach (var width in def.OperandWidths)
            {
                instructionLen += width;
            }

            Debug.Assert(operands.Length == def.OperandWidths.Count);

            var instruction = new List<byte>();
            instruction.Add(op);
            // 首位一定是指令
            for (int i = 0; i < operands.Length; i++)
            {
                var width = def.OperandWidths[i];
                var oprand = operands[i];
                switch (width)
                {
                    case 2:
                    {
                        Debug.Assert(oprand > 0 && oprand < ushort.MaxValue,
                            $"oprand should be in [{ushort.MinValue,ushort.MaxValue}] but is {oprand}");
                        byte[] bytes = BitConverter.GetBytes((ushort) oprand);
                        if (BitConverter.IsLittleEndian)
                        {
                            Array.Reverse(bytes);
                        }

                        instruction.AddRange(bytes);
                        break;
                    }
                    default:
                        Debug.Fail("Unsupported width");
                        break;
                }
            }

            return instruction;
        }
    }

    public class Code
    {
    }
}