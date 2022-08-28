using System.Collections.Generic;
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
            Assert.AreEqual(program.Statements.Count,3);
            List<string> expectedIdentifiers = new()
            {
                "x","y","foobar"
            };
            for (int i = 0; i < expectedIdentifiers.Count; i++)
            {
                CheckLetStatement(program.Statements[i],expectedIdentifiers[i]);
            }
        }
    }

}