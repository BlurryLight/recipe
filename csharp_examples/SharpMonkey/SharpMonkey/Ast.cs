using System.Collections.Generic;

namespace SharpMonkey
{
    public class Ast
    {
        public interface INode
        {
            string TokenLiteral();
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
                    return "";
                }
            }
        }

        public class Identifier : IExpression
        {
            public Token Token;
            public string Value;
            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public Identifier(Token token, string value)
            {
                Token = token;
                Value = value;
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
        }
    }
}