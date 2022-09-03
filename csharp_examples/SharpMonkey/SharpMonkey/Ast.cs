using System;
using System.Collections.Generic;
using System.Text;

namespace SharpMonkey
{
    public class Ast
    {
        public interface INode
        {
            string TokenLiteral();
            string ToPrintableString(); // for debug use
        }

        public interface IStatement : INode
        {
        }

        public interface IExpression : INode
        {
        }

        public class MonkeyProgram : INode
        {
            // 一个program将会有许多个statements组成
            public List<IStatement> Statements = new List<IStatement>();

            public string TokenLiteral()
            {
                if (Statements.Count > 0)
                {
                    return Statements[0].TokenLiteral();
                }
                else
                {
                    return "Empty Program";
                }
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                foreach (var s in Statements)
                {
                    outBuilder.AppendLine(s.ToPrintableString());
                }

                return outBuilder.ToString();
            }
        }


        public class LetStatement : IStatement
        {
            // Let <Name> = <Value> // Value is an expression
            // 为什么Name也要用Expression
            // 因为可以写出 let x = 5; let y = x;这种语句，如果x设置为statement,那么第二句就需要实现从statement到expression的类型转换
            public Token Token;
            public Identifier Name;
            public IExpression Value;

            public LetStatement(Token token)
            {
                Token = token;
            }

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                outBuilder.Append($"{TokenLiteral()} {Name.ToPrintableString()} = ");
                if (Value != null)
                {
                    outBuilder.Append($"{Value.ToPrintableString()}");
                }

                outBuilder.Append(';');
                return outBuilder.ToString();
            }
        }

        public class ReturnStatement : IStatement
        {
            public Token Token;
            public IExpression ReturnValue;

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public ReturnStatement(Token token)
            {
                Token = token;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                outBuilder.Append($"{TokenLiteral()} ");
                if (ReturnValue != null)
                {
                    outBuilder.Append($"{ReturnValue.ToPrintableString()}");
                }

                outBuilder.Append(';');
                return outBuilder.ToString();
            }
        }

        // 脚本语言中支持 x = 5; x + 10; 这种格式，第二句会返回15，它虽然是一个表达式，但是可以构成一个单独的语句(Python也支持类似的语法)
        public class ExpressionStatement : IStatement
        {
            public Token Token; // first token
            public IExpression Expression; // the whole expression

            public ExpressionStatement(Token token)
            {
                Token = token;
            }

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public string ToPrintableString()
            {
                if (Expression != null)
                {
                    return Expression.ToPrintableString();
                }

                return "";
            }
        }

        public class Identifier : IExpression
        {
            public Token Token;
            public string Value; // 这个value值应该等于Token.Literal

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public Identifier(Token token, string value)
            {
                Token = token;
                Value = value;
            }

            public string ToPrintableString()
            {
                return Value;
            }
        }
        
        public class IntegerLiteral : IExpression
        {
            public Token Token;
            public long Value;

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public IntegerLiteral(Token token, string value)
            {
                Token = token;
                Value = Int64.Parse(value);
            }

            public string ToPrintableString()
            {
                return TokenLiteral();
            }
        }
        
        public class PrefixExpression : IExpression
        {
            // 形如 !a; 其钟Token是!, Right是a, Operator是!
            public Token Token; 
            public string Operator;
            public IExpression Right;
            
            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public PrefixExpression(Token token, string op)
            {
                Token = token;
                Operator = op;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder  = new StringBuilder();
                outBuilder.Append('(');
                outBuilder.Append(Operator);
                outBuilder.Append(Right.ToPrintableString());
                outBuilder.Append(')');
                return outBuilder.ToString();
            }
        }
    }
}