using System;
using System.Collections;
using System.Collections.Generic;

namespace SharpMonkey
{
    using TokenType = String;

    public class FixedQueue<T> : Queue<T>
    {
        public int Limit { get; init; } // set only once 

        public FixedQueue(int limit) : base(limit)
        {
            Limit = limit;
        }

        public new void Enqueue(T item) // new keyword hide the original implementation
        {
            while (Count >= Limit)
            {
                Dequeue();
            }
            base.Enqueue(item);
        }
    }
    public class Parser
    {
        private Lexer Lexer;
        private Token _curToken;
        private Token _peekToken;
                                            // there is noway to declare compile-time constant size in C#
        private FixedQueue<Token> _context; // parsing context. 在出错的时候打印，打印之前已经解析的token，以判断错误。

        public Parser(Lexer lexer)
        {
            Lexer = lexer;
            _context = new FixedQueue<Token>(16);
            // 需要调用两次才能让CurToken指向第一个Token
            NextToken();
            NextToken();
        }

        public void NextToken()
        {
            _curToken = _peekToken;
            _peekToken = Lexer.NextToken();
            
            //debug
            _context.Enqueue(_curToken);
        }

        /// <summary>
        /// 如果下一个token是t类型的，那么返回true，并且当前parser前进一个token,否则返回false。
        /// </summary>
        /// <param name="t"> expected TokenType </param>
        /// <returns></returns>
        private bool ExpectPeek(TokenType t)
        {
            if (_peekToken.Type == t)
            {
                NextToken();
                return true;
            }
            return false;
        }

        private Ast.IStatement ParseLetStatement()
        {
            // let x = (expression) ;
            // let <identifier> <assign> <?> <semicolon>
            var statement = new Ast.LetStatement(_curToken);
            if (!ExpectPeek(Constants.Ident)) // 需要let 后的第一个token是identifier
            {
                return null;
            }

            statement.Name = new Ast.Identifier(token: _curToken,value: _curToken.Literal);
            
            if (!ExpectPeek(Constants.Assign))
            {
                return null;
            }
            
            // TODO: handle expression
            while(_curToken.Type == Constants.Semicolon)
            {
                NextToken();
            }

            return statement;
        }

        private Ast.IStatement ParseStatement()
        {
            switch (_curToken.Type)
            {
                
                case Constants.Let:
                    return ParseLetStatement();
                default:
                    return null;
            }
        }
        public Ast.MonkeyProgram ParseProgram()
        {
            var program = new Ast.MonkeyProgram();
            while (_curToken.Type != Constants.Eof)
            {
                var statement = ParseStatement();
                if (statement != null)
                {
                    program.Statements.Add(statement);
                }
                this.NextToken();
            }

            return program;
        }
    }
}