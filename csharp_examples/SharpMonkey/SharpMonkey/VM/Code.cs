﻿using System;
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
        OpEqual,
        OpNotEqual,
        OpGreaterThan,
        OpAnd,
        OpOr,
        OpMinus,
        OpBang,
        OpIncrement,
        OpDecrement,
        OpJumpNotTruthy, // JNZ in X86
        OpJump, // JMP in X86
        OpNull,
        OpSetGlobal,
        OpGetGlobal,
        OpArray,
        OpHash,
        OpIndex,
        OpCall,
        OpReturnValue,
        OpReturn,
        OpSetLocal,
        OpGetLocal,
        OpGetBuiltin,
        OpClosure,
        OpGetFree,

        // 闭包递归的问题是
        // let obj  = fn(){obj();};
        // 当编译最内层的`obj()`时候，其符号并不存在，编译器将其解析为一个自由变量，调用OpGetFree
        // 在外部编译  fn(){obj();}时，我们需要填充自由变量，也就是先在栈中压入自由变量，再调用OpClosure打成闭包
        // 问题是内层的obj 其实引用的是外层的OpClosure以后的产物，而填充OpGetLocal发生在OpClosure以前
        // 所以需要一条额外的指令来处理这种情况，OpCurretClosure用于在闭包内层调用闭包本身的情况
        // 最简单的示例见 `TestRecursiveClosures`的第一个测试用例
        OpCurrentClosure,

        OpAssignGlobal,
        OpAssignLocal,
        OpIndexSet,
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
        public static readonly int OP_INCREMENT_PREFIX = 0x00;
        public static readonly int OP_INCREMENT_POSTFIX = 0xFF;

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
            {(Opcode) OpConstants.OpEqual, new Definition(OpConstants.OpEqual.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpNotEqual, new Definition(OpConstants.OpNotEqual.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpGreaterThan, new Definition(OpConstants.OpGreaterThan.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpAnd, new Definition(OpConstants.OpAnd.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpOr, new Definition(OpConstants.OpOr.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpMinus, new Definition(OpConstants.OpMinus.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpBang, new Definition(OpConstants.OpBang.ToString(), new List<int>())},
            // 第一个字节决定 是Prefix还是PostFix
            {(Opcode) OpConstants.OpIncrement, new Definition(OpConstants.OpIncrement.ToString(), new List<int>() {1})},
            {(Opcode) OpConstants.OpDecrement, new Definition(OpConstants.OpDecrement.ToString(), new List<int>() {1})},

            {(Opcode) OpConstants.OpJump, new Definition(OpConstants.OpJump.ToString(), new List<int> {2})},
            {
                (Opcode) OpConstants.OpJumpNotTruthy,
                new Definition(OpConstants.OpJumpNotTruthy.ToString(), new List<int> {2})
            },
            {(Opcode) OpConstants.OpNull, new Definition(OpConstants.OpNull.ToString(), new List<int>())},
            {(Opcode) OpConstants.OpSetGlobal, new Definition(OpConstants.OpSetGlobal.ToString(), new List<int> {2})},
            {(Opcode) OpConstants.OpGetGlobal, new Definition(OpConstants.OpGetGlobal.ToString(), new List<int> {2})},
            {(Opcode) OpConstants.OpArray, new Definition(OpConstants.OpArray.ToString(), new List<int> {2})},
            {(Opcode) OpConstants.OpHash, new Definition(OpConstants.OpHash.ToString(), new List<int> {2})},
            {(Opcode) OpConstants.OpIndex, new Definition(OpConstants.OpIndex.ToString(), new List<int> { })},
            // Call的操作数是 这个函数调用有多少个参数
            {(Opcode) OpConstants.OpCall, new Definition(OpConstants.OpCall.ToString(), new List<int> {1})},
            {(Opcode) OpConstants.OpReturn, new Definition(OpConstants.OpReturn.ToString(), new List<int> { })},
            {
                (Opcode) OpConstants.OpReturnValue,
                new Definition(OpConstants.OpReturnValue.ToString(), new List<int> { })
            },
            // only support up to 255 local variables
            {(Opcode) OpConstants.OpSetLocal, new Definition(OpConstants.OpSetLocal.ToString(), new List<int> {1})},
            {(Opcode) OpConstants.OpGetLocal, new Definition(OpConstants.OpGetLocal.ToString(), new List<int> {1})},
            {(Opcode) OpConstants.OpGetBuiltin, new Definition(OpConstants.OpGetBuiltin.ToString(), new List<int> {1})},
            {(Opcode) OpConstants.OpGetFree, new Definition(OpConstants.OpGetFree.ToString(), new List<int> {1})},

            // 第一个参数表示 闭包所包含的CompiledFunction在常量池的哪一个位置
            // 第二个参数代表这个闭包捕获了多少变量(自由变量)
            {(Opcode) OpConstants.OpClosure, new Definition(OpConstants.OpClosure.ToString(), new List<int> {2, 1})},
            {
                (Opcode) OpConstants.OpCurrentClosure,
                new Definition(OpConstants.OpCurrentClosure.ToString(), new List<int> { })
            },
            {
                (Opcode) OpConstants.OpAssignGlobal,
                new Definition(OpConstants.OpAssignGlobal.ToString(), new List<int> {2})
            },
            {
                (Opcode) OpConstants.OpAssignLocal,
                new Definition(OpConstants.OpAssignLocal.ToString(), new List<int> {1})
            },

            {(Opcode) OpConstants.OpIndexSet, new Definition(OpConstants.OpIndexSet.ToString(), new List<int> { })},
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
                    case 1:
                    {
                        if (operand < 0 || operand > Byte.MaxValue)
                        {
                            throw new Exception(
                                $"oprand should be in [{Byte.MinValue},{Byte.MaxValue}] but is {operand}");
                        }

                        instruction.Add((byte) operand);
                        break;
                    }
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
                    case 1:
                        operands.Add(ins[offset]);
                        break;
                    case 2:
                        operands.Add(BitConverter.ToUInt16(SubOperands(ins, offset, 2)));
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
                case 2:
                    return $"{def.Name} {operands[0]} {operands[1]}";
            }

            return $"Error: unhandled operandCount for {def.Name}\n";
        }
    }

    public class Code
    {
    }
}