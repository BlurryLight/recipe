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
    }
}