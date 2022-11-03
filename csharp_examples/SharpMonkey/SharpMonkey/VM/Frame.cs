using System;
using System.Collections.Generic;

namespace SharpMonkey.VM
{
    using Instructions = List<Byte>;

    public class Frame
    {
        public MonkeyCompiledFunction Fn;
        public int Ip;

        public Frame(MonkeyCompiledFunction fn)
        {
            Fn = fn;
            Ip = -1;
        }

        public Instructions Instructions => Fn.Instructions;
    }
}