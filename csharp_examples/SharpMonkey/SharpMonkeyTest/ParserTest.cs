using System;
using System.Collections.Generic;
using System.Linq;
using NUnit.Framework;
using SharpMonkey;

namespace SharpMonkeyTest
{
    public class ParserTest
    {
        [SetUp]
        public void Setup()
        {
        }

        static public void CheckParserErrors(in Parser p)
        {
            foreach (var msg in p.Errors)
            {
                Console.WriteLine(msg);
            }
        }

        private void CheckLetStatement(Ast.IStatement statement, string name)
        {
            Assert.IsNotNull(statement);
            Assert.AreEqual(statement.TokenLiteral(), "let");
            var letStatement = statement as Ast.LetStatement;
            Assert.IsNotNull(letStatement);
            Assert.AreEqual(letStatement.Name.TokenLiteral(), name);
            Assert.AreEqual(letStatement.Name.Value, name);
        }

        private void CheckIntegerLiteral(Ast.IExpression exp, long value)
        {
            var numExp = exp as Ast.IntegerLiteral;
            Assert.IsNotNull(numExp);
            Assert.AreEqual(value, numExp.Value);
            Assert.AreEqual(value.ToString(), numExp.TokenLiteral());
        }

        private void CheckIdentifier(Ast.IExpression exp, string value)
        {
            var ident = exp as Ast.Identifier;
            Assert.IsNotNull(ident);
            Assert.AreEqual(value, ident.Value);
            Assert.AreEqual(value, ident.TokenLiteral());
        }

        private void CheckBooleanLiteral(Ast.IExpression exp, bool value)
        {
            var ident = exp as Ast.BooleanLiteral;
            Assert.IsNotNull(ident);
            Assert.AreEqual(value, ident.Value);
            string expectedBool = value ? "true" : "false";
            Assert.AreEqual(expectedBool, ident.TokenLiteral());
            Assert.AreEqual(expectedBool, ident.ToPrintableString());
        }
        
        private void CheckDoubleLiteral(Ast.IExpression exp, double value)
        {
            var ident = exp as Ast.DoubleLiteral;
            Assert.IsNotNull(ident);
            Assert.AreEqual(value, ident.Value);
        }

        // more general version
        private void CheckExpression(Ast.IExpression exp, object val)
        {
            switch (val)
            {
                case string value:
                    CheckIdentifier(exp, value);
                    break;
                case long value:
                    CheckIntegerLiteral(exp, value);
                    break;
                case int value:
                    CheckIntegerLiteral(exp, value);
                    break;
                case bool value:
                    CheckBooleanLiteral(exp, value);
                    break;
                case double value:
                    CheckDoubleLiteral(exp,value);
                    break;
                default:
                    Assert.Fail($"{val.GetType().Name} is not supported in CheckExpression");
                    break;
            }
        }

        private void CheckInfixExpression(Ast.IExpression infixExp, Object left, string op, Object right)
        {
            var exp = infixExp as Ast.InfixExpression;
            Assert.IsNotNull(exp);
            Assert.AreEqual(op, exp.Operator);
            CheckExpression(exp.Left, left);
            CheckExpression(exp.Right, right);
        }

        [Test]
        public void TestLetStatements()
        {
            var testTable = new List<(string Input, string identifier, Object value)>
            {
                new("let x = 5;\r\n", "x", 5),
                new("let x = foo;\r\n", "x", "foo"),
                new("let y = false;\r\n", "y", false)
            };
            foreach (var item in testTable)
            {
                Lexer lexer = new(item.Input);
                Parser parser = new Parser(lexer);

                var program = parser.ParseProgram();
                Assert.NotNull(program);
                Assert.NotNull(program.Statements);
                CheckParserErrors(parser);
                Assert.AreEqual(1, program.Statements.Count);
                CheckLetStatement(program.Statements[0], item.identifier);
                var letStmt = program.Statements[0] as Ast.LetStatement;
                Assert.NotNull(letStmt);
                CheckExpression(letStmt.Value, item.value);
            }
        }

