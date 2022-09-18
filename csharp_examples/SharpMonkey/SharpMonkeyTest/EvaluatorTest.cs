﻿using System;
using System.Collections.Generic;
using NUnit.Framework;
using SharpMonkey;

namespace SharpMonkeyTest
{
    public class EvaluatorTest
    {
        [SetUp]
        public void Setup()
        {
        }

        private static MonkeyObject TestEval(string input)
        {
            var p = new Parser(new Lexer(input));
            var program = p.ParseProgram();
            ParserTest.CheckParserErrors(p);
            // Assert.AreEqual(0 ,p.Errors.Count);
            var env = new SharpMonkey.Environment();
            return Evaluator.Eval(program, env);
        }

        private static void TestIntegerObject(MonkeyObject obj, long expectedVal, string input)
        {
            var intObj = obj as MonkeyInteger;
            Assert.NotNull(intObj);
            Assert.AreEqual(expectedVal, intObj.Value, $"Failed: {input}");
        }

        private static void TestBooleanObject(MonkeyObject obj, bool expectedVal, string input)
        {
            var boolObj = obj as MonkeyBoolean;
            Assert.NotNull(boolObj);
            Assert.AreEqual(expectedVal, boolObj.Value, $"Failed: {input}");
        }

        [Test]
        public void TestEvalIntegerExpression()
        {
            var testTable = new List<(string Input, long ExpectedVal)>
            {
                new("5", 5),
                new("10", 10),
                new("-5", -5),
                new("-0", 0),
                new("-0", -0),
                new("5 * 5;", 25),
                new("5 + 5 + 10;", 20),
                new("5 - 5;", 0),
                new("5 - 5 - 5;", -5),
                new("2 * 10;", 20),
                new("100 / 10;", 10),
                new("100 / 10 * 5;", 50),
                new("100 / (10 * 10);", 1),
                new("0 / (10 * 10) + -10;", -10),
                new("-5 - -5", 0),
                new("++5", 6),
                new("++(++5)", 7),
                new("--(++5)", 5),
                new("--(--5)", 3),
                new("5--", 5),
                new("5++", 5),

                new("true ? 5 : 3", 5),
                new("!true ? 5 : 3", 3),
                new("1? 5 : 3", 5),
                new("0? 5 : 3", 3),
                new("0? 5 : 3", 3),
                new("(0 && (a + b))? 5 : 3", 3),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                TestIntegerObject(evaluated, item.ExpectedVal, item.Input);
            }
        }

        [Test]
        public void TestEvalBooleanExpression()
        {
            var testTable = new List<(string Input, bool ExpectedVal)>
            {
                new("true", true),
                new("false", false),
                new("1 < 2;", true),
                new("1 > 2;", false),
                new("1 > 1;", false),
                new("1 == 1;", true),
                new("1 != 2;", true),
                new("1 != 1;", false),
                new("!(1 != 1);", true),
                new("!(!(1 != 1));", false),

                new("true == true;", true),
                new("true == false;", false),
                new("true != false;", true),
                new("true != true;", false),
                new("(1 < 2) != true;", false),
                new("(1 > 2) != true;", true),
                new("(1 != 2) != true;", false),
                new("(2 == 2) == true;", true),
                new("true ? true : false;", true),
                new("true ? false : true;", false),
                new("false ? false : true;", true),
                new("false ? true : false;", false),
                new("true && false", false),
                new("true && true", true),
                new("true || false", true),
                new("false || false && true", false),
                new("0 && (a + b || c + d)", false),
                new("1 && (1 + 2) || false", true),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                TestBooleanObject(evaluated, item.ExpectedVal, item.Input);
            }
        }

        [Test]
        public void TestBangPrefixOperator()
        {
            var testTable = new List<(string Input, bool ExpectedVal)>
            {
                new("!true", false),
                new("!false", true),
                new("!5", false),
                new("!0", true),
                new("!!!0", true),
                new("!1", false),
                new("!!true", true),
                new("!!false", false)
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                TestBooleanObject(evaluated, item.ExpectedVal, item.Input);
            }
        }


