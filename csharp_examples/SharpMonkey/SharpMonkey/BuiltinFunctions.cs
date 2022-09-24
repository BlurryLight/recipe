using System;
using System.Globalization;

namespace SharpMonkey
{
    public static class BuiltinFunctions
    {
        public static IMonkeyObject Len(params IMonkeyObject[] args)
        {
            if (args.Length != 1)
            {
                return new MonkeyError($"wrong number of arguments to len");
            }

            var obj = args[0];
            return obj switch
            {
                MonkeyString stringObj => new MonkeyInteger(stringObj.Value.Length),
                MonkeyArray arrayObj => new MonkeyInteger(arrayObj.Elements.Count),
                _ => new MonkeyError($"argument {obj.Type()} to len is not supported")
            };
            // C#的一个Char是utf-16表示，对于BMP平面内的字符可以精确计数，对于非BMP内平面的不行
            // 精确计数需要o(n)的复杂度 ,一个设计上的考虑, 我决定选用O(1)的实现
            // var si = new StringInfo(stringObj.Value);
            // return new MonkeyInteger(si.LengthInTextElements);
        }
    }
}