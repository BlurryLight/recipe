using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

namespace SharpMonkey.VM
{
    using Instructions = List<byte>;
    using Opcode = Byte;

    public class MonkeyVM
    {
        private const int KStackSize = 2048;
        private const int KFrameSize = 1024;
        private const int KGlobalSize = UInt16.MaxValue;
        private readonly List<IMonkeyObject> _constantsPool;
        private IMonkeyObject[] _stack; // 以数组尾部为顶形成的栈
        private int _sp = 0;
        public readonly IMonkeyObject[] Globals;
        private Stack<Frame> Frames;

        public Frame CurrentFrame()
        {
            return Frames.Peek();
        }

        public void PushFrame(Frame frame)
        {
            Frames.Push(frame);
        }

        public Frame PopFrame()
        {
            return Frames.Pop();
        }

        public MonkeyVM(Bytecode code)
        {
            _constantsPool = code.Constants;
            _stack = new IMonkeyObject[KStackSize];
            Globals = new IMonkeyObject[KGlobalSize];
            Frames = new Stack<Frame>(KFrameSize);

            var mainFn = new MonkeyCompiledFunction(code.Instructions);
            var mainClosure = new MonkeyClosure() {Fn = mainFn};
            var mainFrame = new Frame(mainClosure, 0);
            PushFrame(mainFrame);
            _sp = 0;
        }

        public MonkeyVM(Bytecode code, IMonkeyObject[] globals)
        {
            Globals = globals;

            var mainFn = new MonkeyCompiledFunction(code.Instructions);
            var mainClosure = new MonkeyClosure() {Fn = mainFn};
            var mainFrame = new Frame(mainClosure, 0);
            Frames = new Stack<Frame>();
            PushFrame(mainFrame);
            _constantsPool = code.Constants;
            _stack = new IMonkeyObject[KStackSize];
            _sp = 0;
        }

        public IMonkeyObject StackTop()
        {
            return _sp == 0 ? null : _stack[_sp - 1];
        }

        // 对于表达式 "1;2;", 我们会压入一个元素，随后在运行下一句指令之前清栈。
        // 类似于 Push 1; Pop();Push 2; Pop();
        // 在pop的时候我们并没有把元素置为null,这意味着元素仍然在栈上，只有在下一次Push的时候才会被覆盖
        // 所以我们可以通过这个方法拿到仍然在栈上，尚未被覆写的元素。
        public IMonkeyObject LastPoppedStackElem()
        {
            return _stack[_sp];
        }

        private void Push(IMonkeyObject obj)
        {
            if (_sp >= KStackSize)
            {
                throw new IndexOutOfRangeException($"Push: VM sp {_sp} is out of range [0, {KStackSize})");
            }

            _stack[_sp++] = obj;
        }

        private IMonkeyObject Pop()
        {
            if (_sp <= 0)
            {
                throw new IndexOutOfRangeException($"Pop: VM sp {_sp - 1} is out of range [0, {KStackSize})");
            }

            var res = _stack[_sp - 1];
            _sp--;
            return res;
        }

