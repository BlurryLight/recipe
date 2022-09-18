#nullable enable
using System.Collections.Generic;

namespace SharpMonkey
{
    public class Environment
    {
        private Dictionary<string, MonkeyObject> Bindings = null;

        public Environment()
        {
            Bindings = new Dictionary<string, MonkeyObject>();
        }

        public MonkeyObject? Get(string valName)
        {
            return Bindings.TryGetValue(valName, out var obj) ? obj : null;
        }

        public MonkeyObject Set(string valName, MonkeyObject val)
        {
            Bindings[valName] = val;
            return val;
        }
    }
}