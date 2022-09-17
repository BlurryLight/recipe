using System;
using System.Diagnostics;

namespace SharpMonkey
{
    using ObjectType = System.String;

    public static class ObjType
    {
        public const string IntegerObj = "Integer";
        public const string BooleanObj = "Boolean";
        public const string NullObj = "Null";
        public const string ReturnObj = "ReturnValue";
    }

    public interface MonkeyObject
    {
        ObjectType Type();
        string Inspect(); // for debug
    }

    public class MonkeyInteger : MonkeyObject
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
    }

    public class MonkeyBoolean : MonkeyObject
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

        public static MonkeyBoolean ImplicitConvertFrom(MonkeyObject obj)
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

    public class MonkeyNull : MonkeyObject
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

    public class MonkeyReturnValue : MonkeyObject
    {
        public MonkeyObject ReturnObj;

        public MonkeyReturnValue(MonkeyObject returnObj)
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
}