using System;
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
                case MonkeyError:
                    return false;
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

    public static class Evaluator
    {
        /// <summary>
        /// 需要考虑两种情况。 一种是顶层语句，如REPL里直接输入的`return 5;`此类，返回值就是直接是5
        /// 第二类情况较为复杂
        /// 比如 if(1){ if(1} return 5;}
        /// 当执行到内层的return 5,如果返回一个MonkeyInteger,那么外层的Eval就不能知道内层碰见了一个return语句，会理解成 if(1){5;}这样的语句并继续向下执行。
        /// 内层return需要拥有中断外层语句向下执行的作用。
        /// 因此把这个return语句层层往上抛
        ///
        /// 另外一种可行的做法是在内层执行到return的时候直接抛异常，然后在Eval Ast.Program分支捕获这个异常，返回值通过异常机制拿到，这样直接跳出了内层的eval嵌套调用回到了顶层的Program
        /// 但是这个需要语言实现异常机制
        /// </summary>
        /// <param name="stmts"></param>
        /// <param name="TopLevel">是否是Program顶层的语句</param>
        /// <returns></returns>
        private static MonkeyObject EvalStatements(List<Ast.IStatement> stmts, bool TopLevel, Environment env)
        {
            MonkeyObject result = null;
            foreach (var statement in stmts)
            {
                result = Eval(statement, env);
                switch (result)
                {
                    case MonkeyReturnValue resultRef:
                        return TopLevel ? resultRef.ReturnObj : result;
                    case MonkeyError:
                        return result;
                }
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
                return new MonkeyError($"unsupported prefix: -{right.Type()}");
            }

            return new MonkeyInteger(-intObj.Value);
        }

        //  原地自增，自减
        private static MonkeyObject EvalSelfPrefixOpExpression(MonkeyObject right, bool increment)
        {
            if (right is not MonkeyInteger intObj)
            {
                return new MonkeyError($"unsupported prefix: {(increment ? "++" : "--")}{right.Type()}");
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
                    return new MonkeyError($"unsupported prefix: {op}{right.Type()}");
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
                    return new MonkeyError($"unsupported infix: {left.Type()} {op} {right.Type()}");
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
                    return new MonkeyError($"unsupported infix: {left.Type()} {op} {right.Type()}");
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

            if (left is MonkeyBoolean && right is MonkeyBoolean)
            {
                return EvalBoolInfixExpression(op, left, right);
            }

            if (left.Type() != right.Type())
            {
                return new MonkeyError($"type mismatch: {left.Type()} {op} {right.Type()}");
            }

            return new MonkeyError($"unsupported infix: {left.Type()} {op} {right.Type()}");
        }

        private static MonkeyObject EvalPostfixExpression(string op, ref MonkeyObject left)
        {
            if (left is not MonkeyInteger intObj)
            {
                return new MonkeyError($"unsupported postfix: {left.Type()}{op}");
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
                    return new MonkeyError($"unsupported postfix: {left.Type()}{op}");
            }

            return oldValue;
        }

        private static MonkeyObject EvalIfExpression(Ast.IfExpression exp, Environment env)
        {
            var condition = Eval(exp.Condition, env);
            if (condition is MonkeyError) return condition;
            if (EvaluatorHelper.IsTrueObject(condition))
            {
                return Eval(exp.Consequence, env);
            }

            if (exp.Alternative != null)
            {
                return Eval(exp.Alternative, env);
            }

            return MonkeyNull.NullObject;
        }

        private static MonkeyObject EvalIdentifier(Ast.Identifier exp, Environment env)
        {
            var value = env.Get(exp.Value);
            if (value is null)
            {
                return new MonkeyError($"identifier not found: {exp.Value}");
            }

            return value;
        }

        public static MonkeyObject Eval(Ast.INode node, Environment env)
        {
            switch (node) // switch on type
            {
                case Ast.MonkeyProgram program:
                    return EvalStatements(program.Statements, true, env);
                case Ast.ExpressionStatement stmt:
                    return Eval(stmt.Expression, env);
                case Ast.IntegerLiteral val:
                    return new MonkeyInteger(val.Value);
                case Ast.BooleanLiteral val:
                    // a small optimization. will never create new Boolean object
                    return val.Value ? MonkeyBoolean.TrueObject : MonkeyBoolean.FalseObject;
                case Ast.PrefixExpression exp:
                {
                    var right = Eval(exp.Right, env);
                    return EvalPrefixExpression(exp.Operator, right);
                }
                case Ast.InfixExpression exp:
                {
                    var left = Eval(exp.Left, env);
                    if (left is MonkeyError) return left;
                    MonkeyObject right = MonkeyNull.NullObject;
                    // 处理短路原则
                    if (exp.Operator == "&&")
                    {
                        // 如果 (a && b)中a不合法，则b不会被执行
                        if (!EvaluatorHelper.IsTrueObject(left))
                            return EvalInfixExpression(exp.Operator, left, MonkeyNull.NullObject);
                    }

                    right = Eval(exp.Right, env);
                    if (right is MonkeyError) return right;
                    return EvalInfixExpression(exp.Operator, left, right);
                }
                case Ast.ConditionalExpression exp:
                {
                    var condition = Eval(exp.Condition, env);
                    if (condition is MonkeyError) return condition;
                    return Eval(EvaluatorHelper.IsTrueObject(condition) ? exp.ThenArm : exp.ElseArm, env);
                }
                case Ast.PostfixExpression exp:
                {
                    var left = Eval(exp.Left, env);
                    if (left is MonkeyError) return left;
                    return EvalPostfixExpression(exp.Operator, ref left);
                }
                case Ast.BlockStatement exp:
                {
                    return EvalStatements(exp.Statements, false, env);
                }
                case Ast.IfExpression exp:
                {
                    return EvalIfExpression(exp, env);
                }
                case Ast.ReturnStatement stmt:
                {
                    MonkeyObject returnVal = MonkeyNull.NullObject;
                    if (stmt.ReturnValue != null)
                    {
                        returnVal = Eval(stmt.ReturnValue, env);
                        if (returnVal is MonkeyError) return returnVal;
                    }

                    return new MonkeyReturnValue(returnVal);
                }
                case Ast.LetStatement stmt:
                {
                    var val = Eval(stmt.Value, env);
                    if (val is MonkeyError) return val;
                    env.Set(stmt.Name.Value, val);
                    return null; // let x = 1; 没有返回值
                }
                case Ast.Identifier exp:
                {
                    return EvalIdentifier(exp, env);
                }
                case Ast.AssignExpression exp:
                {
                    var namedObj = EvalIdentifier(exp.Name, env);
                    if (namedObj is MonkeyError) return namedObj;
                    var val = Eval(exp.Value, env);
                    env.Set(exp.Name.Value, val);
                    return null; // x = 1;  assign语句没有返回值
                }
                case null:
                    return new MonkeyError("Invalid expressions appear during parsing.");
                default:
                    var msg = (
                        $"Eval Node '{node.GetType().FullName} {node.ToPrintableString()}' has not implemented yet.");
                    return new MonkeyError(msg);
            }
        }
    }
}