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

        public Token NextToken()
        {
            Token token = new Token();
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
                    token = new Token(Constants.Eof, "");
                    break;
            }

            ReadChar();
            return token;
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