        // 依次执行指令
        public void Run()
        {
            int i;
            OpConstants op;
            Instructions ins;

            // 第一次跑的时候，将起始地址设置为0
            // 调用 PushFrame时，初始化的Frame IP 为 -1
            // 然后再到下一个循环时触发 CurrentFrame().Ip++, 这样新的Frame就会从0开始了
            for (CurrentFrame().Ip = 0; CurrentFrame().Ip < CurrentFrame().Instructions.Count; CurrentFrame().Ip++)
            {
                ins = CurrentFrame().Instructions;
                i = CurrentFrame().Ip;
                op = (OpConstants) ins[i];
                switch (op)
                {
                    case OpConstants.OpConstant:
                        var constIndex = OpcodeUtils.ReadUint16(ins, i + 1);
                        CurrentFrame().Ip += 2;
                        Push(_constantsPool[constIndex]);
                        break;
                    case OpConstants.OpAdd:
                    case OpConstants.OpSub:
                    case OpConstants.OpMul:
                    case OpConstants.OpDiv:
                    case OpConstants.OpEqual:
                    case OpConstants.OpGreaterThan:
                    case OpConstants.OpNotEqual:
                    case OpConstants.OpAnd:
                    case OpConstants.OpOr:
                        ExecuteInfixOperation(op);
                        break;
                    case OpConstants.OpPop:
                        Pop();
                        break;
                    case OpConstants.OpTrue:
                        Push(MonkeyBoolean.TrueObject);
                        break;
                    case OpConstants.OpFalse:
                        Push(MonkeyBoolean.FalseObject);
                        break;
                    case OpConstants.OpBang:
                    case OpConstants.OpMinus:
                        ExecutePrefixOperation(op);
                        break;
                    case OpConstants.OpIncrement:
                    case OpConstants.OpDecrement:
                        var bPrefix = ins[i + 1];
                        CurrentFrame().Ip += 1;
                        if (bPrefix == OpcodeUtils.OP_INCREMENT_PREFIX)
                        {
                            ExecutePrefixOperation(op);
                        }
                        else
                        {
                            ExecutePostfixOperation(op);
                        }

                        break;
                    case OpConstants.OpJump:
                        var jumpPos = OpcodeUtils.ReadUint16(ins, i + 1);
                        CurrentFrame().Ip = jumpPos - 1; // - 1是因为break以后会执行 i++
                        break;
                    case OpConstants.OpJumpNotTruthy:
                        jumpPos = OpcodeUtils.ReadUint16(ins, i + 1);
                        var condition = Pop();
                        if (!EvaluatorHelper.IsTrueObject(condition))
                        {
                            CurrentFrame().Ip = jumpPos - 1;
                        }
                        else
                        {
                            // 无事发生
                            CurrentFrame().Ip += 2;
                        }

                        break;
                    case OpConstants.OpNull:
                        Push(MonkeyNull.NullObject);
                        break;
                    case OpConstants.OpSetGlobal:
                        var setGlobalIdx = OpcodeUtils.ReadUint16(ins, i + 1);
                        Globals[setGlobalIdx] = Pop();
                        CurrentFrame().Ip += 2;
                        break;
                    case OpConstants.OpGetGlobal:
                        var getGlobalIdx = OpcodeUtils.ReadUint16(ins, i + 1);
                        Push(Globals[getGlobalIdx]);
                        CurrentFrame().Ip += 2;
                        break;

                    case OpConstants.OpArray:
                        var numElements = OpcodeUtils.ReadUint16(ins, i + 1);
                        ExecuteBuildArray(numElements);
                        CurrentFrame().Ip += 2;
                        break;
                    case OpConstants.OpHash:
                        numElements = OpcodeUtils.ReadUint16(ins, i + 1);
                        ExecuteBuildMap(numElements);
                        CurrentFrame().Ip += 2;
                        break;
                    case OpConstants.OpIndex:
                        var index = Pop();
                        var left = Pop();
                        Push(ExecuteIndexExpression(left, index));
                        break;
                    case OpConstants.OpCall:
                        var numArgs = (byte) ins[i + 1];
                        CurrentFrame().Ip += 1;
                        ExecuteCall(numArgs);
                        break;
                    case OpConstants.OpSetLocal:
                        byte setLocalIdx = (byte) ins[i + 1];
                        CurrentFrame().Ip += 1;
                        _stack[CurrentFrame().BasePointer + setLocalIdx] = Pop();
                        break;
                    case OpConstants.OpGetLocal:
                        byte getLocalIdx = (byte) ins[i + 1];
                        CurrentFrame().Ip += 1;
                        Push(_stack[CurrentFrame().BasePointer + getLocalIdx]);
                        break;
                    case OpConstants.OpReturnValue:
                        var ret = Pop();
                        var frame = PopFrame();
                        // frame.BasePointer - 1会指向调用 OpCall时候的 fn object
                        // 下一个Push会直接覆盖掉那个fn(),实现清栈的目的
                        _sp = frame.BasePointer - 1;
                        Push(ret); // push the returnd value from the fn()
                        break;
                    case OpConstants.OpReturn:
                        frame = PopFrame();
                        _sp = frame.BasePointer - 1;
                        Push(MonkeyNull.NullObject);
                        break;
                    case OpConstants.OpGetBuiltin:
                        var builtinIndex = (byte) ins[i + 1];
                        CurrentFrame().Ip += 1;
                        var builtinFunc = MonkeyBuiltinFunc.BuiltinArrays[builtinIndex].Value;
                        Push(builtinFunc);
                        break;
                    case OpConstants.OpClosure:
                        var fnIndex = OpcodeUtils.ReadUint16(ins, i + 1);
                        var freeVarNum = (byte) ins[i + 3];
                        CurrentFrame().Ip += 3;
                        PushClosure(fnIndex, freeVarNum);
                        break;
                    case OpConstants.OpGetFree:
                        var freeIndex = (byte) ins[i + 1];
                        CurrentFrame().Ip += 1;
                        Push(CurrentFrame().ClosureFn.FreeVariables[freeIndex]);
                        break;
                    case OpConstants.OpCurrentClosure:
                        var currentClosure = CurrentFrame().ClosureFn;
                        Push(currentClosure);
                        break;
                    default:
                        throw new NotImplementedException($"VM op {op.ToString()} not implemented!");
                }
            }
        }