        [Test]
        public void TestLetStatementsErrors()
        {
            var input = "let x 5;\r\n let  = 10;\r\n let 838 383;\0";

            Lexer lexer = new(input);
            Parser parser = new Parser(lexer);

            var program = parser.ParseProgram();
            CheckParserErrors(parser);
            // 以上不合法的语句可能有多种解析报错方式，但是至少有3个错误
            Assert.GreaterOrEqual(parser.Errors.Count, 3);
        }

        [Test]
        public void TestReturnStatements()
        {
            var testTable = new List<(string Input, Object value, string DebugFormat)>
            {
                new("return 5;\r\n", 5, "return 5;"),
                new("return 5.0;\r\n", 5.0, "return 5.0;"),
                new("return x;\r\n", "x", "return x;"),
                new("return;\r\n", null, "return ;"),
            };

            foreach (var item in testTable)
            {
                Lexer lexer = new(item.Input);
                Parser parser = new Parser(lexer);

                var program = parser.ParseProgram();
                Assert.AreEqual(0, parser.Errors.Count);
                CheckParserErrors(parser);
                Assert.AreEqual(1, program.Statements.Count);
                var stmt = program.Statements[0] as Ast.ReturnStatement;
                Assert.NotNull(stmt);
                Assert.AreEqual("return", stmt.TokenLiteral());
                if (item.value != null)
                {
                    CheckExpression(stmt.ReturnValue, item.value);
                }
                else
                {
                    Assert.IsNull(stmt.ReturnValue);
                }

                Assert.AreEqual(stmt.ToPrintableString(), item.DebugFormat);
            }

            {
                // special case
                var input = "return x + y;\r\n\0";
                Lexer lexer = new(input);
                Parser parser = new Parser(lexer);

                var program = parser.ParseProgram();
                Assert.AreEqual(0, parser.Errors.Count);
                CheckParserErrors(parser);
                Assert.AreEqual(1, program.Statements.Count);
                var stmt = program.Statements[0] as Ast.ReturnStatement;
                Assert.NotNull(stmt);
                Assert.AreEqual("return", stmt.TokenLiteral());
                var infixExp = stmt.ReturnValue as Ast.InfixExpression;
                Assert.NotNull(infixExp);
                CheckExpression(infixExp.Left, "x");
                CheckExpression(infixExp.Right, "y");
                Assert.AreEqual("+", infixExp.Operator);
            }
        }

        [Test]
        public void TestToPrintString()
        {
            var program = new Ast.MonkeyProgram();

            var letToken = new Token(Constants.Let, "let");

            var myVarToken = new Token(Constants.Ident, "myVar");
            var leftIdentifier = new Ast.Identifier(myVarToken, "myVar");

            var anotherVar = new Token(Constants.Ident, "anotherVar");
            var rightIdentifier = new Ast.Identifier(anotherVar, "anotherVar");

            var letStatement = new Ast.LetStatement(letToken)
            {
                Name = leftIdentifier,
                Value = rightIdentifier // 这就是为什么Identifier需要是IExpression的子类，因为它可以出现在等号右边作为表达式
            };
            program.Statements.Add(letStatement);

            Assert.AreEqual("let myVar = anotherVar;", program.ToPrintableString().TrimEnd());
        }

        [Test]
        public void TestIdentifierExpression()
        {
            // 测试类似于 foobar;这类的最简单的表达式
            var input = "foobar;\r\n";
            var l = new Lexer(input);
            var p = new Parser(l);
            var program = p.ParseProgram();

            Assert.AreEqual(0, p.Errors.Count);
            Assert.AreEqual(1, program.Statements.Count); // 这一句构成一个ExpressionStatement
            var stmt = program.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            var identifier = stmt.Expression as Ast.Identifier;
            Assert.NotNull(identifier);
            CheckIdentifier(identifier, "foobar");
        }

        [Test]
        public void TestIntegerExpression()
        {
            var input = "5;\r\n";
            var l = new Lexer(input);
            var p = new Parser(l);
            var program = p.ParseProgram();

            Assert.AreEqual(0, p.Errors.Count);
            Assert.AreEqual(1, program.Statements.Count);
            var stmt = program.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            var identifier = stmt.Expression as Ast.IntegerLiteral;
            Assert.NotNull(identifier);
            CheckIntegerLiteral(identifier, 5);
        }
        
