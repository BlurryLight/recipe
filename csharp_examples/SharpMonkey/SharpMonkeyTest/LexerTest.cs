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

        private void CheckExpectedTokens(in Lexer lexer, in List<Token> expectedTokens)
        {
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

        [Test]
        public void TestNextTokenSymbols()
        {
            var input = "=+(){},;\0"; // C# string is not \0 terminated
            var expectedTokens = new List<Token>()
            {
                new Token(Constants.Assign, "="),
                new Token(Constants.Plus, "+"),
                new Token(Constants.LParen, "("),
                new Token(Constants.RParen, ")"),
                new Token(Constants.LBrace, "{"),
                new Token(Constants.RBrace, "}"),
                new Token(Constants.Comma, ","),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.Eof, "EOF"),
            };

            Lexer lexer = new Lexer(input);
            CheckExpectedTokens(lexer, expectedTokens);
        }

        [Test]
        public void TestNextTokenIdentifiers()
        {
            var input = "let five = 5;\r\n let ten = 10\r\n" +
                        "   let add = fn(x,y) {x + y;};" +
                        " let result = add(--five,++ten);\0";

            var expectedTokens = new List<Token>()
            {
                new Token(Constants.Let, "let"),
                new Token(Constants.Ident, "five"),
                new Token(Constants.Assign, "="),
                new Token(Constants.Int, "5"),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.Let, "let"),
                new Token(Constants.Ident, "ten"),
                new Token(Constants.Assign, "="),
                new Token(Constants.Int, "10"),
                new Token(Constants.Let, "let"),
                new Token(Constants.Ident, "add"),
                new Token(Constants.Assign, "="),
                new Token(Constants.Function, "fn"),
                new Token(Constants.LParen, "("),
                new Token(Constants.Ident, "x"),
                new Token(Constants.Comma, ","),
                new Token(Constants.Ident, "y"),
                new Token(Constants.RParen, ")"),
                new Token(Constants.LBrace, "{"),
                new Token(Constants.Ident, "x"),
                new Token(Constants.Plus, "+"),
                new Token(Constants.Ident, "y"),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.RBrace, "}"),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.Let, "let"),
                new Token(Constants.Ident, "result"),
                new Token(Constants.Assign, "="),
                new Token(Constants.Ident, "add"),
                new Token(Constants.LParen, "("),
                new Token(Constants.Decrement, "--"),
                new Token(Constants.Ident, "five"),
                new Token(Constants.Comma, ","),
                new Token(Constants.Increment, "++"),
                new Token(Constants.Ident, "ten"),
                new Token(Constants.RParen, ")"),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.Eof, "EOF"),
            };

            Lexer lexer = new Lexer(input);
            CheckExpectedTokens(lexer, expectedTokens);
        }

        [Test]
        public void TestNextTokenMore()
        {
            var input = "!-/*5;\r\n 5 < 10 > 5; 10 == 10; 5 != 4;" +
                        "if(5 < 10) {return true;} else {return false;}\0";

            var expectedTokens = new List<Token>()
            {
                new Token(Constants.Bang, "!"),
                new Token(Constants.Minus, "-"),
                new Token(Constants.Slash, "/"),
                new Token(Constants.Asterisk, "*"),
                new Token(Constants.Int, "5"),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.Int, "5"),
                new Token(Constants.Lt, "<"),
                new Token(Constants.Int, "10"),
                new Token(Constants.Gt, ">"),
                new Token(Constants.Int, "5"),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.Int, "10"),
                new Token(Constants.Eq, "=="),
                new Token(Constants.Int, "10"),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.Int, "5"),
                new Token(Constants.NotEq, "!="),
                new Token(Constants.Int, "4"),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.If, "if"),
                new Token(Constants.LParen, "("),
                new Token(Constants.Int, "5"),
                new Token(Constants.Lt, "<"),
                new Token(Constants.Int, "10"),
                new Token(Constants.RParen, ")"),
                new Token(Constants.LBrace, "{"),
                new Token(Constants.Return, "return"),
                new Token(Constants.True, "true"),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.RBrace, "}"),
                new Token(Constants.Else, "else"),
                new Token(Constants.LBrace, "{"),
                new Token(Constants.Return, "return"),
                new Token(Constants.False, "false"),
                new Token(Constants.Semicolon, ";"),
                new Token(Constants.RBrace, "}"),
                new Token(Constants.Eof, "EOF"),
            };

            Lexer lexer = new Lexer(input);
            CheckExpectedTokens(lexer, expectedTokens);
        }
    }
}