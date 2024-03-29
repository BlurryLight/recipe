﻿using System;
using System.Collections.Generic;
using System.Diagnostics;

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

    public class CompilationScope
    {
        public Instructions Instructions;
        public EmittedInstruction LastInstruction;
        public EmittedInstruction PreviousInstruction;

        public CompilationScope(Instructions instructions, EmittedInstruction lastInstruction,
            EmittedInstruction previousInstruction)
        {
            Instructions = instructions;
            LastInstruction = lastInstruction;
            PreviousInstruction = previousInstruction;
        }
    }

    public class Compiler
    {
        private List<CompilationScope> _scopes;
        private int _scopeIndex = 0;
        public readonly List<IMonkeyObject> ConstantsPool;
        public readonly Dictionary<HashKey, int> ConstantsPoolIndex;
        private static SymbolTable _BuiltinSymbolTable = null;
        public SymbolTable CurSymbolTable;

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
        private int AddInstruction(IEnumerable<byte> ins)
        {
            var pos = CurrentInstructions().Count;
            CurrentInstructions().AddRange(ins);
            return pos;
        }

        private void EnterNewScope()
        {
            var scope = new CompilationScope(
                new Instructions(),
                new EmittedInstruction(),
                new EmittedInstruction());
            _scopes.Add(scope);
            _scopeIndex++;

            CurSymbolTable = new SymbolTable(CurSymbolTable, SymbolScope.Local);
        }

        private Instructions LeaveScope()
        {
            var ins = CurrentScope().Instructions;
            _scopes.RemoveAt(_scopes.Count - 1);
            _scopeIndex--;
            CurSymbolTable = CurSymbolTable.Outer;
            return ins;
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
            var scope = _scopes[_scopeIndex];
            var lastInstruction = new EmittedInstruction() {Op = op, Pos = pos};
            scope.PreviousInstruction = scope.LastInstruction;
            scope.LastInstruction = lastInstruction;
        }

        private bool LastInstructionIs(OpConstants op)
        {
            return CurrentScope().LastInstruction.Op == (byte) op;
        }

        private void RemoveLastOp()
        {
            var prev = CurrentScope().PreviousInstruction;
            var ins = CurrentScope().Instructions;

            ins.RemoveAt(ins.Count - 1);
            CurrentScope().LastInstruction = prev;
        }

        private void RemoveLastPop()
        {
            if (LastInstructionIs(OpConstants.OpPop))
            {
                RemoveLastOp();
            }
        }

        // 把从StartPos开始的字节码，替换为ins内的字节码
        private void ReplaceInstructionsRange(int startPos, Instructions ins)
        {
            var destIns = CurrentInstructions();
            // C# has no support for Buffer.Copy for List<byte>
            for (int i = 0; i < ins.Count; i++)
            {
                destIns[startPos + i] = ins[i];
            }
        }

        private void ChangeOperand(int opPos, params int[] operands)
        {
            var op = CurrentInstructions()[opPos];
            var newIns = OpcodeUtils.MakeBytes(op, operands);
            ReplaceInstructionsRange(opPos, newIns);
        }

        private CompilationScope CurrentScope()
        {
            return _scopes[_scopeIndex];
        }

        private Instructions CurrentInstructions()
        {
            return _scopes[_scopeIndex].Instructions;
        }

        private void InitProperties()
        {
            var instructions = new Instructions();
            var lastInstruction = new EmittedInstruction();
            var previousInstruction = new EmittedInstruction();
            var mainScope = new CompilationScope(instructions, lastInstruction, previousInstruction);
            _scopes = new List<CompilationScope>() {mainScope};
        }

        public Compiler()
        {
            InitProperties();
            if (_BuiltinSymbolTable == null)
            {
                _BuiltinSymbolTable = new SymbolTable(SymbolScope.Builtin);
                for (int i = 0; i < MonkeyBuiltinFunc.BuiltinArrays.Count; i++)
                {
                    _BuiltinSymbolTable.DefineBuiltin(i, MonkeyBuiltinFunc.BuiltinArrays[i].Key);
                }
            }

            // 全局SymbolTable继承自BuiltinTable
            CurSymbolTable = new SymbolTable(_BuiltinSymbolTable, SymbolScope.Global);
            ConstantsPool = new List<IMonkeyObject>();
            ConstantsPoolIndex = new Dictionary<HashKey, int>();
        }

        public Compiler(Compiler other)
        {
            InitProperties();
            ConstantsPool = other.ConstantsPool;
            ConstantsPoolIndex = other.ConstantsPoolIndex;
            CurSymbolTable = other.CurSymbolTable;
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
                        case Constants.Decrement:
                            Emit((byte) OpConstants.OpDecrement, OpcodeUtils.OP_INCREMENT_PREFIX);
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
                case Ast.WhileExpression exp:
                    CompileWhileExpression(exp);
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
                    var symbol = CurSymbolTable.Define(stmt.Name.Value);
                    Compile(stmt.Value);
                    Emit(
                        symbol.Scope == SymbolScope.Global
                            ? (byte) OpConstants.OpSetGlobal
                            : (byte) OpConstants.OpSetLocal,
                        symbol.Index);
                    break;
                case Ast.Identifier ident:
                    symbol = CurSymbolTable.Resolve(ident.Value);
                    EmitSymbol(symbol);
                    break;
                case Ast.AssignExpression exp:
                    CompileAssignExpression(exp);
                    break;
                case Ast.PostfixExpression exp:
                    Compile(exp.Left);
                    switch (exp.Operator)
                    {
                        case Constants.Increment:
                            Emit((byte) OpConstants.OpIncrement, OpcodeUtils.OP_INCREMENT_POSTFIX);
                            break;
                        case Constants.Decrement:
                            Emit((byte) OpConstants.OpDecrement, OpcodeUtils.OP_INCREMENT_POSTFIX);
                            break;
                        default:
                            throw new NotImplementedException($"not implemented for PostOperator{exp.Operator}");
                    }

                    break;
                case Ast.ArrayLiteral exp:
                    foreach (var elem in exp.Elements)
                    {
                        Compile(elem);
                    }

                    Emit((byte) OpConstants.OpArray, exp.Elements.Count);
                    break;
                case Ast.MapLiteral exp:
                    foreach (var pair in exp.Pairs)
                    {
                        Compile(pair.Key);
                        Compile(pair.Value);
                    }

                    Emit((byte) OpConstants.OpHash, exp.Pairs.Count * 2);
                    break;
                case Ast.IndexExpression exp:
                    Compile(exp.Left);
                    Compile(exp.Index);
                    Emit((byte) OpConstants.OpIndex);
                    break;
                case Ast.FunctionLiteral exp:
                    EnterNewScope();
                    if (exp.FuncName != "UnNamed")
                    {
                        CurSymbolTable.DefineFunctionName(exp.FuncName);
                    }

                    foreach (var par in exp.Parameters)
                    {
                        // 从左到右定义所有的形参变量名
                        CurSymbolTable.Define(par.Value);
                    }

                    Compile(exp.FuncBody);
                    if (LastInstructionIs(OpConstants.OpPop))
                    {
                        // 隐式返回，需要把最后一个Pop换成Return
                        CurrentInstructions()[^1] = (byte) OpConstants.OpReturnValue;
                        CurrentScope().LastInstruction.Op = (byte) OpConstants.OpReturnValue;
                    }

                    // 如果函数没有返回值，插入一个 Null返回指令
                    if (!LastInstructionIs(OpConstants.OpReturnValue))
                    {
                        Emit((byte) OpConstants.OpReturn);
                    }

                    var freeSymbols = CurSymbolTable.FreeSymbolsToPush;
                    var freeVariableNum = freeSymbols.Count;
                    var numLocals = CurSymbolTable.NumDefinitions;
                    // LeaveScope会把符号表移动为上一层的符号表，所以在LeaveGroup之前要保存当前符号表的局部变量数
                    var compiledFunction = new MonkeyCompiledFunction(LeaveScope())
                    {
                        NumLocals = numLocals,
                        NumParameters = exp.Parameters.Count
                    };
#if DEBUG
                    compiledFunction.Source = exp.ToPrintableString();
#endif
                    // 在闭包上层压入所有自由变量的值
                    foreach (var fs in freeSymbols)
                    {
                        EmitSymbol(fs);
                    }

                    // Emit((byte) OpConstants.OpConstant, AddConstant(compiledFunction));
                    // 为了支持闭包，我们把所有函数都视作闭包，把普通函数视作闭包的一种无捕获的特殊情形
                    // 指令替换为OpClosure
                    Emit((byte) OpConstants.OpClosure, AddConstant(compiledFunction), freeVariableNum);
                    break;
                case Ast.ReturnStatement stmt:
                    Compile(stmt.ReturnValue);
                    Emit((byte) OpConstants.OpReturnValue);
                    break;
                case Ast.CallExpression exp:
                    Compile(exp.Function);
                    if (exp.Arguments.Count is not (>= 0 and <= byte.MaxValue))
                    {
                        throw new ArgumentException(
                            $"Arguments Number should be in [0,255],now is {exp.Arguments.Count}");
                    }

                    // 依次把参数压栈
                    foreach (var arg in exp.Arguments)
                    {
                        Compile(arg);
                    }

                    Emit((byte) OpConstants.OpCall, exp.Arguments.Count);
                    break;
                default:
                    throw new NotImplementedException(
                        $"not implemented for type {node.GetType()}:{node.ToPrintableString()}");
            }
        }

        private void CompileAssignExpression(Ast.AssignExpression exp)
        {
            if (exp.Name is Ast.Identifier ident)
            {
                var symbol = CurSymbolTable.Resolve(ident.Value);
                Compile(exp.Value);
                Emit(
                    symbol.Scope == SymbolScope.Global
                        ? (byte) OpConstants.OpAssignGlobal
                        : (byte) OpConstants.OpAssignLocal,
                    symbol.Index);
                return;
            }

            if (exp.Name is Ast.IndexExpression idxExpression)
            {
                Compile(idxExpression.Left);
                Compile(idxExpression.Index);
                Compile(exp.Value);
                Emit((byte) OpConstants.OpIndexSet);
                return;
            }

            throw new NotImplementedException();
        }

        private void EmitSymbol(Symbol symbol)
        {
            switch (symbol.Scope)
            {
                case SymbolScope.Global:
                    Emit((byte) OpConstants.OpGetGlobal, symbol.Index);
                    break;
                case SymbolScope.Local:
                    Emit((byte) OpConstants.OpGetLocal, symbol.Index);
                    break;
                case SymbolScope.Builtin:
                    Emit((byte) OpConstants.OpGetBuiltin, symbol.Index);
                    break;
                case SymbolScope.Free:
                    Emit((byte) OpConstants.OpGetFree, symbol.Index);
                    break;
                case SymbolScope.Function:
                    Emit((byte) OpConstants.OpCurrentClosure);
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }


        private void CompileWhileExpression(Ast.WhileExpression exp)
        {
            // CurSymbolTable = new SymbolTable(CurSymbolTable, SymbolScope.Local);
            // 见单元测试
            var beginPos = CurrentInstructions().Count;
            Compile(exp.Condition);
            var jumpConsequencePlaceholder =
                Emit((byte) OpConstants.OpJumpNotTruthy,
                    54321);
            Compile(exp.Body);
            Emit((byte) OpConstants.OpJump,
                beginPos);
            var jumpOutPos = CurrentInstructions().Count;
            ChangeOperand(jumpConsequencePlaceholder, jumpOutPos);
            Emit((byte) OpConstants.OpNull);
            // CurSymbolTable = CurSymbolTable.Outer;
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
            RemoveLastPop();
            var jumpAlternativePlaceholder =
                Emit((byte) OpConstants.OpJump,
                    54321);
            var alternativeBeginPos = CurrentInstructions().Count;
            ChangeOperand(jumpConsequencePlaceholder, alternativeBeginPos);
            if (exp.Alternative == null)
            {
                Emit((byte) OpConstants.OpNull);
            }
            else
            {
                Compile(exp.Alternative);
                RemoveLastPop();
            }

            var alternativeEndPos = CurrentInstructions().Count;
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
            RemoveLastPop();
            var jumpAlternativePlaceholder =
                Emit((byte) OpConstants.OpJump,
                    54321);
            var alternativeBeginPos = CurrentInstructions().Count;
            ChangeOperand(jumpConsequencePlaceholder, alternativeBeginPos);
            Compile(exp.ElseArm);
            RemoveLastPop();

            var alternativeEndPos = CurrentInstructions().Count;
            ChangeOperand(jumpAlternativePlaceholder, alternativeEndPos);
        }

        public Bytecode Bytecode()
        {
            return new Bytecode()
            {
                Instructions = CurrentInstructions(),
                Constants = ConstantsPool,
            };
        }
    }
}