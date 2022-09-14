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
            Assert.AreEqual(0,p.Errors.Count);
            return Evaluator.Eval(program);
        }

        private static void TestIntegerObject(MonkeyObject obj, long expectedVal)
        {
            var intObj = obj as MonkeyInteger;
            Assert.NotNull(intObj);
            Assert.AreEqual(intObj.Value,expectedVal);
        }
        
        private static void TestBooleanObject(MonkeyObject obj, bool expectedVal)
        {
            var boolObj = obj as MonkeyBoolean;
            Assert.NotNull(boolObj);
            Assert.AreEqual(boolObj.Value,expectedVal);
        }

        [Test]
        public void TestEvalIntegerExpression()
        {
            var testTable = new List<(string Input, long ExpectedVal)>
            {
                new("5",5),
                new("10",10)
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                TestIntegerObject(evaluated, item.ExpectedVal);
            }
        }
        
        [Test]
        public void TestEvalBooleanExpression()
        {
            var testTable = new List<(string Input, bool ExpectedVal)>
            {
                new("true",true),
                new("false",false)
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                TestBooleanObject(evaluated, item.ExpectedVal);
            }
        }
    }
}