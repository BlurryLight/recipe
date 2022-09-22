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
        public static bool IsTrueObject(IMonkeyObject monkeyObject)
        {
            switch (monkeyObject)
            {
                case MonkeyNull _:
                    return false;
                case MonkeyBoolean obj:
                    return obj == MonkeyBoolean.TrueObject;
                case MonkeyInteger obj:
                    return obj.Value != 0;
                case MonkeyDouble obj:
                    return obj.Value != 0;
                case MonkeyError:
                    return false;
                default:
                    return true;
            }
        }

        public static bool IsNullObject(IMonkeyObject monkeyObject)
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
        private static IMonkeyObject EvalStatements(List<Ast.IStatement> stmts, bool TopLevel, Environment env)
        {
            IMonkeyObject result = null;
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

        private static IMonkeyObject EvalBangOperatorExpression(IMonkeyObject right)
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

        private static IMonkeyObject EvalMinusPrefixOpExpression(IMonkeyObject right)
        {
            return right switch
            {
                MonkeyInteger intObj => new MonkeyInteger(-intObj.Value),
                MonkeyDouble doubleObj => new MonkeyDouble(-doubleObj.Value),
                _ => new MonkeyError($"unsupported prefix: -{right.Type()}")
            };
        }

        //  原地自增，自减
        private static IMonkeyObject EvalSelfPrefixOpExpression(IMonkeyObject right, bool increment)
        {
            switch (right)
            {
                case MonkeyInteger intObj:
                    intObj.Value += (increment ? 1 : -1);
                    return intObj;
                case MonkeyDouble doubleObj:
                    doubleObj.Value += (increment ? 1.0 : -1.0);
                    return doubleObj;
                default:
                    return new MonkeyError($"unsupported prefix: {(increment ? "++" : "--")}{right.Type()}");
            }
        }

        private static IMonkeyObject EvalPrefixExpression(string op, IMonkeyObject right)
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

        private static IMonkeyObject EvalDoubleInfixExpression(string op, IMonkeyObject left, IMonkeyObject right)
        {
            var leftDouble = left is MonkeyInteger leftInt? leftInt.ToMonkeyDouble() : (MonkeyDouble)left ;
            var rightDouble = right is MonkeyInteger rightInt? rightInt.ToMonkeyDouble() : (MonkeyDouble)right ;
            return op switch
            {
                "+" => new MonkeyDouble(leftDouble.Value + rightDouble.Value),
                "-" => new MonkeyDouble(leftDouble.Value - rightDouble.Value),
                "*" => new MonkeyDouble(leftDouble.Value * rightDouble.Value),
                "/" => new MonkeyDouble(leftDouble.Value / rightDouble.Value),
                "<" => MonkeyBoolean.GetStaticObject(leftDouble.Value < rightDouble.Value),
                ">" => MonkeyBoolean.GetStaticObject(leftDouble.Value > rightDouble.Value),
                // ReSharper disable once CompareOfFloatsByEqualityOperator
                "==" => MonkeyBoolean.GetStaticObject(leftDouble.Value == rightDouble.Value),
                // ReSharper disable once CompareOfFloatsByEqualityOperator
                "!=" => MonkeyBoolean.GetStaticObject(leftDouble.Value != rightDouble.Value),
                _ => new MonkeyError($"unsupported infix: {left.Type()} {op} {right.Type()}")
            };
        }
        private static IMonkeyObject EvalIntegerInfixExpression(string op, IMonkeyObject left, IMonkeyObject right)
        {
            var leftInt = (MonkeyInteger) left;
            var rightInt = (MonkeyInteger) right;
            return op switch
            {
                "+" => new MonkeyInteger(leftInt.Value + rightInt.Value),
                "-" => new MonkeyInteger(leftInt.Value - rightInt.Value),
                "*" => new MonkeyInteger(leftInt.Value * rightInt.Value),
                "/" => new MonkeyInteger(leftInt.Value / rightInt.Value),
                "<" => MonkeyBoolean.GetStaticObject(leftInt.Value < rightInt.Value),
                ">" => MonkeyBoolean.GetStaticObject(leftInt.Value > rightInt.Value),
                "==" => MonkeyBoolean.GetStaticObject(leftInt.Value == rightInt.Value),
                "!=" => MonkeyBoolean.GetStaticObject(leftInt.Value != rightInt.Value),
                _ => new MonkeyError($"unsupported infix: {left.Type()} {op} {right.Type()}")
            };
        }

        private static IMonkeyObject EvalStringInfixExpression(string op, IMonkeyObject left, IMonkeyObject right)
        {
            var leftStr = (MonkeyString) left;
            var rightStr = (MonkeyString) right;
            return op switch
            {
                "+" => new MonkeyString(leftStr.Value + rightStr.Value),
                _ => new MonkeyError($"unsupported infix: {left.Type()} {op} {right.Type()}")
            };

        }

        private static IMonkeyObject EvalBoolInfixExpression(string op, IMonkeyObject left, IMonkeyObject right)
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

        private static IMonkeyObject EvalInfixExpression(string op, IMonkeyObject left, IMonkeyObject right)
        {
            if (op is "&&" or "||")
            {
                return EvalBoolInfixExpression(op, left, right);
            }
            
            switch (left)
            {
                case MonkeyInteger when right is MonkeyInteger:
                    return EvalIntegerInfixExpression(op, left, right);
                case MonkeyBoolean when right is MonkeyBoolean:
                    return EvalBoolInfixExpression(op, left, right);
                case MonkeyString when right is MonkeyString:
                    return EvalStringInfixExpression(op, left, right);
                case MonkeyDouble when right is MonkeyInteger:
                case MonkeyInteger when right is MonkeyDouble:
                case MonkeyDouble when right is MonkeyDouble:
                    return EvalDoubleInfixExpression(op, left, right);
            }


            if (left.Type() != right.Type())
            {
                return new MonkeyError($"type mismatch: {left.Type()} {op} {right.Type()}");
            }

            return new MonkeyError($"unsupported infix: {left.Type()} {op} {right.Type()}");
        }

        private static IMonkeyObject EvalPostfixExpression(string op, ref IMonkeyObject left)
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

        private static IMonkeyObject EvalWhileExpression(Ast.WhileExpression exp, Environment env)
        {
            IMonkeyObject result = null;
            while (true)
            {
                var condition = Eval(exp.Condition, env);
                if (condition is MonkeyError) return condition;
                if (!EvaluatorHelper.IsTrueObject(condition)) break;
                result = Eval(exp.Body, env);
                if (result is MonkeyReturnValue) //  如果循环体内部碰见了return语句，跳出循环
                {
                    return result;
                }
            }

            return null;
        }

        private static IMonkeyObject EvalIfExpression(Ast.IfExpression exp, Environment env)
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

        private static IMonkeyObject EvalIdentifier(Ast.Identifier exp, Environment env)
        {
            var value = env.Get(exp.Value);
            if (value is null)
            {
                return new MonkeyError($"identifier not found: {exp.Value}");
            }

            return value;
        }

        /// <summary>
        /// 对参数列表的所有表达式求值
        /// </summary>
        /// <param name="exps"></param>
        /// <param name="env"></param>
        /// <returns>如果过程有错误，返回一个Error,否则返回所有求值后的参数</returns>
        private static List<IMonkeyObject> EvalExpressions(List<Ast.IExpression> exps, Environment env)
        {
            List<IMonkeyObject> objs = new List<IMonkeyObject>();
            foreach (var exp in exps)
            {
                var evaluated = Eval(exp, env);
                if (evaluated is MonkeyError)
                {
                    objs.Clear();
                    objs.Add(evaluated);
                    return objs;
                }

                objs.Add(evaluated);
            }

            return objs;
        }

        public static IMonkeyObject Eval(Ast.INode node, Environment env)
        {
            switch (node) // switch on type
            {
                case Ast.MonkeyProgram program:
                    return EvalStatements(program.Statements, true, env);
                case Ast.ExpressionStatement stmt:
                    return Eval(stmt.Expression, env);
                case Ast.IntegerLiteral val:
                    return new MonkeyInteger(val.Value);
                case Ast.DoubleLiteral val:
                    return new MonkeyDouble(val.Value);
                case Ast.StringLiteral val:
                    return new MonkeyString(val.Value);
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
                    IMonkeyObject right = MonkeyNull.NullObject;
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
                    IMonkeyObject returnVal = MonkeyNull.NullObject;
                    if (stmt.ReturnValue != null)
                    {
                        returnVal = Eval(stmt.ReturnValue, env);
                        if (returnVal is MonkeyError) return returnVal;
                    }

                    return new MonkeyReturnValue(returnVal);
                }
                case Ast.WhileExpression exp:
                {
                    return EvalWhileExpression(exp, env);
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
                case Ast.FunctionLiteral exp:
                {
                    var funcObj = new MonkeyFuncLiteral(exp.Parameters, exp.FuncBody, env);
                    return funcObj;
                }
                case Ast.CallExpression exp:
                {
                    var evalObj = Eval(exp.Function, env); // must be MonkeyFuncLiteral
                    if (evalObj is MonkeyError) return evalObj;
                    var funcObj = (MonkeyFuncLiteral) evalObj;
                    var args = EvalExpressions(exp.Arguments, env);
                    if (args.Count == 1 && args[0] is MonkeyError) return args[0];
                    return ApplyFunction(funcObj, args);
                }
                case null:
                    return new MonkeyError("Invalid expressions appear during parsing.");
                default:
                    var msg = (
                        $"Eval Node '{node.GetType().FullName} {node.ToPrintableString()}' has not implemented yet.");
                    return new MonkeyError(msg);
            }
        }

        private static IMonkeyObject ApplyFunction(MonkeyFuncLiteral funcObj, List<IMonkeyObject> args)
        {
            var funcEnv = new Environment(funcObj.Env);
            for (int i = 0; i < funcObj.Params.Count; i++)
            {
                // 把每个param 和 arg在当前作用域下对应起来
                funcEnv.Set(funcObj.Params[i].Value, args[i]);
            }

            var evaluated = Eval(funcObj.Body, funcEnv);
            //如果在内层碰见了return语句，return的结果被包裹在return语句内，把return的结果解包
            if (evaluated is MonkeyReturnValue returnValue)
            {
                return returnValue.ReturnObj;
            }

            //如果是个表达式，比如 func(){5;}()这样的，那么直接取得结果就可以了
            return evaluated;
        }
    }
}