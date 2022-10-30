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

    public struct EmittedInstruction
    {
        public Opcode Op;
        public int Pos;
    }

    public class Compiler
    {
        private readonly Instructions _instructions;
        public readonly List<IMonkeyObject> ConstantsPool;
        public readonly Dictionary<HashKey, int> ConstantsPoolIndex;
        private EmittedInstruction _lastInstruction;
        private EmittedInstruction _previousInstruction;
        public readonly SymbolTable SymbolTable;

        /// <summary>
        /// 添加一个常量对象到常量池里,并返回对象的索引
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        private int AddConstant(IMonkeyObject obj)
        {
            if (obj is not IMonkeyHash hashObj)
            {
                throw new Exception($"ConstantPool object must be IMonkeyHash");
            }

            var hashKey = hashObj.HashKey();
            if (ConstantsPoolIndex.TryGetValue(hashKey, out int index))
            {
                return index;
            }

            ConstantsPool.Add(obj);
            var idx = ConstantsPool.Count - 1;
            ConstantsPoolIndex[hashKey] = idx;
            return idx;
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
            SetLastInstruction(op, pos);
            return pos;
        }

        private void SetLastInstruction(Opcode op, int pos)
        {
            _previousInstruction = _lastInstruction;
            _lastInstruction.Op = op;
            _lastInstruction.Pos = pos;
        }

        private void RemoveLastOpPop()
        {
            _instructions.RemoveAt(_instructions.Count - 1);
            _lastInstruction = _previousInstruction;
        }

        private void RemoveInnerBlockOpPop()
        {
            if (_lastInstruction.Op == (byte) OpConstants.OpPop)
            {
                RemoveLastOpPop();
            }
        }

        // 把从StartPos开始的字节码，替换为ins内的字节码
        private void ReplaceInstructionsRange(int startPos, Instructions ins)
        {
            // C# has no support for Buffer.Copy for List<byte>
            for (int i = 0; i < ins.Count; i++)
            {
                _instructions[startPos + i] = ins[i];
            }
        }

        private void ChangeOperand(int opPos, params int[] operands)
        {
            var op = _instructions[opPos];
            var newIns = OpcodeUtils.MakeBytes(op, operands);
            ReplaceInstructionsRange(opPos, newIns);
        }

        public Compiler()
        {
            SymbolTable = new SymbolTable();
            _instructions = new Instructions();
            ConstantsPool = new List<IMonkeyObject>();
            ConstantsPoolIndex = new Dictionary<HashKey, int>();
            _lastInstruction = new EmittedInstruction();
            _previousInstruction = new EmittedInstruction();
        }

        public Compiler(List<IMonkeyObject> constantsPool, Dictionary<HashKey, int> constantsPoolIndex,
            SymbolTable symbolTable)
        {
            ConstantsPool = constantsPool;
            ConstantsPoolIndex = constantsPoolIndex;
            SymbolTable = symbolTable;

            _instructions = new Instructions();
            _lastInstruction = new EmittedInstruction();
            _previousInstruction = new EmittedInstruction();
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

                    if (exp.Operator == Constants.And)
                    {
                        // 由于短路原则，And运算符需要特殊处理
                        // 详细分析逻辑看` input = "true && false;" 这个测试用例处的分析
                        Compile(exp.Left);
                        var ph =
                            Emit((byte) OpConstants.OpJumpNotTruthy,
                                54321);
                        Emit((byte) OpConstants.OpTrue);
                        Compile(exp.Right);
                        Emit((byte) OpConstants.OpAnd);
                        var ph2 =
                            Emit((byte) OpConstants.OpJump,
                                54321);
                        var pos1 = Emit((byte) OpConstants.OpFalse);
                        ChangeOperand(ph, pos1);
                        ChangeOperand(ph2, pos1 + 1);
                        break;
                    }

                    Compile(exp.Left);
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
                case Ast.StringLiteral exp:
                    var stringVal = new MonkeyString(exp.Value);
                    Emit((byte) OpConstants.OpConstant, AddConstant(stringVal));
                    break;
                case Ast.PrefixExpression exp:
                    Compile(exp.Right);
                    switch (exp.Operator)
                    {
                        case Constants.Bang:
                            Emit((byte) OpConstants.OpBang);
                            break;
                        case Constants.Minus:
                            Emit((byte) OpConstants.OpMinus);
                            break;
                        case Constants.Increment:
                            Emit((byte) OpConstants.OpIncrement, OpcodeUtils.OP_INCREMENT_PREFIX);
                            break;
                        default:
                            throw new NotImplementedException($"not implemented for PrefixOperator{exp.Operator}");
                    }

                    break;

                case Ast.IfExpression exp:
                    CompileIfExpression(exp);
                    break;
                case Ast.ConditionalExpression exp:
                    CompileConditionExpression(exp);
                    break;
                case Ast.BlockStatement stmts:
                    foreach (var stmt in stmts.Statements)
                    {
                        Compile(stmt);
                    }

                    break;
                case Ast.NullLiteral:
                    Emit((byte) OpConstants.OpNull);
                    break;
                case Ast.LetStatement stmt:
                    Compile(stmt.Value);
                    var symbol = SymbolTable.Define(stmt.Name.Value);
                    Emit((byte) OpConstants.OpSetGlobal, symbol.Index);
                    break;
                case Ast.Identifier ident:
                    symbol = SymbolTable.Resolve(ident.Value);
                    Emit((byte) OpConstants.OpGetGlobal, symbol.Index);
                    break;
                default:
                    throw new NotImplementedException(
                        $"not implemented for type {node.GetType()}:{node.ToPrintableString()}");
            }
        }

        private void CompileIfExpression(Ast.IfExpression exp)
        {
            Compile(exp.Condition);
            var jumpConsequencePlaceholder =
                Emit((byte) OpConstants.OpJumpNotTruthy,
                    54321); // fill an rubissh value since we doesn't know where to jump
            // 对于 if(true){ 10; }这样的语句, Compiler会插入两个OpPop，一次是10;一次是 if()..,
            // 因为if是带有返回值的ExressionStatement
            // 所以如果If内部的Block里带有返回值，需要清除内层的OpPop， Consequence/Alternative两个内层都要清理，只保留外层的 if()else{}的那个OpPop，以维持栈平衡
            Compile(exp.Consequence);
            RemoveInnerBlockOpPop();
            var jumpAlternativePlaceholder =
                Emit((byte) OpConstants.OpJump,
                    54321);
            var alternativeBeginPos = _instructions.Count;
            ChangeOperand(jumpConsequencePlaceholder, alternativeBeginPos);
            if (exp.Alternative == null)
            {
                Emit((byte) OpConstants.OpNull);
            }
            else
            {
                Compile(exp.Alternative);
                RemoveInnerBlockOpPop();
            }

            var alternativeEndPos = _instructions.Count;
            ChangeOperand(jumpAlternativePlaceholder, alternativeEndPos);
        }

        // 逻辑和IfExpression非常相似,可以复用逻辑
        private void CompileConditionExpression(Ast.ConditionalExpression exp)
        {
            Compile(exp.Condition);
            var jumpConsequencePlaceholder =
                Emit((byte) OpConstants.OpJumpNotTruthy,
                    54321);
            Compile(exp.ThenArm);
            RemoveInnerBlockOpPop();
            var jumpAlternativePlaceholder =
                Emit((byte) OpConstants.OpJump,
                    54321);
            var alternativeBeginPos = _instructions.Count;
            ChangeOperand(jumpConsequencePlaceholder, alternativeBeginPos);
            Compile(exp.ElseArm);
            RemoveInnerBlockOpPop();

            var alternativeEndPos = _instructions.Count;
            ChangeOperand(jumpAlternativePlaceholder, alternativeEndPos);
        }

        public Bytecode Bytecode()
        {
            return new Bytecode()
            {
                Instructions = _instructions,
                Constants = ConstantsPool,
            };
        }
    }
}