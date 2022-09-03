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

        void CheckLetStatement(Ast.IStatement statement, string name)
        {
            Assert.IsNotNull(statement);
            Assert.AreEqual(statement.TokenLiteral(),"let");
            var letStatement = statement as Ast.LetStatement;
            Assert.IsNotNull(letStatement);
            Assert.AreEqual(letStatement.Name.TokenLiteral(),name);
            Assert.AreEqual(letStatement.Name.Value,name);
        }

        [Test]
        public void TestLetStatements()
        {
            var input = "let x = 5;\r\n let y = 10;\r\n let foobar = 838383;\0";
            
            Lexer lexer = new (input);
            Parser parser = new Parser(lexer);

            var program = parser.ParseProgram();
            Assert.NotNull(program);
            Assert.NotNull(program.Statements);
            Assert.AreEqual(3,program.Statements.Count);
            List<string> expectedIdentifiers = new()
            {
                "x","y","foobar"
            };
            for (int i = 0; i < expectedIdentifiers.Count; i++)
            {
                CheckLetStatement(program.Statements[i],expectedIdentifiers[i]);
            }
        }
        [Test]
        public void TestLetStatementsErrors()
        {
            var input = "let x 5;\r\n let  = 10;\r\n let 838 383;\0";
            
            Lexer lexer = new (input);
            Parser parser = new Parser(lexer);

            var program = parser.ParseProgram();
            Assert.AreEqual(3,parser.Errors.Count );
            foreach (var msg in parser.Errors)
            {
                Console.WriteLine(msg);
            }
        }

        [Test]
        public void TestReturnStatements()
        {
            var input = "return 5;\r\n return 10;\r\n return 993 3322;\0";
            Lexer lexer = new (input);
            Parser parser = new Parser(lexer);
            
            var program = parser.ParseProgram();
            Assert.AreEqual(3,program.Statements.Count);
            foreach (var statement in program.Statements)
            {
                Assert.AreEqual("return",statement.TokenLiteral());
            }
        }
        
        [Test]
        public void TestToPrintString()
        {
            var program = new Ast.MonkeyProgram();

            var letToken = new Token(Constants.Let, "let");
            
            var myVarToken = new Token(Constants.Ident, "myVar");
            var leftIdentifier = new Ast.Identifier(myVarToken, "myVar");
            
            var anotherVar= new Token(Constants.Ident, "anotherVar");
            var rightIdentifier = new Ast.Identifier(anotherVar, "anotherVar");
            
            var letStatement = new Ast.LetStatement(letToken)
            {
                Name = leftIdentifier,
                Value = rightIdentifier // 这就是为什么Identifier需要是IExpression的子类，因为它可以出现在等号右边作为表达式
            };
            program.Statements.Add(letStatement);
            
            Assert.AreEqual("let myVar = anotherVar;",program.ToPrintableString().TrimEnd());
        }

        [Test]
        public void TestIdentifierExpression()
        {
            // 测试类似于 foobar;这类的最简单的表达式
            var input = "foobar;\r\n";
            var l = new Lexer(input);
            var p = new Parser(l);
            var program = p.ParseProgram();
            
            Assert.AreEqual(0,p.Errors.Count);
            Assert.AreEqual(1,program.Statements.Count); // 这一句构成一个ExpressionStatement
            var stmt = program.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            var identifier = stmt.Expression as Ast.Identifier;
            Assert.NotNull(identifier);
            Assert.AreEqual("foobar",identifier.Value);
            Assert.AreEqual("foobar",identifier.TokenLiteral());
        }
        
        [Test]
        public void TestIntegerExpression()
        {
            var input = "5;\r\n";
            var l = new Lexer(input);
            var p = new Parser(l);
            var program = p.ParseProgram();
            
            Assert.AreEqual(0,p.Errors.Count);
            Assert.AreEqual(1,program.Statements.Count); 
            var stmt = program.Statements[0] as Ast.ExpressionStatement;
            Assert.NotNull(stmt);
            var identifier = stmt.Expression as Ast.Integerliteral;
            Assert.NotNull(identifier);
            Assert.AreEqual(5,identifier.Value);
            Assert.AreEqual("5",identifier.TokenLiteral());
        }
    }

}