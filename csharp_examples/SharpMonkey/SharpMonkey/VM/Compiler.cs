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
            //TODO: avoid add repeated constant
            // 思路: 通过一个HashDict记录每个常量的 Hashkey -> index关系，在已经存在的情况下返回Index
            // 但是这限制了常量池能够存储的类型(IMonkeyHash)，应该是可行的
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
                    Emit((byte) OpConstants.OpPop);
                    break;
                case Ast.InfixExpression exp:
                    // 这里只是展示了在编译成字节码的时候可以通过调整infix的压栈顺序节约 OpCode的一种可能
                    if (exp.Operator == Constants.Lt)
                    {
                        Compile(exp.Right);
                        Compile(exp.Left);
                        Emit((byte) OpConstants.OpGreaterThan);
                        break;
                    }

                    Compile(exp.Left);
                    // TODO: 这里，或者在VM的时候处理短路原则
                    Compile(exp.Right);
                    switch (exp.Operator)
                    {
                        case Constants.Plus:
                            Emit((byte) OpConstants.OpAdd);
                            break;
                        case Constants.Minus:
                            Emit((byte) OpConstants.OpSub);
                            break;
                        case Constants.Asterisk:
                            Emit((byte) OpConstants.OpMul);
                            break;
                        case Constants.Slash:
                            Emit((byte) OpConstants.OpDiv);
                            break;
                        case Constants.Gt:
                            Emit((byte) OpConstants.OpGreaterThan);
                            break;
                        case Constants.Eq:
                            Emit((byte) OpConstants.OpEqual);
                            break;
                        case Constants.NotEq:
                            Emit((byte) OpConstants.OpNotEqual);
                            break;
                        case Constants.And:
                            Emit((byte) OpConstants.OpAnd);
                            break;
                        case Constants.Or:
                            Emit((byte) OpConstants.OpOr);
                            break;
                        default:
                            throw new NotImplementedException($"not implemented for InfixOperator {exp.Operator}");
                    }

                    break;
                case Ast.IntegerLiteral exp:
                    var integer = new MonkeyInteger(exp.Value);
                    Emit((byte) OpConstants.OpConstant, AddConstant(integer));
                    break;
                case Ast.DoubleLiteral exp:
                    var floatVal = new MonkeyDouble(exp.Value);
                    Emit((byte) OpConstants.OpConstant, AddConstant(floatVal));
                    break;
                case Ast.BooleanLiteral exp:
                    Emit(exp.Value ? (byte) OpConstants.OpTrue : (byte) OpConstants.OpFalse);
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