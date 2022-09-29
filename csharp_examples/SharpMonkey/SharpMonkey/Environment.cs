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

        /// <summary>
        /// 在当前环境存储一个变量， let x = 1;
        /// </summary>
        /// <param name="valName"></param>
        /// <param name="val"></param>
        /// <returns></returns>
        public IMonkeyObject Set(string valName, IMonkeyObject val)
        {
            Bindings[valName] = val;
            return val;
        }

        /// <summary>
        /// 设置一个之前已经绑定过的变量
        /// </summary>
        /// <param name="valName"></param>
        /// <param name="val"></param>
        /// <returns>如果设置的变量没有被绑定过，则抛出错误</returns>
        public IMonkeyObject TrySetBoundedVar(string valName, IMonkeyObject val)
        {
            if (Bindings.ContainsKey(valName))
            {
                Bindings[valName] = val;
                return val;
            }

            if (Outer == null) return new MonkeyError($"Try to assign an unbound value {valName}");
            // 从本层往上逐层寻找最近的定义
            return Outer.TrySetBoundedVar(valName, val);
        }
    }
}