using System;
using System.Collections.Generic;

namespace SharpMonkey
{
    public class Evaluator
    {
        public static MonkeyObject EvalStatements(List<Ast.IStatement> stmts)
        {
            MonkeyObject result = null;
            foreach (var statement in stmts)
            {
                result = Eval(statement);
            }

            return result;
        }

        public static MonkeyObject EvalBangOperatorExpression(MonkeyObject right)
        {
            // 如果传入的是null,表达式是(!null),返回true
            switch (right)
            {
                case MonkeyBoolean obj:
                    return (obj == MonkeyBoolean.TrueObject) ? MonkeyBoolean.FalseObject : MonkeyBoolean.TrueObject;
                case MonkeyNull obj:
                    return MonkeyBoolean.TrueObject;
                // 在这里处理不是很优雅，可能定义一个MonkeyInteger到MonkeyBool的类型转换比较好
                case MonkeyInteger obj:
                    return obj.Value == 0 ? MonkeyBoolean.TrueObject : MonkeyBoolean.FalseObject;
                default:
                    return MonkeyBoolean.FalseObject;
            }
        }

        public static MonkeyObject EvalMinusPrefixOpExpression(MonkeyObject right)
        {
            if (right is not MonkeyInteger intObj)
            {
                Console.WriteLine($"Invalid (-{right.Inspect()}) Expression");
                return MonkeyNull.NullObject;
            }

            return new MonkeyInteger(-intObj.Value);
        }

        public static MonkeyObject EvalPrefixExpression(string op, MonkeyObject right)
        {
            switch (op)
            {
                case "!":
                    return EvalBangOperatorExpression(right);
                case "-":
                    return EvalMinusPrefixOpExpression(right);
                default:
                    Console.WriteLine($"Prefix {op} has not implemented yet.");
                    return MonkeyNull.NullObject;
            }
        }

        public static MonkeyObject EvalIntegerInfixExpression(string op, MonkeyObject left, MonkeyObject right)
        {
            var leftInt = (MonkeyInteger) left;
            var rightInt = (MonkeyInteger) right;
            switch (op)
            {
                case "+":
                    return new MonkeyInteger(leftInt.Value + rightInt.Value);
                case "-":
                    return new MonkeyInteger(leftInt.Value - rightInt.Value);
                case "*":
                    return new MonkeyInteger(leftInt.Value * rightInt.Value);
                case "/":
                    return new MonkeyInteger(leftInt.Value / rightInt.Value);
                case "<":
                    return MonkeyBoolean.GetStaticObject(leftInt.Value < rightInt.Value);
                case ">":
                    return MonkeyBoolean.GetStaticObject(leftInt.Value > rightInt.Value);
                case "==":
                    return MonkeyBoolean.GetStaticObject(leftInt.Value == rightInt.Value);
                case "!=":
                    return MonkeyBoolean.GetStaticObject(leftInt.Value != rightInt.Value);
                default:
                    Console.WriteLine($"Integer Infix {op} has not implemented yet.");
                    return MonkeyNull.NullObject;
            }
        }

        public static MonkeyObject EvalBoolInfixExpression(string op, MonkeyObject left, MonkeyObject right)
        {
            switch (op)
            {
                case "==":
                    return MonkeyBoolean.GetStaticObject(left == right);
                case "!=":
                    return MonkeyBoolean.GetStaticObject(left != right);
                default:
                    Console.WriteLine($"Boolean Infix {op} has not implemented yet.");
                    return MonkeyNull.NullObject;
            }
        }
        public static MonkeyObject EvalInfixExpression(string op, MonkeyObject left, MonkeyObject right)
        {
            switch (left)
            {
                case MonkeyInteger when right is MonkeyInteger:
                    return EvalIntegerInfixExpression(op, left, right);
                case MonkeyBoolean when right is MonkeyBoolean:
                    return EvalBoolInfixExpression(op, left, right);
                default:
                    Console.WriteLine($"infix {left.Inspect()} {op} {right.Inspect()} has not implemented yet");
                    return MonkeyNull.NullObject;
            }
        }

        public static MonkeyObject Eval(Ast.INode node)
        {
            switch (node) // switch on type
            {
                case Ast.MonkeyProgram program:
                    return EvalStatements(program.Statements);
                case Ast.ExpressionStatement stmt:
                    return Eval(stmt.Expression);
                case Ast.IntegerLiteral val:
                    return new MonkeyInteger(val.Value);
                case Ast.BooleanLiteral val:
                    // a small optimization. will never create new Boolean object
                    return val.Value ? MonkeyBoolean.TrueObject : MonkeyBoolean.FalseObject;
                case Ast.PrefixExpression exp:
                {
                    var right = Eval(exp.Right);
                    return EvalPrefixExpression(exp.Operator, right);
                }
                case Ast.InfixExpression exp:
                {
                    var left = Eval(exp.Left);
                    var right = Eval(exp.Right);
                    return EvalInfixExpression(exp.Operator, left, right);
                }
                default:
                    Console.WriteLine(
                        $"Eval Node {node.GetType().FullName} {node.ToPrintableString()} has not implemented yet.");
                    return null;
            }
        }
    }
}