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
        private int Limit { get; init; } // set only once 

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
        Assign, // =
        Condition,
        And, // && ||
        Equals, // == 
        LessGreater, // > or <
        Sum, // +
        Product, // *
        Prefix, // -x or !x
        Postfix, // a++
        Call, // a + func(b),
        Index, // a + func(b[0]),[优先级应该大于,否则会变成func(b  [0]),会碰见找不到)的错误
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
        // postfix也可以放在infix里一起解析了
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

            {Constants.Increment, Priority.Postfix},
            {Constants.Decrement, Priority.Postfix},

            {Constants.QuestionMark, Priority.Condition},
            {Constants.And, Priority.And},
            {Constants.Or, Priority.And},
            {Constants.LParen, Priority.Call},
            {Constants.Assign, Priority.Assign},
            {Constants.LBracket, Priority.Index}
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
            int count = contextCopy.Count;
            for (int i = (contextCopy.Count - 1); i >= 0; i--)
            {
                var token = contextCopy.Dequeue();
                msg.AppendLine($"{count - i,2}: TokenType: {token.Type,10}, Token Literal: {token.Literal}\n");
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
            RegisterPrefixParseFunc(Constants.String, ParseString);
            RegisterPrefixParseFunc(Constants.Int, ParseInteger);
            RegisterPrefixParseFunc(Constants.Double, ParseDouble);
            RegisterPrefixParseFunc(Constants.Minus, ParsePrefixExpression);
            RegisterPrefixParseFunc(Constants.Bang, ParsePrefixExpression);
            RegisterPrefixParseFunc(Constants.Increment, ParsePrefixExpression);
            RegisterPrefixParseFunc(Constants.Decrement, ParsePrefixExpression);
            RegisterPrefixParseFunc(Constants.True, ParseBoolean);
            RegisterPrefixParseFunc(Constants.False, ParseBoolean);
            RegisterPrefixParseFunc(Constants.LParen, ParseGroupedExpression);
            RegisterPrefixParseFunc(Constants.If, ParseIfExpression);
            RegisterPrefixParseFunc(Constants.Function, ParseFuncLiteral);
            RegisterPrefixParseFunc(Constants.While, ParseWhileExpression);
            RegisterPrefixParseFunc(Constants.LBracket, ParseArrayLiteral);
            RegisterPrefixParseFunc(Constants.LBrace, ParseMapLiteral);

            // register infix function
            RegisterInfixParseFunc(Constants.Plus, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Minus, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Asterisk, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Slash, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Eq, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.NotEq, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Lt, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Gt, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.And, ParseInfixExpression);
            RegisterInfixParseFunc(Constants.Or, ParseInfixExpression);

            // 也许可以用ParseInfixExpression处理，但是单独领出来一个函数可以做更多的校验
            RegisterInfixParseFunc(Constants.Assign, ParseAssignExpression);

            RegisterInfixParseFunc(Constants.Increment, ParsePostfixExpression);
            RegisterInfixParseFunc(Constants.Decrement, ParsePostfixExpression);

            RegisterInfixParseFunc(Constants.QuestionMark, ParseConditionalExpression);
            // 出现 <ident>(args,...)这种情况时候，应该判定为函数调用
            RegisterInfixParseFunc(Constants.LParen, ParseCallExpression);

            RegisterInfixParseFunc(Constants.LBracket, ParseIndexExpression);
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

            NextToken(); // move to expression
            statement.Value = ParseExpression(Priority.Lowest);
            return !ExpectPeek(Constants.Semicolon) ? null : statement;
        }


        private Ast.IStatement ParseReturnStatement()
        {
            var statement = new Ast.ReturnStatement(_curToken);
            NextToken();
            // 支持  return 5; /  return; 这样的写法
            if (_curToken.Type == Constants.Semicolon)
            {
                // stmt.Value is null
                return statement;
            }

            statement.ReturnValue = ParseExpression(Priority.Lowest);
            return !ExpectPeek(Constants.Semicolon) ? null : statement;
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

        private Ast.IExpression ParseString()
        {
            var expression = new Ast.StringLiteral(_curToken, _curToken.Literal);
            return expression;
        }

        private Ast.IExpression ParseBoolean()
        {
            var expression = new Ast.BooleanLiteral(_curToken, _curToken.Literal);
            return expression;
        }

        private Ast.IExpression ParseInteger()
        {
            var expression = new Ast.IntegerLiteral(_curToken, _curToken.Literal);
            return expression;
        }

        private Ast.IExpression ParseDouble()
        {
            try
            {
                var expression = new Ast.DoubleLiteral(_curToken, _curToken.Literal);
                return expression;
            }
            catch (Exception e)
            {
                AppendError($"Parse Double {_curToken.Literal} Failed! Reason is {e.Message}");
            }

            return null;
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

        private Ast.IExpression ParseConditionalExpression(Ast.IExpression left)
        {
            // a ? b : c
            // 进入时curToken指向?
            var exp = new Ast.ConditionalExpression(_curToken, left);
            var precedence = CurPrecedence(); // 提取这个operator的优先级
            NextToken(); // 移动curToken到 b
            exp.ThenArm = ParseExpression(precedence);
            NextToken(); // consume colon
            NextToken(); // 移动curToken到c
            exp.ElseArm = ParseExpression(precedence);
            return exp;
        }

        private Ast.IExpression ParsePostfixExpression(Ast.IExpression left)
        {
            var exp = new Ast.PostfixExpression(_curToken, _curToken.Literal, left);
            return exp;
        }

        private Ast.IExpression ParseGroupedExpression()
        {
            // 目前_CurToken在 '('上
            NextToken(); // 往前移动一个token

            // BlackMagic: 把左括号的优先级设置为最低，那么自然而言括号内部的表达式就能聚合在一起成为一个整体
            // 如果括号内部是一个`infix operator`或者是`prefix op`，那么其stringify的时候这个op会自动加一个括号
            var exp = ParseExpression(Priority.Lowest);
            if (!ExpectPeek(Constants.RParen))
            {
                return null;
            }

            return exp;
        }

        private Ast.BlockStatement ParseBlockStatement()
        {
            var block = new Ast.BlockStatement(_curToken);
            NextToken();
            while (_curToken.Type != Constants.RBrace && _curToken.Type != Constants.Eof)
                // 想不到什么情况下会在{}中出现EOF,只有在不合法的输入下才会有这种情况。
            {
                var stmt = ParseStatement();
                if (stmt != null)
                {
                    block.Statements.Add(stmt);
                }

                NextToken();
            }

            if (_curToken.Type == Constants.Eof)
            {
                AppendError($"Unexpected EOF appears in ParseBlockStatement!");
            }

            return block;
        }

        private Ast.IExpression ParseIfExpression()
        {
            var exp = new Ast.IfExpression(_curToken); // if token
            if (!ExpectPeek(Constants.LParen))
            {
                return null;
            }

            // 此时cur在 ‘(’上
            exp.Condition = ParseGroupedExpression();
            // parse完以后，cur 在)上
            // 检查下一个TOKEN '{'
            if (!ExpectPeek(Constants.LBrace))
            {
                return null;
            }

            exp.Consequence = ParseBlockStatement();

            if (_peekToken.Type == Constants.Else)
            {
                NextToken(); // consume else token
                if (!ExpectPeek(Constants.LBrace))
                {
                    return null;
                }

                exp.Alternative = ParseBlockStatement();
            }

            return exp;
        }

        private Ast.IExpression ParseWhileExpression()
        {
            var exp = new Ast.WhileExpression(_curToken); // while token
            if (!ExpectPeek(Constants.LParen))
            {
                return null;
            }

            exp.Condition = ParseGroupedExpression();
            if (!ExpectPeek(Constants.LBrace))
            {
                return null;
            }

            exp.Body = ParseBlockStatement();
            return exp;
        }

        private List<Ast.Identifier> ParseFunctionParameters()
        {
            var identifiers = new List<Ast.Identifier>();
            // 参数列表为空的情况
            if (_peekToken.Type == Constants.RParen)
            {
                NextToken();
                return identifiers;
            }

            // 移动token到(下一个token，第一个参数
            // 需要手动处理第一个参数，因为第一个参数后面可能没有逗号,
            NextToken();
            identifiers.Add(ParseIdentifier() as Ast.Identifier);

            while (_peekToken.Type == Constants.Comma)
            {
                NextToken(); // 移动到逗号
                NextToken(); //移动到下一个Identifier
                identifiers.Add(ParseIdentifier() as Ast.Identifier);
            }

            // 从最后一个逗号出来了,暂时不支持变参函数
            return !ExpectPeek(Constants.RParen) ? null : identifiers;
        }

        private Ast.IExpression ParseFuncLiteral()
        {
            var fn = new Ast.FunctionLiteral(_curToken);
            if (!ExpectPeek(Constants.LParen))
            {
                return null;
            }

            fn.Parameters = ParseFunctionParameters();

            // 现在指针在右)上，看下一个token应该是{
            if (!ExpectPeek(Constants.LBrace))
            {
                return null;
            }

            fn.FuncBody = ParseBlockStatement();
            return fn;
        }

        private Ast.IExpression ParseArrayLiteral()
        {
            var exp = new Ast.ArrayLiteral(_curToken, ParseExpressionList(Constants.RBracket));
            return exp;
        }

        private List<Ast.IExpression> ParseExpressionList(TokenType endTokenType)
        {
            // 仿照ParseParameters的逻辑
            var args = new List<Ast.IExpression>();
            if (_peekToken.Type == endTokenType)
            {
                NextToken();
                return args;
            }

            NextToken();
            args.Add(ParseExpression(Priority.Lowest));
            while (_peekToken.Type == Constants.Comma)
            {
                NextToken(); // 移动到逗号
                NextToken();
                args.Add(ParseExpression(Priority.Lowest));
            }

            return !ExpectPeek(endTokenType) ? null : args;
        }

        private Ast.IExpression ParseCallExpression(Ast.IExpression left)
        {
            var exp = new Ast.CallExpression(_curToken, left)
            {
                Arguments = ParseExpressionList(Constants.RParen)
            };
            return exp;
        }

        private Ast.IExpression ParseMapLiteral()
        {
            var exp = new Ast.MapLiteral()
            {
                Token = _curToken,
                Pairs = new Dictionary<Ast.IExpression, Ast.IExpression>()
            };
            while (_peekToken.Type != Constants.RBrace)
            {
                // 从{或者,移动到第一个key
                NextToken();
                var key = ParseExpression(Priority.Lowest);
                if (!ExpectPeek(Constants.Colon)) // consume :
                {
                    return null;
                }

                // 当前token在 : 上，移动一格
                NextToken();
                var value = ParseExpression(Priority.Lowest);
                exp.Pairs.Add(key, value);
                if (_peekToken.Type != Constants.Comma && _peekToken.Type != Constants.RBrace)
                {
                    AppendError(
                        $"unexpected token {_peekToken.Type} appears in ParseMapLiteral! Expect Comma or RBrace");
                    return null;
                }

                // 如果是, 移动到，上
                if (_peekToken.Type == Constants.Comma)
                    NextToken();
            }

            if (!ExpectPeek(Constants.RBrace)) return null;
            return exp;
        }

        private Ast.IExpression ParseIndexExpression(Ast.IExpression left)
        {
            var exp = new Ast.IndexExpression(_curToken, left); // cur Token is [
            NextToken();
            exp.Index = ParseExpression(Priority.Lowest);
            return !ExpectPeek(Constants.RBracket) ? null : exp;
        }

        private Ast.IExpression ParseAssignExpression(Ast.IExpression left)
        {
            if (left is not Ast.Identifier && left is not Ast.IndexExpression)
            {
                AppendError($"Assign left must be left value!");
                return null;
            }

            var exp = new Ast.AssignExpression(_curToken, left);
            // 从 = token移动到下一个token
            NextToken();
            exp.Value = ParseExpression(Priority.Lowest);
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