        [Test]
        public void TestDoubleExpression()
        {
            var input = "5e2;\r\n";
            var l = new Lexer(input);
            var p = new Parser(l);
            var program = p.ParseProgram();

            Assert.AreEqual(0, p.Errors.Count);
            Assert.AreEqual(1, program.Statements.Count);
            var stmt = (Ast.ExpressionStatement) program.Statements[0];
            var identifier = (Ast.DoubleLiteral) stmt.Expression;
            Assert.AreEqual(5e2,identifier.Value);
        }


        [Test]
        public void TestPrefixExpression()
        {
            // named tuple
            var testTable = new List<(string Input, string PrefixOp, Object value, string DebugFormat)>
            {
                new("!5;", "!", 5, "(!5)"),
                new("!5.0;", "!", 5.0, "(!5.0)"),
                new("-15;", "-", 15, "(-15)"),
                new("-15e1;", "-", 15e1, "(-15e1)"),
                new("!true;", "!", true, "(!true)"),
                new("!false;", "!", false, "(!false)"),
            };
            foreach (var item in testTable)
            {
                var l = new Lexer(item.Input);
                var p = new Parser(l);
                var program = p.ParseProgram();
                CheckParserErrors(p);
                Assert.AreEqual(0, p.Errors.Count);
                Assert.AreEqual(1, program.Statements.Count);
                var stmt = program.Statements[0] as Ast.ExpressionStatement;
                Assert.NotNull(stmt);
                var exp = stmt.Expression as Ast.PrefixExpression;
                Assert.NotNull(exp);
                Assert.AreEqual(item.PrefixOp, exp.Operator);
                CheckExpression(exp.Right, item.value);
                Assert.AreEqual(exp.ToPrintableString(), item.DebugFormat);
            }
        }


        [Test]
        public void TestInfixExpression()
        {
            // named tuple
            var testTable = new List<(string Input, object LeftValue, string Operator, object RightValue, string
                DebugFormat)>
            {
                new("5 + 5;", 5, "+", 5, "(5 + 5)"),
                new("5 - 5;", 5, "-", 5, "(5 - 5)"),
                new("5 * 5;", 5, "*", 5, "(5 * 5)"),
                new("5 / 5;", 5, "/", 5, "(5 / 5)"),
                new("5 > 5;", 5, ">", 5, "(5 > 5)"),
                new("5 < 5;", 5, "<", 5, "(5 < 5)"),
                new("5 == 5;", 5, "==", 5, "(5 == 5)"),
                new("5 != 5;", 5, "!=", 5, "(5 != 5)"),
                
                new("5 != 5.0;", 5, "!=", 5.0, "(5 != 5.0)"),
                new("5 + 5.0;", 5, "+", 5.0, "(5 + 5.0)"),
            };
            foreach (var item in testTable)
            {
                var l = new Lexer(item.Input);
                var p = new Parser(l);
                var program = p.ParseProgram();
                CheckParserErrors(p);
                Assert.AreEqual(0, p.Errors.Count);
                Assert.AreEqual(1, program.Statements.Count);
                var stmt = program.Statements[0] as Ast.ExpressionStatement;
                Assert.NotNull(stmt);
                CheckInfixExpression(stmt.Expression, item.LeftValue, item.Operator, item.RightValue);
                Assert.AreEqual(stmt.ToPrintableString(), item.DebugFormat);
            }
        }