        private void PushClosure(ushort fnIndex, byte freeVarNum)
        {
            var fn = (MonkeyCompiledFunction) _constantsPool[fnIndex];
            // TODO: fill free variables
            var freeVars = new IMonkeyObject[freeVarNum];
            for (int i = 0; i < freeVarNum; i++)
            {
                freeVars[i] = _stack[_sp - freeVarNum + i];
            }

            var closure = new MonkeyClosure() {Fn = fn, FreeVariables = freeVars.ToList()};
            Push(closure);
        }

        private void ExecuteCall(byte numArgs)
        {
            var functor = _stack[_sp - 1 - numArgs];
            switch (functor)
            {
                case MonkeyClosure closureFunc:
                    ExecuteFuncCall(closureFunc, numArgs);
                    break;
                case MonkeyBuiltinFunc builtinFunc:
                    ExecuteBuiltinFuncCall(builtinFunc, numArgs);
                    break;
            }
        }

        private void ExecuteBuiltinFuncCall(MonkeyBuiltinFunc builtinFunc, byte numArgs)
        {
            // 通过slice取出参数
            var args = _stack[(_sp - numArgs).._sp];
            var result = builtinFunc.Func(args);
            // 回退栈顶到BuiltInFuncObject所在的位置,准备用返回值覆盖它
            _sp -= (numArgs + 1);
            // 部分内置函数，比如puts,没有返回值，会返回Null
            // 另一种做法可以修改puts的返回值为 MonkeyNull
            Push(result ?? MonkeyNull.NullObject);
        }

        private void ExecuteFuncCall(MonkeyClosure closure, byte numArgs)
        {
            var fn = closure.Fn;
            if (numArgs != fn.NumParameters)
            {
                throw new ArgumentException(
                    $"Wrong Number of arguments: {fn.Inspect()}, wanted {fn.NumParameters}, got: {numArgs} ");
            }

            // call func的时候，为函数体内所有的局部变量分配空间
            PushFrame(new Frame(closure, _sp - numArgs));
            _sp += fn.NumLocals;
        }

        private IMonkeyObject ExecuteIndexExpression(IMonkeyObject left, IMonkeyObject index)
        {
            // 这个实现和Evaluator是一样的，直接复用吧
            return Evaluator.EvalIndexExpressionImmutable(left, index);
        }

        private void ExecuteBuildMap(ushort numElements)
        {
            Debug.Assert(numElements % 2 == 0);
            var pairs = new Dictionary<HashKey, MonkeyMap.HashPairs>();
            _sp -= numElements;
            for (int i = 0; i < numElements; i += 2)
            {
                var key = _stack[_sp + i];
                var value = _stack[_sp + i + 1];
                var hashKey = ((IMonkeyHash) key).HashKey();
                pairs.Add(hashKey, new MonkeyMap.HashPairs() {KeyObj = key, ValueObj = value});
            }

            var monkeyMap = new MonkeyMap(pairs);
            Push(monkeyMap);
        }

