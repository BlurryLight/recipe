﻿using System;
using System.Collections.Generic;

namespace SharpMonkey
{
    public class EvaluatorHelper
    {
        /// <summary>
        /// 当obj等于NullObject或者FalseObject的时候返回false，否则true
        /// </summary>
        /// <param name="monkeyObject"></param>
        /// <returns></returns>
        public static bool IsTrueObject(MonkeyObject monkeyObject)
        {
            switch (monkeyObject)
            {
                case MonkeyNull _:
                    return false;
                case MonkeyBoolean obj:
                    return obj == MonkeyBoolean.TrueObject;
                case MonkeyInteger obj:
                    return obj.Value != 0;
                default:
                    return true;
            }
        }
        
        public static bool IsNullObject(MonkeyObject monkeyObject)
        {
            if (monkeyObject is not MonkeyNull) return false;
            return monkeyObject == MonkeyNull.NullObject;
        }
    }

    public class Evaluator
    {
        private static MonkeyObject EvalStatements(List<Ast.IStatement> stmts)
        {
            MonkeyObject result = null;
            foreach (var statement in stmts)
            {
                result = Eval(statement);
            }

            return result;
        }

        private static MonkeyObject EvalBangOperatorExpression(MonkeyObject right)
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

        private static MonkeyObject EvalMinusPrefixOpExpression(MonkeyObject right)
        {
            if (right is not MonkeyInteger intObj)
            {
                Console.WriteLine($"Invalid (-{right.Inspect()}) Expression");
                return MonkeyNull.NullObject;
            }

            return new MonkeyInteger(-intObj.Value);
        }

        //  原地自增，自减
        private static MonkeyObject EvalSelfPrefixOpExpression(MonkeyObject right, bool increment)
        {
            if (right is not MonkeyInteger intObj)
            {
                Console.WriteLine($"Invalid (-{right.Inspect()}) Expression");
                return MonkeyNull.NullObject;
            }

            intObj.Value += (increment ? 1 : -1);
            return intObj;
        }

        private static MonkeyObject EvalPrefixExpression(string op, MonkeyObject right)
        {
            switch (op)
            {
                case "!":
                    return EvalBangOperatorExpression(right);
                case "-":
                    return EvalMinusPrefixOpExpression(right);
                case "++":
                    return EvalSelfPrefixOpExpression(right, true);
                case "--":
                    return EvalSelfPrefixOpExpression(right, false);
                default:
                    Console.WriteLine($"Prefix {op} has not implemented yet.");
                    return MonkeyNull.NullObject;
            }
        }

        private static MonkeyObject EvalIntegerInfixExpression(string op, MonkeyObject left, MonkeyObject right)
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

        private static MonkeyObject EvalBoolInfixExpression(string op, MonkeyObject left, MonkeyObject right)
        {
            var leftBoolean = MonkeyBoolean.ImplicitConvertFrom(left);
            var rightBoolean = MonkeyBoolean.ImplicitConvertFrom(right);
            switch (op)
            {
                case "==":
                    return MonkeyBoolean.GetStaticObject(leftBoolean == rightBoolean);
                case "!=":
                    return MonkeyBoolean.GetStaticObject(leftBoolean != rightBoolean);
                case "&&":
                    return MonkeyBoolean.GetStaticObject(leftBoolean.Value && rightBoolean.Value);
                case "||":
                    return MonkeyBoolean.GetStaticObject(leftBoolean.Value || rightBoolean.Value);
                default:
                    Console.WriteLine($"Boolean Infix {op} has not implemented yet.");
                    return MonkeyNull.NullObject;
            }
        }

        private static MonkeyObject EvalInfixExpression(string op, MonkeyObject left, MonkeyObject right)
        {
            if (op is "&&" or "||")
            {
                return EvalBoolInfixExpression(op, left, right);
            }
            if (left is MonkeyInteger && right is MonkeyInteger)
            {
                return EvalIntegerInfixExpression(op, left, right);
            }
            if (left is MonkeyBoolean || right is MonkeyBoolean)
            {
                return EvalBoolInfixExpression(op, left, right);
            }
            Console.WriteLine($"infix {left.Inspect()} {op} {right.Inspect()} has not implemented yet");
            return MonkeyNull.NullObject;
        }

        private static MonkeyObject EvalPostfixExpression(string op, ref MonkeyObject left)
        {
            if (left is not MonkeyInteger intObj)
            {
                Console.WriteLine($"Invalid ({left.Inspect()}{op}) PostFix Expression");
                return MonkeyNull.NullObject;
            }

            var oldValue = new MonkeyInteger(intObj.Value);
            switch (op)
            {
                case "++":
                    intObj.Value++;
                    break;
                case "--":
                    intObj.Value--;
                    break;
                default:
                    Console.WriteLine($"Integer Postfix {op} has not implemented yet.");
                    break;
            }

            return oldValue;
        }

        private static MonkeyObject EvalIfExpression(Ast.IfExpression exp)
        {
            var condition = Eval(exp.Condition);
            if (EvaluatorHelper.IsTrueObject(condition))
            {
                return Eval(exp.Consequence);
            }

            if (exp.Alternative != null)
            {
                return Eval(exp.Alternative);
            }

            return MonkeyNull.NullObject;
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
                    MonkeyObject right = null;
                    // 处理短路原则
                    if (exp.Operator == "&&")
                    {
                        // 如果 (a && b)中a不合法，则b不会被执行
                        if (!EvaluatorHelper.IsTrueObject(left))
                            return EvalInfixExpression(exp.Operator, left, MonkeyNull.NullObject);
                        right = Eval(exp.Right);
                        return EvalInfixExpression(exp.Operator, left, right);
                    }
                    right = Eval(exp.Right);
                    return EvalInfixExpression(exp.Operator, left, right);
                }
                case Ast.ConditionalExpression exp:
                {
                    return Eval(Eval(exp.Condition) == MonkeyBoolean.TrueObject ? exp.ThenArm : exp.ElseArm);
                }
                case Ast.PostfixExpression exp:
                {
                    var left = Eval(exp.Left);
                    return EvalPostfixExpression(exp.Operator, ref left);
                }
                case Ast.BlockStatement exp:
                {
                    return EvalStatements(exp.Statements);
                }
                case Ast.IfExpression exp:
                {
                    return EvalIfExpression(exp);
                }
                default:
                    Console.WriteLine(
                        $"Eval Node '{node.GetType().FullName} {node.ToPrintableString()}' has not implemented yet.");
                    return MonkeyNull.NullObject;
            }
        }
    }
}