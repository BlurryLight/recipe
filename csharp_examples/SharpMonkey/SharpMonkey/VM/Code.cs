using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace SharpMonkey.VM
{
    using Instructions = List<byte>;
    using Opcode = Byte;

    public enum OpConstants : byte
    {
        OpConstant,
        OpPop,
        OpAdd,
        OpSub,
        OpMul,
        OpDiv,
        OpTrue,
        OpFalse,
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
            {(Opcode) OpConstants.OpConstant, new Definition(OpConstants.OpConstant.ToString(), new List<int> {2})},
            {(Opcode) OpConstants.OpPop, new Definition(OpConstants.OpPop.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpAdd, new Definition(OpConstants.OpAdd.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpSub, new Definition(OpConstants.OpSub.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpMul, new Definition(OpConstants.OpMul.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpDiv, new Definition(OpConstants.OpDiv.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpTrue, new Definition(OpConstants.OpTrue.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpFalse, new Definition(OpConstants.OpFalse.ToString(), new List<int>())},
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

        public static List<byte> MakeBytes(OpConstants op, params int[] operands)
        {
            return MakeBytes((byte) op, operands);
        }

        /// <summary>
        /// 输入一个可读的Opcode表示，输出其字节码表示
        /// </summary>
        /// <returns></returns>
        public static List<byte> MakeBytes(Opcode op, params int[] operands)
        {
            var def = Lookup(op);
            Debug.Assert(operands.Length == def.OperandWidths.Count);

            var instruction = new List<byte> {op};
            // 首位一定是指令
            for (int i = 0; i < operands.Length; i++)
            {
                var width = def.OperandWidths[i];
                var operand = operands[i];
                switch (width)
                {
                    case 2:
                    {
                        if (operand < 0 || operand > ushort.MaxValue)
                        {
                            throw new Exception(
                                $"oprand should be in [{ushort.MinValue},{ushort.MaxValue}] but is {operand}");
                        }

                        byte[] bytes = BitConverter.GetBytes((ushort) operand);
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

        private static ReadOnlySpan<byte> SubOperands(Instructions ins, int offset, int count)
        {
            var span = CollectionsMarshal.AsSpan(ins.GetRange(offset, count));
            if (BitConverter.IsLittleEndian)
            {
                span.Reverse();
            }

            return span;
        }

        public static ushort ReadUint16(Instructions ins, int offset)
        {
            return BitConverter.ToUInt16(SubOperands(ins, offset, 2));
        }

        public static Tuple<List<int>, int> ReadOperands(Instructions ins, Definition def)
        {
            var operands = new List<int>();
            var offset = 0;
            for (int i = 0; i < def.OperandWidths.Count; i++)
            {
                var width = def.OperandWidths[i];
                switch (width)
                {
                    case 2:
                        operands.Add(BitConverter.ToUInt16(SubOperands(ins, i, 2)));
                        break;
                }

                offset += width;
            }

            return new Tuple<List<int>, int>(operands, offset);
        }

        /// <summary>
        /// 把字节码翻译为可读格式。 格式为 <指令开始的字节> <Op名> <操作数>
        /// 比如 0006 OpConstant 65535 
        /// </summary>
        /// <param name="ins"></param>
        /// <returns></returns>
        public static string DecodeInstructions(Instructions ins)
        {
            var sb = new StringBuilder();
            for (int i = 0; i < ins.Count;)
            {
                var def = Lookup(ins[i]);
                Debug.Assert(def != null);
                var (operands, offset) = ReadOperands(ins.Skip(i + 1).ToList(), def);
                sb.Append($"{i:0000} {FormatIns(def, operands)}\n");
                // 格式化一条指令，以及它的所有操作符，移动偏移量到下一条指令的起始处
                i += offset + 1;
            }

            return sb.ToString();
        }

        private static string FormatIns(Definition def, List<int> operands)
        {
            var operandCount = def.OperandWidths.Count;
            Debug.Assert(operandCount == operands.Count);
            switch (operands.Count)
            {
                case 0:
                    return $"{def.Name} NULL";
                case 1:
                    return $"{def.Name} {operands[0]}";
            }

            return $"Error: unhandled operandCount for {def.Name}\n";
        }
    }

    public class Code
    {
    }
}