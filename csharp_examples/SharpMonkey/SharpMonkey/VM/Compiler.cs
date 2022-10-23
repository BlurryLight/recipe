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
        private readonly List<IMonkeyObject> _constantsPool;

        /// <summary>
        /// 添加一个常量对象到常量池里,并返回对象的索引
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        private int AddConstant(IMonkeyObject obj)
        {
            _constantsPool.Add(obj);
            return _constantsPool.Count - 1;
        }

        /// <summary>
        /// 添加一条字节指令到字节码内，并返回该条指令的起始位置(字节数)
        /// </summary>
        /// <param name="ins"></param>
        /// <returns></returns>
        private int AddInstruction(List<byte> ins)
        {
            var pos = _instructions.Count;
            _instructions.AddRange(ins);
            return pos;
        }

        private int Emit(Opcode op, params int[] operands)
        {
            var ins = OpcodeUtils.MakeBytes(op, operands);
            var pos = AddInstruction(ins);
            return pos;
        }

        public Compiler()
        {
            _instructions = new Instructions();
            _constantsPool = new List<IMonkeyObject>();
        }

        public void Compile(Ast.INode node)
        {
            switch (node)
            {
                case Ast.MonkeyProgram program:
                    foreach (var stmt in program.Statements)
                    {
                        Compile(stmt);
                    }

                    break;
                case Ast.ExpressionStatement exp:
                    Compile(exp.Expression);
                    break;
                case Ast.InfixExpression exp:
                    Compile(exp.Left);
                    Compile(exp.Right);
                    switch (exp.Operator)
                    {
                        case "+":
                            Emit((byte) OpConstants.OpAdd);
                            break;
                        default:
                            throw new NotImplementedException($"not implemented for InfixOperator {exp.Operator}");
                    }

                    break;
                case Ast.IntegerLiteral exp:
                    var integer = new MonkeyInteger(exp.Value);
                    Emit((byte) OpConstants.OpConstant, AddConstant(integer));
                    break;

                default:
                    throw new NotImplementedException($"not implemented for type {node.ToPrintableString()}");
            }
        }

        public Bytecode Bytecode()
        {
            return new Bytecode()
            {
                Instructions = _instructions,
                Constants = _constantsPool
            };
        }
    }
}