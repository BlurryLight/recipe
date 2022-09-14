using System;

namespace SharpMonkey
{
    using ObjectType = System.String;

    public static class ObjType
    {
        public const string IntegerObj = "Integer";
        public const string BooleanObj = "Boolean";
        public const string NullObj = "Null";
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

        public static MonkeyBoolean FalseObject { get; } = new MonkeyBoolean(false);
        public static MonkeyBoolean TrueObject { get; } = new MonkeyBoolean(true);
    }
    
    public class MonkeyNull : MonkeyObject
    {
        public string Type()
        {
            return ObjType.NullObj;
        }

        public string Inspect()
        {
            return "Null";
        }
    }

}
