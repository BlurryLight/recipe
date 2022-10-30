﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
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

        private struct VMTestCase
        {
            public string Input;
            public Object Expected;
        };

        private void TestExpectedObject(Object expected, IMonkeyObject actual)
        {
            if (actual is MonkeyError)
            {
                if (expected is MonkeyError)
                {
                    Assert.AreEqual(expected.ToString(), actual.ToString());
                    return;
                }

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
                    Assert.AreEqual(boolVal, MonkeyBoolean.ImplicitConvertFrom(actual).Value);
                    break;
                case string stringVal:
                    CompilerTest.TestStringObject(stringVal, actual);
                    break;
                case MonkeyNull:
                    Assert.IsTrue(EvaluatorHelper.IsNullObject(actual));
                    break;
                case List<object> lst:
                    var monkeyArray = (MonkeyArray) actual;
                    Assert.AreEqual(lst.Count, monkeyArray.Elements.Count);
                    for (int i = 0; i < lst.Count; i++)
                    {
                        TestExpectedObject(lst[i], monkeyArray.Elements[i]);
                    }

                    break;
                case Dictionary<object, object> dict:
                    var monkeyMap = (MonkeyMap) actual;
                    Assert.AreEqual(dict.Count, monkeyMap.Pairs.Count);
                    foreach (var pair in monkeyMap.Pairs)
                    {
                        Assert.IsTrue(dict.ContainsKey(pair.Key));
                        //  假设测试样例中，value项都是integer
                        long valueVal = ((MonkeyInteger) dict[pair.Key]).Value;
                        TestExpectedObject(valueVal, pair.Value.ValueObj);
                    }

                    break;
                default:
                    Assert.Fail("Should not be here");
                    break;
            }
        }

        private void RunVMTests(List<VMTestCase> cases)
        {
            foreach (var testCase in cases)
            {
                var p = CompilerTest.Parse(testCase.Input);
                var comp = new Compiler();
                comp.Compile(p);

                var vm = new MonkeyVM(comp.Bytecode());
                vm.Run();
                var stackTop = vm.LastPoppedStackElem();
                TestExpectedObject(testCase.Expected, stackTop);
                // 确认已经清栈
                Assert.IsNull(vm.StackTop());
            }
        }


        [Test]
        public void TestIntegerArithmetic()
        {
            var testTable = new List<VMTestCase>
            {
                new() {Input = "1", Expected = 1},
                new() {Input = "2", Expected = 2},
                new() {Input = "-2", Expected = -2},
                new() {Input = "1 + 2", Expected = 3},
                new() {Input = "2 * 2", Expected = 4},
                new() {Input = "4 / 2", Expected = 2},
                new() {Input = "5 - 2", Expected = 3},
                new() {Input = "++5;", Expected = 6},
                new() {Input = "--5;", Expected = 4},
                new() {Input = "5++;", Expected = 5},
                new() {Input = "(5++)++;", Expected = 5},
                new() {Input = "++(5++);", Expected = 6},
                new() {Input = "5--;", Expected = 5},
            };
            RunVMTests(testTable);
        }

        [Test]
        public void TestDoubleArithmetic()
        {
            var testTable = new List<VMTestCase>
            {
                new() {Input = "1.0", Expected = 1.0},
                new() {Input = "2.0", Expected = 2.0},
                new() {Input = "-2.0", Expected = -2.0},
                new() {Input = "1 * 2.0", Expected = 2.0},
                new() {Input = "1 + 2.0", Expected = 3.0},
                new() {Input = "4 / 2.0", Expected = 2.0},
                new() {Input = "5 - 2.5", Expected = 2.5},
                new() {Input = "++5.0;", Expected = 6.0},
                new() {Input = "5.0++;", Expected = 5.0},
                new() {Input = "--5.0;", Expected = 4.0},
                new() {Input = "5.0--;", Expected = 5.0},
            };
            RunVMTests(testTable);
        }

        [Test]
        public void TestBooleanExpressions()
        {
            var testTable = new List<VMTestCase>
            {
                new() {Input = "true", Expected = true},
                new() {Input = "false", Expected = false},
                new() {Input = "!true", Expected = false},
                new() {Input = "!!true", Expected = true},
                new() {Input = "!false", Expected = true},
                new() {Input = "!!false", Expected = false},
                new() {Input = "!5", Expected = false},
                new() {Input = "!0", Expected = true},
                new() {Input = "!!5", Expected = true},
                new() {Input = "1 < 2", Expected = true},
                new() {Input = "1 > 2", Expected = false},
                new() {Input = "true == true", Expected = true},
                new() {Input = "true != true", Expected = false},
                new() {Input = "false != true", Expected = true},
                new() {Input = "false != false", Expected = false},
                new() {Input = "(1 < 2 ) != true", Expected = false},
                new() {Input = "((1 < 2 ) && (2 < 3)) != true", Expected = false},
                new() {Input = "(1 < 2 ) == true", Expected = true},
                new() {Input = "(1 > 2 ) || (2 > 1) == true", Expected = true},

                new() {Input = "if(false){10;}", Expected = false},
                new() {Input = "!(if(false){10;})", Expected = true},
                new() {Input = "null", Expected = false},
                new() {Input = "!null", Expected = true},

                new() {Input = "false && (1 / 0) ", Expected = false}, // will not throw exception
                new() {Input = "(false && false)  && (1 / 0) ", Expected = false},
                new() {Input = "(false || true )  || 1  ", Expected = true},
                new() {Input = "true && (1 / 1) ", Expected = true},
                new() {Input = "false && (true) ", Expected = false},
            };
            RunVMTests(testTable);
        }


        [Test]
        public void TestIfExpression()
        {
            var testTable = new List<VMTestCase>
            {
                new() {Input = "if (true) {10;}", Expected = 10},
                new() {Input = "if (true) {10;} else { 20;}", Expected = 10},
                new() {Input = "if (false) {10;} else { 20;}", Expected = 20},
                new() {Input = "if ( 5 < 10) {10;} else { 20;}", Expected = 10},
                new() {Input = "if ( 5 > 10) {10;} else { 20;}", Expected = 20},
                new() {Input = "if ( 1 ) {10;} else { 20;}", Expected = 10},
                new() {Input = "if ( 0 ) {10;} else { 20;}", Expected = 20},

                new() {Input = "true ? 10 : 20;", Expected = 10},
                new() {Input = "false ? 10 : 20;", Expected = 20},
                new() {Input = "if ( true ? false : true ) {10;} else { 20;}", Expected = 20},

                new() {Input = "if ( false ) {10;} ", Expected = MonkeyNull.NullObject},
            };
            RunVMTests(testTable);
        }

        [Test]
        public void TestGlobalLetStatements()
        {
            var testTable = new List<VMTestCase>
            {
                new() {Input = "let a = 1; a", Expected = 1},
                new() {Input = "let a = 1; let b = a + 1; b", Expected = 2},
                new() {Input = "let a = 1; let b = 3; a + b;", Expected = 4},
                new() {Input = "let a = 1; a++;", Expected = 1},
                new() {Input = "let a = 1; a++;a;", Expected = 2},
                new() {Input = "let a = 1; ++a;", Expected = 2},
                new() {Input = "let a = 1; ++a;a", Expected = 2},
                new() {Input = "let a = 1; --(++a);", Expected = 1},
                new() {Input = "let a = 1; --(++a);a", Expected = 1},
            };
            RunVMTests(testTable);
        }

        [Test]
        public void TestStringExpressions()
        {
            var testTable = new List<VMTestCase>
            {
                new() {Input = "\"Hello\"", Expected = "Hello"},
                new() {Input = "\"Hello\" + \" Monkey\"", Expected = "Hello Monkey"},
            };
            RunVMTests(testTable);
        }

        [Test]
        public void TestArrayLiterals()
        {
            var testTable = new List<VMTestCase>
            {
                new() {Input = "[1,2]", Expected = new List<object> {1, 2}},
                new() {Input = "[]", Expected = new List<object> { }},
                new() {Input = "[1 + 2,\"hello\",3.0]", Expected = new List<object> {3, "hello", 3.0}},
            };
            RunVMTests(testTable);
        }

        [Test]
        public void TestHashLiterals()
        {
            var testTable = new List<VMTestCase>
            {
                new() {Input = "{}", Expected = new Dictionary<object, object>() { }},
                new()
                {
                    Input = "{1:2}",
                    Expected = new Dictionary<object, object>() {{new MonkeyInteger(1).HashKey(), new MonkeyInteger(2)}}
                },
                new()
                {
                    Input = "{\"hello\":2}",
                    Expected = new Dictionary<object, object>()
                        {{new MonkeyString("hello").HashKey(), new MonkeyInteger(2)}}
                },
            };
            RunVMTests(testTable);
        }

        [Test]
        public void TestIndexExpressions()
        {
            var testTable = new List<VMTestCase>
            {
                new() {Input = "{1:2,2:3}[1]", Expected = 2},
                new() {Input = "{1:2,2:3}[2]", Expected = 3},
                new() {Input = "[1,2,3][0]", Expected = 1},

                // error
                new() {Input = "[][0]", Expected = new MonkeyError("Array[0] index out of range")},
                new() {Input = "[][-1]", Expected = new MonkeyError("Array[-1] index out of range")},
                new() {Input = "{}[0]", Expected = new MonkeyError("Map[0] doesn't exist")},
                new() {Input = "{}[\"a\"]", Expected = new MonkeyError("Map[\"a\"] doesn't exist")},
            };
            RunVMTests(testTable);
        }
    }
}