﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
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
                StringBuilder outBuilder = new StringBuilder();
                outBuilder.Append('(');
                outBuilder.Append(Operator);
                outBuilder.Append(Right.ToPrintableString());
                outBuilder.Append(')');
                return outBuilder.ToString();
            }
        }

        /// <summary>
        /// expression like "a++"
        /// </summary>
        public class PostfixExpression : IExpression
        {
            public Token Token;
            public IExpression Left;
            public string Operator;

            public PostfixExpression(Token token, string op, IExpression left)
            {
                Token = token;
                Operator = op;
                Left = left;
            }

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                outBuilder.Append('(');
                outBuilder.Append(Left.ToPrintableString());
                outBuilder.Append(Operator);
                outBuilder.Append(')');
                return outBuilder.ToString();
            }
        }

        /// <summary>
        /// expression like "<condition> ? <then> : <else> "
        /// </summary>
        public class ConditionalExpression : IExpression
        {
            public Token Token;
            public IExpression Condition;
            public IExpression ThenArm;
            public IExpression ElseArm;

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public ConditionalExpression(Token token, IExpression condition)
            {
                Token = token;
                Condition = condition;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                outBuilder.Append('(');
                outBuilder.Append(Condition.ToPrintableString());
                outBuilder.Append(" ? ");
                outBuilder.Append(ThenArm.ToPrintableString());
                outBuilder.Append(" : ");
                outBuilder.Append(ElseArm.ToPrintableString());
                outBuilder.Append(')');
                return outBuilder.ToString();
            }
        }

        public class InfixExpression : IExpression
        {
            // <left> <operator> <right>
            public Token Token;
            public IExpression Left;
            public string Operator;
            public IExpression Right;

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public InfixExpression(Token token, string op, Ast.IExpression left)
            {
                Token = token;
                Operator = op;
                Left = left;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                outBuilder.Append('(');
                outBuilder.Append(Left.ToPrintableString());
                outBuilder.Append(" " + Operator + " ");
                outBuilder.Append(Right.ToPrintableString());
                outBuilder.Append(')');
                return outBuilder.ToString();
            }
        }

        public class StringLiteral : IExpression
        {
            public Token Token;
            public string Value;

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public string ToPrintableString()
            {
                return $"\"{TokenLiteral()}\"";
            }

            public StringLiteral(Token token, string value)
            {
                Token = token;
                Value = value;
            }
        }

        public class BooleanLiteral : IExpression
        {
            public Token Token;
            public bool Value;

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public BooleanLiteral(Token token, string value)
            {
                Token = token;
                Debug.Assert(value is "true" or "false");
                Value = (value == "true");
            }

            public string ToPrintableString()
            {
                return TokenLiteral();
            }
        }

        public class BlockStatement : IStatement
        {
            public Token Token; // {
            public List<IStatement> Statements; // 由一系列的ExpressionStatement和 Let/Return语句组成

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public BlockStatement(Token token)
            {
                Token = token;
                Statements = new List<IStatement>();
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                foreach (var stmt in Statements)
                {
                    outBuilder.Append(stmt.ToPrintableString());
                }

                return outBuilder.ToString();
            }
        }

        public class WhileExpression : IExpression
        {
            // while <condition> { BlockBody }
            public Token Token; // while
            public IExpression Condition;
            public BlockStatement Body;

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public WhileExpression(Token token)
            {
                Token = token;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                outBuilder.Append($"while( {Condition.ToPrintableString()} ) {{ {Body.ToPrintableString()} }} ");
                return outBuilder.ToString();
            }
        }

        public class IfExpression : IExpression
        {
            // if <condition> { <Consequence> } else {<alter>}
            public Token Token; // if
            public IExpression Condition;
            public BlockStatement Consequence;
            public BlockStatement Alternative; // nullable

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public IfExpression(Token token)
            {
                Token = token;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                outBuilder.Append($"if( {Condition.ToPrintableString()} ) {{ {Consequence.ToPrintableString()} }} ");
                if (Alternative != null)
                {
                    outBuilder.Append($"else {{ {Alternative.ToPrintableString()} }}");
                }

                return outBuilder.ToString();
            }
        }

        // 函数声明
        // fn (args...) { statements }
        public class FunctionLiteral : IExpression
        {
            public Token Token; // fn
            public List<Identifier> Parameters;
            public BlockStatement FuncBody;

            public string TokenLiteral()
            {
                return Token.Literal;
            }

            public FunctionLiteral(Token token)
            {
                Token = token;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                List<string> paramStrs = new();
                foreach (var p in Parameters)
                {
                    paramStrs.Add(p.ToPrintableString());
                }

                outBuilder.Append(TokenLiteral());
                outBuilder.Append('(');
                outBuilder.Append(string.Join(", ", paramStrs));
                outBuilder.Append(") ");
                outBuilder.Append(FuncBody.ToPrintableString());
                return outBuilder.ToString();
            }
        }

        // 函数调用
        // 两种形式
        // let func = fn(...){...}; func(); 
        // fn(...){...}();
        // 括号的左边可能是一个Identifier,也可能是一个FuncLiteral
        public class CallExpression : IExpression
        {
            public Token Token; // 以(作为词法单元
            public IExpression Function;
            public List<IExpression> Arguments; // args可以是一个表达式  add(1+2,foo())

            public CallExpression(Token token, IExpression function)
            {
                Token = token;
                Debug.Assert(function != null);
                Function = function;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                List<string> argStrs = new();
                foreach (var p in Arguments)
                {
                    argStrs.Add(p.ToPrintableString());
                }

                outBuilder.Append(Function.ToPrintableString());
                outBuilder.Append('(');
                outBuilder.Append(string.Join(", ", argStrs));
                outBuilder.Append(')');
                return outBuilder.ToString();
            }

            public string TokenLiteral()
            {
                return Token.Literal;
            }
        }

        // <ident> = <expression>
        public class AssignExpression : IExpression
        {
            public Token Token; // 以=作为词法单元
            public Identifier Name; // <ident>部分
            public IExpression Value; // <exp>部分

            public AssignExpression(Token token, Identifier name)
            {
                Token = token;
                Debug.Assert(name != null);
                Name = name;
            }

            public string ToPrintableString()
            {
                StringBuilder outBuilder = new StringBuilder();
                outBuilder.Append($"{Name.ToPrintableString()} = {Value.ToPrintableString()}");
                return outBuilder.ToString();
            }

            public string TokenLiteral()
            {
                return Token.Literal;
            }
        }
    }
}