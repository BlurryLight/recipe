using System;
using System.Collections.Generic;

namespace SharpMonkey.VM
{
    using Instructions = List<Byte>;

    public class Frame
    {
        public MonkeyClosure ClosureFn;

        public int Ip;

        // 由于函数的局部变量会和VM当前的运行上下文共用一个栈
        // 所以在索引函数局部变量时，需要以一个地址作为BaseAddr，偏移计算所有的局部变量
        // 在函数返回时清栈会清理所有函数相关的变量
        public int BasePointer;

        public Frame(MonkeyClosure closureFn, int basePointer)
        {
            ClosureFn = closureFn;
            Ip = -1;
            BasePointer = basePointer;
        }

        public Instructions Instructions => ClosureFn.Fn.Instructions;
    }
}