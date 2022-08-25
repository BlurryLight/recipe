using System;
using System.Collections.Generic;
using NUnit.Framework;
using SharpMonkey;

namespace SharpMonkeyTest
{
    public class LexerTest
    {
        [SetUp]
        public void Setup()
        {
        }

        [Test]
        public void TestNextToken()
        {
            var input = "=+(){},;\0"; // C# string is not \0 terminated
            var expectedTokens= new List<Token>()
            {
                new Token(Constants.Assign,"="),
                new Token(Constants.Plus,"+"),
                new Token(Constants.LParen,"("),
                new Token(Constants.RParen,")"),
                new Token(Constants.LBrace,"{"),
                new Token(Constants.RBrace,"}"),
                new Token(Constants.Comma,","),
                new Token(Constants.Semicolon,";"),
                new Token(Constants.Eof,""),
            };

            Lexer lexer = new Lexer(input);
            int count = 0;
            foreach (var expectedToken in expectedTokens)
            {
                var token = lexer.NextToken();
                Assert.AreEqual(expectedToken.Literal, token.Literal,
                    $"Error! Expected Literal {expectedToken.Literal} , Actual is {token.Literal}");
                Assert.AreEqual(expectedToken.Type, token.Type,
                    $"Error! Expected Type {expectedToken.Type} , Actual is {token.Type}");
                count++;
            }

            Assert.AreEqual(count, expectedTokens.Count);
        }
    }
}