        [Test]
        public void TestInfixExpressionPrecedence()
        {
            // named tuple
            var testTable = new List<(string Input, string DebugFormat)>
            {
                new("-a * b;", "((-a) * b)"),
                new("!-a;", "(!(-a))"),
                new("!++a;", "(!(++a))"),
                new("!--a;", "(!(--a))"),
                new("!a--;", "(!(a--))"),
                new("!a++;", "(!(a++))"),
                new("a + b + c;", "((a + b) + c)"),
                new("a + b - c;", "((a + b) - c)"),
                new("a + b * c;", "(a + (b * c))"),
                new("a + b / c;", "(a + (b / c))"),
                new("a * b + b / c;", "((a * b) + (b / c))"),
                new("a * b != b / c;", "((a * b) != (b / c))"),
                new("a * b == b / c;", "((a * b) == (b / c))"),
                new("a * b == b / c * d * 3;", "((a * b) == (((b / c) * d) * 3))"),
                new("a ? b : c;", "(a ? b : c)"),
                new("a ? ++b : --c;", "(a ? (++b) : (--c))"),
                new("!a ? ++b * 2 : --c;", "((!a) ? ((++b) * 2) : (--c))"),
                new("!a ? ++b / 2 : --!c == 1;", "((!a) ? ((++b) / 2) : ((--(!c)) == 1))"),
                new("a * b == true;", "((a * b) == true)"),
                new("a * b != false;", "((a * b) != false)"),
                //parens
                new("a * (b + c);", "(a * (b + c))"),
                new("!(true == true);", "(!(true == true))"),
                new("(5 + 5) * 2;", "((5 + 5) * 2)"),
                new("1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4)"),
                new("1 + (2);", "(1 + 2)"),
                //calls
                new("1 + func(2 * b);", "(1 + func((2 * b)))"),
                new("func(c) + func(2 * b, a);", "(func(c) + func((2 * b), a))"),
                new("func(c) + func(2 * (b + d), foo(b / 2));", "(func(c) + func((2 * (b + d)), foo((b / 2))))"),
                //logic and/or
                new("a && b;", "(a && b)"),
                new("a && b || c;", "((a && b) || c)"),
                new("a && b && c;", "((a && b) && c)"),
                new("a && (b || c) && c;", "((a && (b || c)) && c)"),
                new("a > b && c < d;", "((a > b) && (c < d))"),
            };
            foreach (var item in testTable)
            {
                var l = new Lexer(item.Input);
                var p = new Parser(l);
                var program = p.ParseProgram();
                CheckParserErrors(p);
                Assert.AreEqual(0, p.Errors.Count);
                Assert.AreEqual(1, program.Statements.Count);
                var stmt = program.Statements[0] as Ast.ExpressionStatement;
                Assert.NotNull(stmt);
                Assert.AreEqual(stmt.ToPrintableString(), item.DebugFormat);
            }
        }

        [Test]
        public void TestBooleanExpression()
        {
            var testTable = new List<(string Input, bool val)>
            {
                new("true;", true),
                new("false;", false),
            };
            foreach (var item in testTable)
            {
                var l = new Lexer(item.Input);
                var p = new Parser(l);
                var program = p.ParseProgram();

                Assert.AreEqual(0, p.Errors.Count);
                Assert.AreEqual(1, program.Statements.Count);

                var stmt = program.Statements[0] as Ast.ExpressionStatement;
                Assert.NotNull(stmt);
                CheckExpression(stmt.Expression, item.val);
            }
        }

        [Test]
        public void TestStringLiteralParsingSimple()
        {
            var Input = "\"foobar你好\";\"\";";
            var p = new Parser(new Lexer(Input));
            var program = p.ParseProgram();
            CheckParserErrors(p);
            Assert.AreEqual(0, p.Errors.Count);
            Assert.AreEqual(2, program.Statements.Count);

            var stmt = program.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            var exp = stmt.Expression as Ast.StringLiteral;
            Assert.NotNull(exp);

            Assert.AreEqual("foobar你好", exp.TokenLiteral());
            Assert.AreEqual("\"foobar你好\"", exp.ToPrintableString());

            stmt = program.Statements[1] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            exp = stmt.Expression as Ast.StringLiteral;
            Assert.NotNull(exp);

            Assert.AreEqual("", exp.TokenLiteral());
            Assert.AreEqual("\"\"", exp.ToPrintableString());
        }

