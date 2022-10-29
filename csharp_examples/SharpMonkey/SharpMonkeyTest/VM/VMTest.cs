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
            if (actual is MonkeyError)
            {
                Assert.Fail($"{actual.Inspect()}");
            }

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
                case bool boolVal:
                    Assert.AreEqual(boolVal, ((MonkeyBoolean) actual).Value);
                    break;
            }
        }

        private void RunVMTests(List<VMTestCase> cases)
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
                new() {input = "-2", expected = -2},
                new() {input = "1 + 2", expected = 3},
                new() {input = "2 * 2", expected = 4},
                new() {input = "4 / 2", expected = 2},
                new() {input = "5 - 2", expected = 3},
                new() {input = "++5;", expected = 6},
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
                new() {input = "-2.0", expected = -2.0},
                new() {input = "1 * 2.0", expected = 2.0},
                new() {input = "1 + 2.0", expected = 3.0},
                new() {input = "4 / 2.0", expected = 2.0},
                new() {input = "5 - 2.5", expected = 2.5},
                new() {input = "++5.0;", expected = 6.0},
            };
            RunVMTests(testTable);
        }

        [Test]
        public void TestBooleanExpressions()
        {
            var testTable = new List<VMTestCase>
            {
                new() {input = "true", expected = true},
                new() {input = "false", expected = false},
                new() {input = "!true", expected = false},
                new() {input = "!!true", expected = true},
                new() {input = "!false", expected = true},
                new() {input = "!!false", expected = false},
                new() {input = "!5", expected = false},
                new() {input = "!0", expected = true},
                new() {input = "!!5", expected = true},
                new() {input = "1 < 2", expected = true},
                new() {input = "1 > 2", expected = false},
                new() {input = "true == true", expected = true},
                new() {input = "true != true", expected = false},
                new() {input = "false != true", expected = true},
                new() {input = "false != false", expected = false},
                new() {input = "(1 < 2 ) != true", expected = false},
                new() {input = "((1 < 2 ) && (2 < 3)) != true", expected = false},
                new() {input = "(1 < 2 ) == true", expected = true},
                new() {input = "(1 > 2 ) || (2 > 1) == true", expected = true},
            };
            RunVMTests(testTable);
        }


        [Test]
        public void TestIfExpression()
        {
            var testTable = new List<VMTestCase>
            {
                new() {input = "if (true) {10;}", expected = 10},
                new() {input = "if (true) {10;} else { 20;}", expected = 10},
                new() {input = "if (false) {10;} else { 20;}", expected = 20},
                new() {input = "if ( 5 < 10) {10;} else { 20;}", expected = 10},
                new() {input = "if ( 5 > 10) {10;} else { 20;}", expected = 20},
                new() {input = "if ( 1 ) {10;} else { 20;}", expected = 10},
                new() {input = "if ( 0 ) {10;} else { 20;}", expected = 20},

                new() {input = "true ? 10 : 20;", expected = 10},
                new() {input = "false ? 10 : 20;", expected = 20},
                new() {input = "if ( true ? false : true ) {10;} else { 20;}", expected = 20},

                // new() {input = "if ( false ) {10;} ", expected = 20}, ///???
            };
            RunVMTests(testTable);
        }
    }
}