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
                        var right = (MonkeyInteger) Pop();
                        var left = (MonkeyInteger) Pop();
                        Push(new MonkeyInteger(left.Value + right.Value));
                        break;
                }
            }
        }
    }
}