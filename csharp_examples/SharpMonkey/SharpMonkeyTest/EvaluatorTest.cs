using System;
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
            Assert.AreEqual(0, p.Errors.Count);
            return Evaluator.Eval(program);
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
                new("if( false ) {10} else{5}",5),
                new("if( true) {10}",10),
                new("if( 10 < 5 ) {10} else{ -5;}",-5),
                new("if( 0 == 1 ) {10} else{ 20;}",20),
                new("if( 1 > 0 ) {10} else{ 20;}",10),
                new("if( 1 < 0 ) {10}",null),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                if (item.ExpectedVal != null)
                    TestIntegerObject(evaluated, item.ExpectedVal.Value, item.Input);
                else
                    Assert.AreEqual( MonkeyNull.NullObject,evaluated);
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
                    return 10;",1),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                if (item.ExpectedVal != null)
                {
                    var returnVal = (MonkeyInteger) evaluated;
                    TestIntegerObject(returnVal,item.ExpectedVal.Value,item.Input);
                }
                else
                    Assert.AreEqual( MonkeyNull.NullObject,evaluated);
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
                new("5;true + false;true;","Error: unsupported infix: Boolean + Boolean;"),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                Assert.AreEqual(item.ExpectedMsg, evaluated.Inspect());
            }
        }
    }
}