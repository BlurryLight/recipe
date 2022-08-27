using System;
using System.Diagnostics;

namespace SharpMonkey
{
    public class Lexer
    {
        public string InputContent;
        public int CurPos; // Lexer需要两个游标，一个是当前在读的字符串，一个是下一个字符的指针，用来读取 != 这种组合token
        public int ReadPos;

        // TODO: There is no unicode support.
        public char CurCh;

        public Lexer(string input)
        {
            InputContent = input;
            CurPos = 0;
            ReadPos = 0;
            CurCh = '\0';
            ReadChar();
        }

        private static bool IsAsciiLetter(char c)
        {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
        }

        private void SkipWhitespace()
        {
            while (CurCh == ' ' || CurCh == '\t' || CurCh == '\n' || CurCh == '\r')
            {
                ReadChar();
            }
        }

        public Token NextToken()
        {
            Token token = new Token();
            SkipWhitespace();
            switch (CurCh)
            {
                case '=':
                    token = new Token(Constants.Assign, CurCh.ToString());
                    break;
                case '+':
                    token = new Token(Constants.Plus, CurCh.ToString());
                    break;
                case '-':
                    token = new Token(Constants.Minus, CurCh.ToString());
                    break;
                case '*':
                    token = new Token(Constants.Asterisk, CurCh.ToString());
                    break;
                case '/':
                    token = new Token(Constants.Slash, CurCh.ToString());
                    break;
                case ';':
                    token = new Token(Constants.Semicolon, CurCh.ToString());
                    break;
                case ',':
                    token = new Token(Constants.Comma, CurCh.ToString());
                    break;
                case '(':
                    token = new Token(Constants.LParen, CurCh.ToString());
                    break;
                case ')':
                    token = new Token(Constants.RParen, CurCh.ToString());
                    break;
                case '{':
                    token = new Token(Constants.LBrace, CurCh.ToString());
                    break;
                case '}':
                    token = new Token(Constants.RBrace, CurCh.ToString());
                    break;
                case '\0':
                    token = new Token(Constants.Eof, "EOF");
                    break;
                default:
                    if (IsAsciiLetter(CurCh))
                    {
                        token = new Token(Constants.Ident, ReadIdentifier());
                        token.Type = Constants.LookupIdent(token.Literal);
                        // this line is necessary. 
                        // 在ReadIdentifier()里我们循环调用了`ReadChar`,此时CurPos指标已经在合适的位置，如果再调用一次ReadChar就会跳过一个字符
                        return token;
                    }
                    else if (char.IsDigit(CurCh))
                    {
                        token = new Token(Constants.Int, ReadDigit());
                        return token;
                    }
                    else
                    {
                        token = new Token(Constants.Illegal, CurCh.ToString());
                    }
                    break;
            }

            ReadChar();
            return token;
        }

        private string ReadIdentifier()
        {
            var pos = this.CurPos;
            while (IsAsciiLetter(CurCh))
            {
                ReadChar();
            }
            return InputContent.Substring(pos, CurPos - pos);
        }

        private string ReadDigit()
        {
            var pos = this.CurPos;
            while (char.IsDigit(CurCh))
            {
                ReadChar();
            }
            return InputContent.Substring(pos, CurPos - pos);
        }

        private void ReadChar()
        {
            if (ReadPos >= InputContent.Length)
            {
                CurCh = '\0'; // 设置为一个不合法的字符
            }
            else
            {
                CurCh = InputContent[ReadPos];
            }

            // 移动游标
            CurPos = ReadPos++;
        }
    }
}