using System;
using System.Collections.Generic;

namespace SharpMonkey.VM
{
    using Instructions = List<byte>;
    using Opcode = Byte;

    public class MonkeyVM
    {
        private const int KStackSize = 2048;
        private Instructions _instructions;
        private readonly List<IMonkeyObject> _constantsPool;
        private IMonkeyObject[] _stack; // 以数组尾部为顶形成的栈
        private int _sp = 0;

        public MonkeyVM(Bytecode code)
        {
            _instructions = code.Instructions;
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
            for (int i = 0; i < _instructions.Count; i++)
            {
                var op = (OpConstants) _instructions[i];
                switch (op)
                {
                    case OpConstants.OpConstant:
                        var constIndex = OpcodeUtils.ReadUint16(_instructions, i + 1);
                        i += 2;
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
                        var bPrefix = _instructions[i + 1];
                        i += 1;
                        if (bPrefix == OpcodeUtils.OP_INCREMENT_PREFIX)
                            ExecutePrefixOperation(op);
                        break;
                    case OpConstants.OpJump:
                        var jumpPos = OpcodeUtils.ReadUint16(_instructions, i + 1);
                        i = jumpPos - 1; // - 1是因为break以后会执行 i++
                        break;
                    case OpConstants.OpJumpNotTruthy:
                        jumpPos = OpcodeUtils.ReadUint16(_instructions, i + 1);
                        var condition = Pop();
                        if (!EvaluatorHelper.IsTrueObject(condition))
                        {
                            i = jumpPos - 1;
                        }
                        else
                        {
                            // 无事发生
                            i += 2;
                        }

                        break;
                    case OpConstants.OpNull:
                        Push(MonkeyNull.NullObject);
                        break;
                    default:
                        throw new NotImplementedException($"VM op {op.ToString()} not implemented!");
                }
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
                    if (_stack[_sp - 1] is MonkeyInteger iVal)
                    {
                        iVal.Value += 1;
                    }

                    if (_stack[_sp - 1] is MonkeyDouble dVal)
                    {
                        dVal.Value += 1.0;
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
                // case MonkeyString when right is MonkeyString:
                //     return EvalStringInfixExpression(op, left, right);
                case MonkeyDouble when right is MonkeyInteger:
                case MonkeyInteger when right is MonkeyDouble:
                case MonkeyDouble when right is MonkeyDouble:
                    res = ExecuteDoubleInfixExpression(op, left, right);
                    break;
            }

            Push(res);
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