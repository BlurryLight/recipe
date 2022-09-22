using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;

namespace SharpMonkey
{
    using ObjectType = System.String;

    public static class ObjType
    {
        public const string IntegerObj = "Integer";
        public const string BooleanObj = "Boolean";
        public const string NullObj = "Null";
        public const string ReturnObj = "ReturnValue";
        public const string ErrorObj = "Error";
        public const string FuncObj = "Function";
        public const string StringObj = "String";
    }

    public interface IMonkeyObject
    {
        ObjectType Type();
        string Inspect(); // for debug
    }

    public class MonkeyString : IMonkeyObject
    {
        public string Value;

        public MonkeyString(string value)
        {
            Value = value;
        }

        public string Type()
        {
            return ObjType.StringObj;
        }

        public string Inspect()
        {
            return $"\"{Value}\"";
        }
    }
    
    public class MonkeyDouble : IMonkeyObject
    {
        public double Value;

        public MonkeyDouble(double val)
        {
            Value = val;
        }

        public string Type()
        {
            return ObjType.IntegerObj;
        }

        public string Inspect()
        {
            return Value.ToString();
        }

        // 继承于同一个类的对象似乎没法定义显式/隐式转换 
        public MonkeyInteger ToMonkeyInteger() => new MonkeyInteger((long)Value);
    }

    public class MonkeyInteger : IMonkeyObject
    {
        public Int64 Value;

        public MonkeyInteger(Int64 val)
        {
            Value = val;
        }

        public string Type()
        {
            return ObjType.IntegerObj;
        }

        public string Inspect()
        {
            return Value.ToString();
        }
        public MonkeyDouble ToMonkeyDouble() => new MonkeyDouble(Value);
    }

    public class MonkeyBoolean : IMonkeyObject
    {
        public bool Value;

        private MonkeyBoolean(bool val)
        {
            Value = val;
        }

        public string Type()
        {
            return ObjType.BooleanObj;
        }

        public string Inspect()
        {
            return Value.ToString();
        }

        public static MonkeyBoolean ImplicitConvertFrom(IMonkeyObject obj)
        {
            switch (obj)
            {
                case MonkeyNull _:
                    return FalseObject;
                case MonkeyBoolean booleanObj:
                    return booleanObj;
                default:
                    return TrueObject;
            }
        }

        public static MonkeyBoolean FalseObject { get; } = new MonkeyBoolean(false);
        public static MonkeyBoolean TrueObject { get; } = new MonkeyBoolean(true);

        public static MonkeyBoolean GetStaticObject(bool condition)
        {
            return condition ? TrueObject : FalseObject;
        }
    }

    public class MonkeyNull : IMonkeyObject
    {
        private MonkeyNull()
        {
        }

        public string Type()
        {
            return ObjType.NullObj;
        }

        public string Inspect()
        {
            return "Null";
        }

        public static MonkeyNull NullObject { get; } = new MonkeyNull();
    }

    public class MonkeyReturnValue : IMonkeyObject
    {
        public IMonkeyObject ReturnObj;

        public MonkeyReturnValue(IMonkeyObject returnObj)
        {
            ReturnObj = returnObj;
        }

        public string Type()
        {
            return ObjType.ReturnObj;
        }

        public string Inspect()
        {
            Debug.Assert(ReturnObj != null);
            return $"Return {ReturnObj.Inspect()};";
        }
    }

    public class MonkeyError : IMonkeyObject
    {
        public string Msg;

        public MonkeyError(string msg)
        {
            Msg = msg;
        }

        public string Type()
        {
            return ObjType.ErrorObj;
        }

        public string Inspect()
        {
            return $"Error: {Msg};";
        }
    }

    public class MonkeyFuncLiteral : IMonkeyObject
    {
        public List<Ast.Identifier> Params;
        public Ast.BlockStatement Body;
        public Environment Env;

        public MonkeyFuncLiteral(List<Ast.Identifier> parameters, Ast.BlockStatement body, Environment env)
        {
            Params = parameters;
            Body = body;
            Env = env;
        }

        public string Type()
        {
            return ObjType.FuncObj;
        }

        public string Inspect()
        {
            StringBuilder outBuilder = new StringBuilder();
            List<string> paramStrs = new();
            foreach (var p in Params)
            {
                paramStrs.Add(p.ToPrintableString());
            }

            outBuilder.Append($"fn({string.Join(", ", paramStrs)}) {{ {Body.ToPrintableString()} }}");
            return outBuilder.ToString();
        }
    }
}