        private void ExecuteBuildArray(ushort numElements)
        {
            var lst = new List<IMonkeyObject>(numElements);
            // _sp一开始指向 List最后一个元素之后的一个元素
            // [1,2,3] _sp = 3
            // 减去num以后指向0，恰好是数组的开始
            _sp -= numElements;
            for (int i = 0; i < numElements; i++)
            {
                lst.Add(_stack[_sp + i]);
            }

            var monkeyArray = new MonkeyArray(lst);
            Push(monkeyArray);
        }

        private void ExecutePostfixOperation(OpConstants op)
        {
            var val = Pop();
            switch (val)
            {
                case MonkeyInteger intVal:
                {
                    var oldVal = new MonkeyInteger(intVal.Value);
                    intVal.Value += (op == OpConstants.OpDecrement) ? -1 : 1;
                    Push(oldVal);
                    break;
                }
                case MonkeyDouble doubleVal:
                {
                    var oldVal = new MonkeyDouble(doubleVal.Value);
                    doubleVal.Value += (op == OpConstants.OpDecrement) ? -1 : 1;
                    Push(oldVal);
                    break;
                }
                default:
                    throw new NotImplementedException($"Post op {val.Type()}{op.ToString()} not implemented!");
            }
        }

        private void ExecutePrefixOperation(OpConstants op)
        {
            switch (op)
            {
                case OpConstants.OpBang:
                    ExecutePrefixBangOperation();
                    break;
                case OpConstants.OpMinus:
                    ExecutePrefixMinusOperation();
                    break;
                case OpConstants.OpIncrement:
                case OpConstants.OpDecrement:
                    if (_stack[_sp - 1] is MonkeyInteger iVal)
                    {
                        iVal.Value += op == OpConstants.OpDecrement ? -1 : 1;
                    }

                    if (_stack[_sp - 1] is MonkeyDouble dVal)
                    {
                        dVal.Value += op == OpConstants.OpDecrement ? -1 : 1;
                    }

                    break;
            }
        }

        private void ExecutePrefixMinusOperation()
        {
            var operand = Pop();
            switch (operand)
            {
                case MonkeyDouble dVal:
                    Push(new MonkeyDouble(-dVal.Value));
                    break;
                case MonkeyInteger iVal:
                    Push(new MonkeyInteger(-iVal.Value));
                    break;
                default:
                    throw new ArgumentException($"!{operand.Type()} is not supported");
            }
        }

        private void ExecutePrefixBangOperation()
        {
            var operand = Pop();
            Push(MonkeyBoolean.GetStaticObject(!EvaluatorHelper.IsTrueObject(operand)));
        }

        private void ExecuteInfixOperation(OpConstants op)
        {
            var right = Pop();
            var left = Pop();

            IMonkeyObject res = null;
            // 单独处理隐式转换的问题
            if (op == OpConstants.OpAnd || op == OpConstants.OpOr)
            {
                res = ExecuteBoolInfixExpression(op, left, right);
                Push(res);
                return;
            }

            switch (left)
            {
                case MonkeyInteger when right is MonkeyInteger:
                    res = ExecuteIntegerInfixOperation(op, left, right);
                    break;
                case MonkeyBoolean when right is MonkeyBoolean:
                    res = ExecuteBoolInfixExpression(op, left, right);
                    break;
                case MonkeyString when right is MonkeyString:
                    res = ExecuteStringInfixExpression(op, left, right);
                    break;
                case MonkeyDouble when right is MonkeyInteger:
                case MonkeyInteger when right is MonkeyDouble:
                case MonkeyDouble when right is MonkeyDouble:
                    res = ExecuteDoubleInfixExpression(op, left, right);
                    break;
                default:
                    res = new MonkeyError($"unsupported infix: {left.Type()} {op} {right.Type()}");
                    break;
            }

            Push(res);
        }