        [Test]
        public void TestIfElseExpressions()
        {
            var testTable = new List<(string Input, long? ExpectedVal)>
            {
                new("if( false ) {10} else{5}", 5),
                new("if( true) {10}", 10),
                new("if( 10 < 5 ) {10} else{ -5;}", -5),
                new("if( 0 == 1 ) {10} else{ 20;}", 20),
                new("if( 1 > 0 ) {10} else{ 20;}", 10),
                new("if( 1 < 0 ) {10}", null),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                if (item.ExpectedVal != null)
                    TestIntegerObject(evaluated, item.ExpectedVal.Value, item.Input);
                else
                    Assert.AreEqual(MonkeyNull.NullObject, evaluated);
            }
        }

        [Test]
        public void TestReturnExpressions()
        {
            var testTable = new List<(string Input, long? ExpectedVal)>
            {
                new("return 5;", 5),
                new("return 10;9;", 10),
                new("return 1 < 2 ? 10 : 0;a + b;", 10),
                new("return 2 * 5;a + b;", 10),
                new("return;", null),
                new("if(1){return 5;}", 5),
                new("if(0){return 5;}else{return -5;}", -5),
                new(@"if(2 > 1){
                        if(true)
                        {
                            return 1;
                        }
                      }
                    return 10;", 1),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                if (item.ExpectedVal != null)
                {
                    var returnVal = (MonkeyInteger) evaluated;
                    TestIntegerObject(returnVal, item.ExpectedVal.Value, item.Input);
                }
                else
                    Assert.AreEqual(MonkeyNull.NullObject, evaluated);
            }
        }

        [Test]
        public void TestErrorMessages()
        {
            var testTable = new List<(string Input, string ExpectedMsg)>
            {
                new("5 + true", "Error: type mismatch: Integer + Boolean;"),
                new("5 + true;5;", "Error: type mismatch: Integer + Boolean;"),
                new("-true;", "Error: unsupported prefix: -Boolean;"),
                new("--true;", "Error: unsupported prefix: --Boolean;"),
                new("++true;", "Error: unsupported prefix: ++Boolean;"),
                new("true + false;", "Error: unsupported infix: Boolean + Boolean;"),
                new("if(10 > 1) {true + false;};", "Error: unsupported infix: Boolean + Boolean;"),
                new("if(10 > 1) {if(1){true + false;}} return 1;", "Error: unsupported infix: Boolean + Boolean;"),
                new("5;true + false;true;", "Error: unsupported infix: Boolean + Boolean;"),
                new("foobar;", "Error: identifier not found: foobar;"),
                new("if(b == 1) {return 5;}", "Error: identifier not found: b;"),
                new("let b = 1; (b + 1) = 2;", "Error: Invalid expressions appear during parsing.;"),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                Assert.AreEqual(item.ExpectedMsg, evaluated.Inspect());
            }
        }

        [Test]
        public void TestLetStatements()
        {
            var testTable = new List<(string Input, long expectedVal)>
            {
                new ("let a = 5;a;",5),
                new ("let a = 5 - 5;a;",0),
                new ("let a = 5 * 5 - 5;a;",20),
                new ("let a = 1;let b = 2;let c = 3;a + b + c;",6),
                new ("let a = 1;let b = a;b;",1),
                new ("let b = 1;if(b == 1) {10} else {1}",10),
                new("let b = 1;b = 2;b;", 2),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                TestIntegerObject(evaluated, item.expectedVal, item.Input);
            }
        }
        
        [Test]
        public void TestWhileExpression()
        {
            var testTable = new List<(string Input, long expectedVal)>
            {
                new ("let a = 1;while(a < 5){a++;};a;",5),
                new ("let a = 1;while(true){ a++;if(a > 5) {return 100;}};",100),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                TestIntegerObject(evaluated, item.expectedVal, item.Input);
            }
        }
    }
}