        [Test]
        public void TestIfExpression()
        {
            var input = "if (x < y && c > d) { return x;} else { false; } ";
            var l = new Lexer(input);
            var p = new Parser(l);
            var program = p.ParseProgram();

            CheckParserErrors(p);
            Assert.AreEqual(0, p.Errors.Count);
            Assert.AreEqual(1, program.Statements.Count);

            var stmt = program.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            var exp = stmt.Expression as Ast.IfExpression;
            Assert.NotNull(exp);
            var complexCondition = (Ast.InfixExpression) exp.Condition;
            CheckInfixExpression(complexCondition.Left, "x", "<", "y");
            CheckInfixExpression(complexCondition.Right, "c", ">", "d");
            Assert.AreEqual("&&", complexCondition.Operator);
            var thenStmt = exp.Consequence.Statements[0] as Ast.ReturnStatement;
            Assert.NotNull(thenStmt);
            Assert.AreEqual("return", thenStmt.TokenLiteral());
            CheckIdentifier(thenStmt.ReturnValue, "x");

            var alterStmt = exp.Alternative.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(alterStmt);
            var alterBoolean = alterStmt.Expression as Ast.BooleanLiteral;
            CheckBooleanLiteral(alterBoolean, false);
        }

        [Test]
        public void TestFunctionLiteralParsingSimple()
        {
            var Input = "fn() {}";
            var p = new Parser(new Lexer(Input));
            var program = p.ParseProgram();
            CheckParserErrors(p);
            Assert.AreEqual(0, p.Errors.Count);
            Assert.AreEqual(1, program.Statements.Count);

            var stmt = program.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            var exp = stmt.Expression as Ast.FunctionLiteral;
            Assert.NotNull(exp);

            Assert.AreEqual("fn", exp.TokenLiteral());
            Assert.AreEqual(0, exp.Parameters.Count);
            Assert.AreEqual(0, exp.FuncBody.Statements.Count);
        }


        [Test]
        public void TestFunctionLiteralParsing()
        {
            var Input = "fn(x,y) { x + y;}";
            var p = new Parser(new Lexer(Input));
            var program = p.ParseProgram();
            CheckParserErrors(p);
            Assert.AreEqual(0, p.Errors.Count);
            Assert.AreEqual(1, program.Statements.Count);

            var stmt = program.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            var exp = stmt.Expression as Ast.FunctionLiteral;
            Assert.NotNull(exp);

            Assert.AreEqual("fn", exp.TokenLiteral());
            Assert.AreEqual(2, exp.Parameters.Count);
            CheckIdentifier(exp.Parameters[0], "x");
            CheckIdentifier(exp.Parameters[1], "y");

            Assert.AreEqual(1, exp.FuncBody.Statements.Count);
            var bodyStmt = exp.FuncBody.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(bodyStmt);
            CheckInfixExpression(bodyStmt.Expression, "x", "+", "y");
        }

        [Test]
        public void TestCallExpressionParsing()
        {
            var Input = "func(1,2*3,4+5);";
            var p = new Parser(new Lexer(Input));
            var program = p.ParseProgram();
            CheckParserErrors(p);
            Assert.AreEqual(0, p.Errors.Count);
            Assert.AreEqual(1, program.Statements.Count);

            var stmt = program.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            var exp = stmt.Expression as Ast.CallExpression;
            Assert.NotNull(exp);

            CheckIdentifier(exp.Function, "func");
            Assert.AreEqual("(", exp.TokenLiteral());
            Assert.AreEqual(3, exp.Arguments.Count);

            CheckExpression(exp.Arguments[0], 1);
            CheckInfixExpression(exp.Arguments[1], 2, "*", 3);
            CheckInfixExpression(exp.Arguments[2], 4, "+", 5);
            Assert.AreEqual("func(1, (2 * 3), (4 + 5))", exp.ToPrintableString());
        }

        [Test]
        public void TestWhileExpression()
        {
            var input = "while(a < b){return x;}";
            var l = new Lexer(input);
            var p = new Parser(l);
            var program = p.ParseProgram();

            CheckParserErrors(p);
            Assert.AreEqual(0, p.Errors.Count);
            Assert.AreEqual(1, program.Statements.Count);

            var stmt = program.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            var exp = stmt.Expression as Ast.WhileExpression;
            Assert.NotNull(exp);
            CheckInfixExpression(exp.Condition, "a", "<", "b");
            var thenStmt = exp.Body.Statements[0] as Ast.ReturnStatement;
            Assert.NotNull(thenStmt);
            Assert.AreEqual("return", thenStmt.TokenLiteral());
            CheckIdentifier(thenStmt.ReturnValue, "x");
        }
    }
}