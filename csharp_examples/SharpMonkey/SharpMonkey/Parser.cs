using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace SharpMonkey
{
    using TokenType = String;
    using PrefixParseFunc = Func<Ast.IExpression>;
    using InfixParseFunc = Func<Ast.IExpression,Ast.IExpression>;

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

        public List<string> Errors;

        private Dictionary<TokenType, PrefixParseFunc> _prefixParseFuncMap = new();
        // 对于infix表达式，需要传入左侧的表达式。
        private Dictionary<TokenType, InfixParseFunc> _infixParseFuncMap = new();

        private void RegisterPrefixParseFunc(TokenType tokenType, in PrefixParseFunc fn)
        {
            _prefixParseFuncMap[tokenType] = fn;
        }
        
        private void RegisterInfixParseFunc(TokenType tokenType, in InfixParseFunc fn)
        {
            _infixParseFuncMap[tokenType] = fn;
        }

        private string FormatDebugContext()
        {
            StringBuilder msg = new StringBuilder();
            msg.AppendLine("Parse Error. Context is:");
            var contextCopy = _context;
            int Count = contextCopy.Count;
            for (int i = (contextCopy.Count - 1); i >= 0; i--)
            {
                var token = contextCopy.Dequeue();
                msg.AppendLine($"{Count - i,2}: TokenType: {token.Type,10}, Token Literal: {token.Literal}\n");
            }

            _context.Clear();
            return msg.ToString();
        }

        public Parser(Lexer lexer)
        {
            Lexer = lexer;
            _context = new FixedQueue<Token>(16);
            Errors = new List<string>();
            // 需要调用两次才能让CurToken指向第一个Token
            NextToken();
            NextToken();
        }

        public void NextToken()
        {
            _curToken = _peekToken;
            _peekToken = Lexer.NextToken();

            //debug
            if (_curToken != null)
            {
                _context.Enqueue(_curToken);
            }
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
            else
            {
                PeekError(t);
                return false;
            }
        }

        private void PeekError(TokenType t)
        {
            StringBuilder msg = new StringBuilder();
            msg.AppendLine($"Expected Next Token to be {t}, but got {_peekToken.Type} instead!");
            msg.AppendLine(FormatDebugContext());
            Errors.Add(msg.ToString());
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

            statement.Name = new Ast.Identifier(token: _curToken, value: _curToken.Literal);

            if (!ExpectPeek(Constants.Assign))
            {
                return null;
            }

            // TODO: handle expression
            while (_curToken.Type != Constants.Semicolon)
            {
                NextToken();
            }

            return statement;
        }


        private Ast.IStatement ParseReturnStatement()
        {
            var statement = new Ast.ReturnStatement(_curToken);
            NextToken();
            // TODO: handle expression
            while (_curToken.Type != Constants.Semicolon)
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
                case Constants.Return:
                    return ParseReturnStatement();
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