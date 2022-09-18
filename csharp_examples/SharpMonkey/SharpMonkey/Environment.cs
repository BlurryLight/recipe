#nullable enable
using System.Collections.Generic;

namespace SharpMonkey
{
    // 代表作用域，有一个Outer变量指向外层的作用域
    public class Environment
    {
        private Dictionary<string, MonkeyObject> Bindings = null;
        public Environment? Outer;
        public Environment()
        {
            Bindings = new Dictionary<string, MonkeyObject>();
            Outer = null;
        }
        public Environment(Environment outer)
        {
            Bindings = new Dictionary<string, MonkeyObject>();
            Outer = outer;
        }

        public MonkeyObject? Get(string valName)
        {
            var value =  Bindings.TryGetValue(valName, out var obj) ? obj : null;
            if (value == null && Outer != null)
            {
                value = Outer.Get(valName);
            }

            return value;
        }

        public MonkeyObject Set(string valName, MonkeyObject val)
        {
            Bindings[valName] = val;
            return val;
        }
    }
}