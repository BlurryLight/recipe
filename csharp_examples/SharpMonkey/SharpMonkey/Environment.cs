#nullable enable
using System.Collections.Generic;

namespace SharpMonkey
{
    // 代表作用域，有一个Outer变量指向外层的作用域
    public class Environment
    {
        private Dictionary<string, IMonkeyObject> Bindings = null;
        public Environment? Outer;

        public Environment()
        {
            Bindings = new Dictionary<string, IMonkeyObject>();
            Outer = null;
        }

        public Environment(Environment outer)
        {
            Bindings = new Dictionary<string, IMonkeyObject>();
            Outer = outer;
        }

        public IMonkeyObject? Get(string valName)
        {
            var value = Bindings.TryGetValue(valName, out var obj) ? obj : null;
            if (value == null && Outer != null)
            {
                value = Outer.Get(valName);
            }

            return value;
        }

        public IMonkeyObject Set(string valName, IMonkeyObject val)
        {
            Bindings[valName] = val;
            return val;
        }
    }
}