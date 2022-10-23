using System;
using System.Collections.Generic;
using NUnit.Framework;
using SharpMonkey;
using SharpMonkey.VM;

namespace SharpMonkeyTest
{
    using Instructions = List<byte>;
    using Opcode = Byte;

    public class VMTest
    {
        [SetUp]
        public void Setup()
        {
        }

        public struct VMTestCase
        {
            public string input;
            public Object expected;
        };

        public void TestExpectedObject(Object expected, IMonkeyObject actual)
        {
            switch (expected)
            {
                case int:
                case long:
                    var val = Convert.ToInt64(expected);
                    CompilerTest.TestIntegerObject(val, actual);
                    break;
            }
        }

        public void RunVMTests(List<VMTestCase> cases)
        {
            foreach (var testCase in cases)
            {
                var p = CompilerTest.Parse(testCase.input);
                var comp = new Compiler();
                comp.Compile(p);

                var vm = new MonkeyVM(comp.Bytecode());
                vm.Run();
                var stackTop = vm.StackTop();
                TestExpectedObject(testCase.expected, stackTop);
            }
        }


        [Test]
        public void TestIntegerArithmetic()
        {
            var testTable = new List<VMTestCase>
            {
                // new() {input = "1", expected = 1},
                // new() {input = "2", expected = 2},
                new() {input = "1 + 2", expected = 2}, // TODO: Fix this
            };
            RunVMTests(testTable);
        }
    }
}