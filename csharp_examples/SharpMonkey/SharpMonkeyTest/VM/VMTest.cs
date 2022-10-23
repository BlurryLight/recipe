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
                case double doubleVal:
                    CompilerTest.TestDoubleObject(doubleVal, actual);
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
                var stackTop = vm.LastPoppedStackElem();
                TestExpectedObject(testCase.expected, stackTop);
                // 确认已经清栈
                Assert.IsNull(vm.StackTop());
            }
        }


        [Test]
        public void TestIntegerArithmetic()
        {
            var testTable = new List<VMTestCase>
            {
                new() {input = "1", expected = 1},
                new() {input = "2", expected = 2},
                new() {input = "1 + 2", expected = 3},
                new() {input = "2 * 2", expected = 4},
                new() {input = "4 / 2", expected = 2},
                new() {input = "5 - 2", expected = 3},
            };
            RunVMTests(testTable);
        }

        [Test]
        public void TestDoubleArithmetic()
        {
            var testTable = new List<VMTestCase>
            {
                new() {input = "1.0", expected = 1.0},
                new() {input = "2.0", expected = 2.0},
                new() {input = "1 * 2.0", expected = 2.0},
                new() {input = "1 + 2.0", expected = 3.0},
                new() {input = "4 / 2.0", expected = 2.0},
                new() {input = "5 - 2.5", expected = 2.5},
            };
            RunVMTests(testTable);
        }
    }
}