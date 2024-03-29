﻿using System.Collections.Generic;
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

        private static IMonkeyObject TestEval(string input)
        {
            var p = new Parser(new Lexer(input));
            var program = p.ParseProgram();
            ParserTest.CheckParserErrors(p);
            // Assert.AreEqual(0 ,p.Errors.Count);
            var env = new SharpMonkey.Environment();
            return Evaluator.Eval(program, env);
        }

        private static void TestIntegerObject(IMonkeyObject obj, long expectedVal, string input)
        {
            var intObj = obj as MonkeyInteger;
            var failedMsg = $"Failed: {input}, obj inspect: <{obj.Inspect()}>";
            Assert.NotNull(intObj, failedMsg);
            Assert.AreEqual(expectedVal, intObj.Value, failedMsg);
        }

        private static void TestDoubleObject(IMonkeyObject obj, double expectedVal, string input)
        {
            var failedMsg = $"Failed: {input}, obj inspect: <{obj.Inspect()}>";
            switch (obj)
            {
                case MonkeyInteger val:
                {
                    var doubleObj = val.ToMonkeyDouble();
                    Assert.AreEqual(expectedVal, doubleObj.Value, failedMsg);
                    break;
                }
                case MonkeyDouble doubleObj:
                    Assert.AreEqual(expectedVal, doubleObj.Value, failedMsg);
                    break;
                default:
                    Assert.Fail(failedMsg);
                    break;
            }
        }


        private static void TestBooleanObject(IMonkeyObject obj, bool expectedVal, string input)
        {
            var boolObj = MonkeyBoolean.ImplicitConvertFrom(obj);
            var failedMsg = $"Failed: {input}, obj inspect: <{obj.Inspect()}>";
            Assert.NotNull(boolObj, failedMsg);
            Assert.AreEqual(expectedVal, boolObj.Value, failedMsg);
        }

        [Test]
        public void TestEvalNumberExpression()
        {
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

            {
                var testTable = new List<(string Input, double ExpectedVal)>
                {
                    new("5.0", 5.0),
                    new("-5.0", -5.0),
                    new("-0.0", 0.0),
                    new("-0.0", -0.0),
                    new("1.0 ? 5 : 3", 5.0),
                    new("++5.0", 6.0),
                    new("10 / 4.0;", 2.5),
                    new("10 * 1.0;", 10.0),
                };
                foreach (var item in testTable)
                {
                    var evaluated = TestEval(item.Input);
                    TestDoubleObject(evaluated, item.ExpectedVal, item.Input);
                }
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

                new("if(false){10;}", false),
                new("!(if(false){10;})", true),
                new("null", false),
                new("!null", true),
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
                new("!true;", false),
                new("!false;", true),
                new("!5;", false),
                new("!0;", true),
                new("!!!0;", true),
                new("!1;", false),
                new("!!true;", true),
                new("!!false;", false),
                new("!1;", false),
                new("!1.0;", false),
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
                new("if( 1 < 1.2 ) {10}", 10),
                new("if( 0.9 < 1.2 ) {10}", 10),
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
                new("5 + true;", "Error: type mismatch: Integer + Boolean;"),
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
                new("\"Hello \" + 1;", "Error: type mismatch: String + Integer;"),
                new("\"Hello \" - \"H\";", "Error: unsupported infix: String - String;"),
                new("1[2]", "Error: Integer[Integer] index operator is not supported;"),
                new("{[]:1}", "Error: ArrayObj is not hashable!;"),
                new("fn(){ a = 1;}();", "Error: Try to assign an unbound value a;"),
                new("let a = 1;fn(){ a = 1;}();", "Error: Try to assign an unbound value a;"),
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
                new("let a = 5;a;", 5),
                new("let a = 5 - 5;a;", 0),
                new("let a = 5 * 5 - 5;a;", 20),
                new("let a = 1;let b = 2;let c = 3;a + b + c;", 6),
                new("let a = 1;let b = a;b;", 1),
                new("let b = 1;if(b == 1) {10} else {1}", 10),
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
                new("let a = 1;while(a < 5){a++;};a;", 5),
                // 这个测试是错误的，return不应该出现在while里
                // new("let a = 1;while(true){ a++;if(a > 5) {return 100;}};", 100),
                new("let a = 1;let i = 0;while(i<1){ i++;a = 5;};a;", 5),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                TestIntegerObject(evaluated, item.expectedVal, item.Input);
            }
        }

        [Test]
        public void TestFuncLiteralObject()
        {
            var input = "fn(x) { x + 2;}";
            var evaluated = TestEval(input);
            Assert.IsTrue(evaluated is MonkeyFuncLiteral);
            var funcObj = (MonkeyFuncLiteral) evaluated;
            Assert.AreEqual(1, funcObj.Params.Count);
            Assert.AreEqual("x", funcObj.Params[0].ToPrintableString());
            Assert.AreEqual("(x + 2)", funcObj.Body.ToPrintableString());
        }

        [Test]
        public void TestStringObject()
        {
            var input = "\"HelloWorld!\";";
            var evaluated = TestEval(input);
            Assert.IsTrue(evaluated is MonkeyString);
            var stringRef = (MonkeyString) evaluated;
            Assert.AreEqual("\"HelloWorld!\"", stringRef.Inspect());
            Assert.AreEqual("HelloWorld!", stringRef.Value);
        }

        [Test]
        public void TestFuncCall()
        {
            var testTable = new List<(string Input, long expectedVal)>
            {
                new("let identity = fn(x){x;}; identity(5);", 5),
                new("let identity = fn(x){ return x;}; identity(5);", 5),
                new("let double = fn(x){ return x * 2;}; double(5);", 10),
                new("let add= fn(x,y){ return x +y;}; add(5,5);", 10),
                new("let add= fn(x,y){ return x +y;}; add(5 + 5,add(5,-5));", 10),
                new("fn(x){x*x}(5);", 25), // lambda
                new("fn(){5;}();", 5),
                // 闭包，调用newFunc返回一个新的函数。新的函数能够访问旧的函数的参数。
                // 原理是因为addFive的环境的outer是newFunc的环境，捕获了addFive作用域外的变量
                // 这个在lua里叫做上值
                new("let newFunc = fn(x){ return fn(y){x + y;};};let addFive = newFunc(5);addFive(10);", 15),
                // 函数是第一类值(因为MonkeyFuncliteral也属于MonkeyObject，所以可以作为参数传递)
                new("let add= fn(x,y){ return x+y;};let newFunc = fn(x,y,func){return func(x,y);};newFunc(5,5,add);",
                    10),
                new("let sub= fn(x,y){ return x-y;};let newFunc = fn(x,y,func){return func(x,y);};newFunc(5,5,sub);",
                    0),
                // shadow variable
                new("let a = 1;fn(){ let a = 2;a;}();", 2),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                TestIntegerObject(evaluated, item.expectedVal, item.Input);
            }
        }

        [Test]
        public void TestBuiltinFunctions()
        {
            var testTable = new List<(string Input, long? expectedVal, string errorMsg)>
            {
                // len
                new("len(\"\");", 0, ""),
                new("len(\" \");", 1, ""),
                new("len(\"hello\");", 5, ""),
                new("len(\"hello\r\n\");", 7, ""),
                new("len(\"你好\r\n\");", 4, ""),
                new("len(\"你𠈓好\");", 4, ""), // 不能正确处理代理对
                new("len(\"hello\",\"world\");", null, "Error: wrong number of arguments to len;"),
                new("len(1);", null, "Error: argument Integer to len is not supported;"),
                new("len([0,1,2]);", 3, ""),

                // array funcs
                new("let a = [1,2,3];first(a);", 1, ""),
                new("let a = [1,2,3];last(a);", 3, ""),
                new("let a = [1,2,3];let b = rest(a);b[0];", 2, ""),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                if (item.expectedVal is null)
                {
                    var errorObj = (MonkeyError) evaluated;
                    Assert.AreEqual(item.errorMsg, errorObj.Inspect());
                }
                else
                {
                    TestIntegerObject(evaluated, item.expectedVal.Value, item.Input);
                }
            }
        }

        [Test]
        public void TestArrayAndIndex()
        {
            // array declaration
            var testTable = new List<(string Input, long expectedVal)>
            {
                new("let a = [1,2 * 3, 3 + 3 + 3];a[0];", 1),
                new("let a = [1,2 * 3, 3 + 3 + 3];a[1];", 6),
                new("let a = [1,2 * 3, 3 + 3 + 3];a[2];", 9),
                // mutable array index
                new("let a = [1];a[0] = 2;a[0];", 2),

                // map
                new("let a = {\"one\":1};a[\"one\"];", 1),
                new("let a = {\"one\":1};a[\"one\"] = 2;a[\"one\"];", 2),
            };
            foreach (var item in testTable)
            {
                var evaluated = TestEval(item.Input);
                TestIntegerObject(evaluated, item.expectedVal, item.Input);
            }
        }

        [Test]
        public void TestHashKey()
        {
            {
                var string1 = "\"hello\";";
                var string2 = "\"hello\";";

                var str1Obj = (MonkeyString) TestEval(string1);
                var str2Obj = (MonkeyString) TestEval(string2);
                Assert.AreEqual(string1, string2);
                Assert.AreNotEqual(str1Obj, str2Obj);
                Assert.AreNotEqual(str1Obj.GetHashCode(), str2Obj.GetHashCode());
                Assert.AreEqual(str1Obj.Value, str2Obj.Value);

                Assert.AreEqual(str1Obj.HashKey(), str2Obj.HashKey());
                Assert.AreEqual(str1Obj.HashKey().GetHashCode(), str2Obj.HashKey().GetHashCode());
            }

            {
                var string1 = "1.1;";
                var string2 = "1.1;";

                var str1Obj = (MonkeyDouble) TestEval(string1);
                var str2Obj = (MonkeyDouble) TestEval(string2);
                Assert.AreEqual(string1, string2);
                Assert.AreNotEqual(str1Obj, str2Obj);
                Assert.AreNotEqual(str1Obj.GetHashCode(), str2Obj.GetHashCode());
                Assert.AreEqual(str1Obj.Value, str2Obj.Value);

                Assert.AreEqual(str1Obj.HashKey(), str2Obj.HashKey());
                Assert.AreEqual(str1Obj.HashKey().GetHashCode(), str2Obj.HashKey().GetHashCode());
            }

            {
                var string1 = "true;";
                var string2 = "true;";

                var str1Obj = (MonkeyBoolean) TestEval(string1);
                var str2Obj = (MonkeyBoolean) TestEval(string2);
                Assert.AreEqual(string1, string2);
                Assert.AreEqual(str1Obj.Value, str2Obj.Value);
                Assert.AreEqual(str1Obj.HashKey(), str2Obj.HashKey());
                Assert.AreEqual(str1Obj.HashKey().GetHashCode(), str2Obj.HashKey().GetHashCode());
            }
        }


        [Test]
        public void TestHashLiteral()
        {
            var input = @"let two = ""two"";
            {""one"": 10 - 9,
            two: 1+1,
            ""thr"" + ""ee"": 6 / 2,
            4:4,
            true : 5,
            false: 6,
            7.0:7
            };";

            var evaluated = TestEval(input);
            var mapObj = evaluated as MonkeyMap;
            Assert.IsNotNull(mapObj);
            var expected = new Dictionary<HashKey, int>
            {
                {new MonkeyString("one").HashKey(), 1},
                {new MonkeyString("two").HashKey(), 2},
                {new MonkeyString("three").HashKey(), 3},
                {new MonkeyInteger(4).HashKey(), 4},
                {MonkeyBoolean.GetStaticObject(true).HashKey(), 5},
                {MonkeyBoolean.GetStaticObject(false).HashKey(), 6},
                {new MonkeyDouble(7.0).HashKey(), 7}
            };
            Assert.AreEqual(expected.Count, mapObj.Pairs.Count);
            foreach (var entry in expected)
            {
                Assert.IsTrue(mapObj.Pairs.TryGetValue(entry.Key, out var value));
                TestIntegerObject(value.ValueObj, entry.Value, value.KeyObj.Inspect());
            }
        }
    }
}