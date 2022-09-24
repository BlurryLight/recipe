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

            if (args[0] is not MonkeyString)
                return new MonkeyError($"argument {args[0].Type()} to len is not supported");
            var stringObj = (MonkeyString) args[0];
            return new MonkeyInteger(stringObj.Value.Length);
            // C#的一个Char是utf-16表示，对于BMP平面内的字符可以精确计数，对于非BMP内平面的不行
            // 精确计数需要o(n)的复杂度 ,一个设计上的考虑, 我决定选用O(1)的实现
            // var si = new StringInfo(stringObj.Value);
            // return new MonkeyInteger(si.LengthInTextElements);
        }
    }
}