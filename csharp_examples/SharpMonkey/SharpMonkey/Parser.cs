using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace SharpMonkey
{
    using TokenType = String;
    using PrefixParseFunc = Func<Ast.IExpression>;
    using InfixParseFunc = Func<Ast.IExpression, Ast.IExpression>;

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

    /// <summary>
    /// 优先级由低到高， 一个例子 a == b * c + !x 会被解析为 a == ( (b * c) + (!x))
    /// </summary>
    enum Priority : int
    {
        _ = 0,
        Lowest,
        Equals, // == 
        LessGreater, // > or <
        Sum, // +
        Product, // *
        Prefix, // -x or !x
        Call // a + func(b)
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

        private static readonly Dictionary<TokenType, Priority> Precedences = new()
        {
            {Constants.Eq, Priority.Equals},
            {Constants.NotEq, Priority.Equals},
            {Constants.Lt, Priority.LessGreater},
            {Constants.Gt, Priority.LessGreater},
            {Constants.Plus, Priority.Sum},
            {Constants.Minus, Priority.Sum},
            {Constants.Slash, Priority.Product},
            {Constants.Asterisk, Priority.Product},
        };

        private Priority PeekPrecedence()
        {
            if (Precedences.TryGetValue(_peekToken.Type, out Priority priority))
            {
                return priority;
            }

            return Priority.Lowest;
        }

        private Priority CurPrecedence()
        {
            if (Precedences.TryGetValue(_curToken.Type, out Priority priority))
            {
                return priority;
            }

            return Priority.Lowest;
        }

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

            // register function
            RegisterPrefixParseFunc(Constants.Ident, ParseIdentifier);
            RegisterPrefixParseFunc(Constants.Int, ParseInteger);
            RegisterPrefixParseFunc(Constants.Minus, ParsePrefixExpression);
            RegisterPrefixParseFunc(Constants.Bang, ParsePrefixExpression);
            RegisterPrefixParseFunc(Constants.Increment, ParsePrefixExpression);
            RegisterPrefixParseFunc(Constants.Decrement, ParsePrefixExpression);

            // register infix function
            RegisterInfixParseFunc(Constants.Plus, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Minus, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Asterisk, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Slash, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Eq, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.NotEq, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Lt, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Gt, ParseInfixExpression);
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

        private void AppendError(string msg)
        {
            StringBuilder outWriter = new StringBuilder();
            outWriter.AppendLine(msg);
            outWriter.AppendLine(FormatDebugContext());
            Errors.Add(outWriter.ToString());
        }

        private void PeekError(TokenType t)
        {
            AppendError($"Expected Next Token to be {t}, but got {_peekToken.Type} instead!");
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
                    return ParseExpressionStatement();
            }
        }

        private Ast.ExpressionStatement ParseExpressionStatement()
        {
            var stmt = new Ast.ExpressionStatement(_curToken)
            {
                Expression = ParseExpression(Priority.Lowest)
            };

            // 表达式内不应该包含分号，分号要扔掉
            if (_peekToken.Type == Constants.Semicolon)
            {
                NextToken();
            }

            return stmt;
        }

        private Ast.IExpression ParseExpression(Priority precedence)
        {
            if (!_prefixParseFuncMap.TryGetValue(_curToken.Type, out var prefixFunc))
            {
                AppendError($"No valid Prefix Parse Func for type {_curToken.Type} Found");
                return null;
            }

            var leftExp = prefixFunc();
            // 这里分情况
            // 1.  <exp>; 这种情况，直接返回exp
            // 2. <exp1> + <exp2> 首先先解析exp1，然后解析到+号时意识到这是一个infix的表达式，最后返回一个infix的 exp
            // 3. <exp1> + <exp2> * <exp3> 在parse +号时候会递归调用Parse expression 从而解析exp2，在解析exp2时又会再往后看，发现是*
            // 由于*的优先级大于+号，所以又会递归调用infix parse,最后返回的是  (exp1 + (exp2 * exp3))

            // 普拉特解析法的递归扫描整个表达式，递归停止的条件是遇见分号或者遇见优先级低于自己的符号。
            // a + b + c 
            // 第一层优先级Lowest
            // parse到(a + b)停止 （内层不会走循环，因为是连续的同优先级的）
            // 然后while循环还没结束，此时leftExp = (a + b) , precedence 为lowest
            // 会再parse一个 ((a + b) + c)


            // 另外一种直观的理解为，一个操作符的优先级可以代表一种吸附力
            // -1 + 3  => ((-1) + 3)
            // 因为 prefix minus的优先级较高，所以1被(-)吸过去。
            // 这决定了 1这个token，究竟是属于 Prefix - 的右表达式， 还是 +号的左表达式

            while (_peekToken.Type != Constants.Semicolon && precedence < PeekPrecedence())
            {
                if (!_infixParseFuncMap.TryGetValue(_peekToken.Type, out InfixParseFunc infixFunc))
                {
                    return leftExp;
                }

                NextToken();
                leftExp = infixFunc(leftExp);
            }

            return leftExp;
        }

        private Ast.IExpression ParseIdentifier()
        {
            var expression = new Ast.Identifier(_curToken, _curToken.Literal);
            return expression;
        }

        private Ast.IExpression ParseInteger()
        {
            var expression = new Ast.IntegerLiteral(_curToken, _curToken.Literal);
            return expression;
        }

        private Ast.IExpression ParsePrefixExpression()
        {
            var expression = new Ast.PrefixExpression(_curToken, _curToken.Literal);
            NextToken();
            // 对于 -5 这样的表达式，首先解析 TOKEN("-"),随后移动Token指针到5,然后再次递归调用ParseExpression,此时会转而调用ParseInteger
            // 以完成解析
            expression.Right = ParseExpression(Priority.Prefix);
            return expression;
        }

        private Ast.IExpression ParseInfixExpression(Ast.IExpression left)
        {
            var exp = new Ast.InfixExpression(_curToken, _curToken.Literal, left);
            // 当调用到这个函数的时候，_CurToken指向一个infix operator
            var precedence = CurPrecedence(); // 提取这个operator的优先级
            NextToken(); //移动到下一个 Expression
            exp.Right = ParseExpression(precedence);
            return exp;
        }

        public Ast.MonkeyProgram ParseProgram()
        {
            var program = new Ast.MonkeyProgram();
            while (_curToken.Type != Constants.Eof)
            {
                // 这里有个设计上的考虑，是碰见错误就停止，还是继续。
                // 有的时候前面的错误可能导致后面的parse全是错的
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