        private IMonkeyObject ExecuteStringInfixExpression(OpConstants op, IMonkeyObject left, IMonkeyObject right)
        {
            var leftStr = (MonkeyString) left;
            var rightStr = (MonkeyString) right;
            return op switch
            {
                OpConstants.OpAdd => new MonkeyString(leftStr.Value + rightStr.Value),
                _ => new MonkeyError($"unsupported String infix: {left.Type()} {op} {right.Type()}")
            };
        }

        private IMonkeyObject ExecuteBoolInfixExpression(OpConstants op, IMonkeyObject left, IMonkeyObject right)
        {
            var leftBoolean = MonkeyBoolean.ImplicitConvertFrom(left);
            var rightBoolean = MonkeyBoolean.ImplicitConvertFrom(right);
            switch (op)
            {
                case OpConstants.OpEqual:
                    return MonkeyBoolean.GetStaticObject(leftBoolean == rightBoolean);
                case OpConstants.OpNotEqual:
                    return MonkeyBoolean.GetStaticObject(leftBoolean != rightBoolean);
                case OpConstants.OpAnd:
                    return MonkeyBoolean.GetStaticObject(leftBoolean.Value && rightBoolean.Value);
                case OpConstants.OpOr:
                    return MonkeyBoolean.GetStaticObject(leftBoolean.Value || rightBoolean.Value);
                default:
                    return new MonkeyError($"unsupported infix: {left.Type()} {op} {right.Type()}");
            }
        }

        private IMonkeyObject ExecuteDoubleInfixExpression(OpConstants op, IMonkeyObject left, IMonkeyObject right)
        {
            var leftDouble = left is MonkeyInteger leftInt ? leftInt.ToMonkeyDouble() : (MonkeyDouble) left;
            var rightDouble = right is MonkeyInteger rightInt ? rightInt.ToMonkeyDouble() : (MonkeyDouble) right;
            return op switch
            {
                OpConstants.OpAdd => new MonkeyDouble(leftDouble.Value + rightDouble.Value),
                OpConstants.OpSub => new MonkeyDouble(leftDouble.Value - rightDouble.Value),
                OpConstants.OpMul => new MonkeyDouble(leftDouble.Value * rightDouble.Value),
                OpConstants.OpDiv => new MonkeyDouble(leftDouble.Value / rightDouble.Value),
                OpConstants.OpGreaterThan => MonkeyBoolean.GetStaticObject(leftDouble.Value > rightDouble.Value),
                OpConstants.OpEqual => MonkeyBoolean.GetStaticObject(leftDouble.Value == rightDouble.Value),
                OpConstants.OpNotEqual => MonkeyBoolean.GetStaticObject(leftDouble.Value != rightDouble.Value),
                _ => new MonkeyError($"unsupported Double infix: {left.Type()} {op} {right.Type()}")
            };
        }

        private IMonkeyObject ExecuteIntegerInfixOperation(OpConstants op, IMonkeyObject left, IMonkeyObject right)
        {
            var leftInt = (MonkeyInteger) left;
            var rightInt = (MonkeyInteger) right;
            return op switch
            {
                OpConstants.OpAdd => new MonkeyInteger(leftInt.Value + rightInt.Value),
                OpConstants.OpSub => new MonkeyInteger(leftInt.Value - rightInt.Value),
                OpConstants.OpMul => new MonkeyInteger(leftInt.Value * rightInt.Value),
                OpConstants.OpDiv => new MonkeyInteger(leftInt.Value / rightInt.Value),
                OpConstants.OpGreaterThan => MonkeyBoolean.GetStaticObject(leftInt.Value > rightInt.Value),
                OpConstants.OpEqual => MonkeyBoolean.GetStaticObject(leftInt.Value == rightInt.Value),
                OpConstants.OpNotEqual => MonkeyBoolean.GetStaticObject(leftInt.Value != rightInt.Value),
                _ => new MonkeyError($"unsupported Integer infix: {left.Type()} {op} {right.Type()}")
            };
        }
    }
}