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

        private void CheckParserErrors(in Parser p)
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

        // more general version
        private void CheckExpression(Ast.IExpression exp, Object val)
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
            var input = "let x = 5;\r\n let y = 10;\r\n let foobar = 838383;\0";

            Lexer lexer = new(input);
            Parser parser = new Parser(lexer);

            var program = parser.ParseProgram();
            Assert.NotNull(program);
            Assert.NotNull(program.Statements);
            Assert.AreEqual(3, program.Statements.Count);
            List<string> expectedIdentifiers = new()
            {
                "x", "y", "foobar"
            };
            for (int i = 0; i < expectedIdentifiers.Count; i++)
            {
                CheckLetStatement(program.Statements[i], expectedIdentifiers[i]);
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
            var input = "return 5;\r\n return 10;\r\n return 993 3322;\0";
            Lexer lexer = new(input);
            Parser parser = new Parser(lexer);

            var program = parser.ParseProgram();
            Assert.AreEqual(3, program.Statements.Count);
            foreach (var statement in program.Statements)
            {
                Assert.AreEqual("return", statement.TokenLiteral());
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
        public void TestPrefixExpression()
        {
            // named tuple
            var testTable = new List<(string Input, string PrefixOp, Object value, string DebugFormat)>
            {
                new("!5;", "!", 5, "(!5)"),
                new("-15;", "-", 15, "(-15)"),
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
            var testTable = new List<(string Input, long LeftValue, string Operator, long RightValue, string
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
        public void TestIfExpression()
        {
            var input = "if (x < y) { return 5;} else { false; } ";
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
            CheckInfixExpression(exp.Condition, "x", "<", "y");
            var thenStmt = exp.Consequence.Statements[0] as Ast.ReturnStatement;
            Assert.NotNull(thenStmt);
            Assert.AreEqual("return", thenStmt.TokenLiteral());
            CheckIdentifier(thenStmt.ReturnValue, "x");

            var alterStmt = exp.Alternative.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(alterStmt);
            var alterBoolean = alterStmt.Expression as Ast.BooleanLiteral;
            CheckBooleanLiteral(alterBoolean, false);
        }
    }
}