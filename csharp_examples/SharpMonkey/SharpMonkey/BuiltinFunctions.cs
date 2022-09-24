using System;
using System.Globalization;
using System.Linq;

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

        public static IMonkeyObject First(params IMonkeyObject[] args)
        {
            if (args.Length != 1)
            {
                return new MonkeyError($"wrong number of arguments to first");
            }

            var obj = args[0];
            return obj switch
            {
                MonkeyArray arrayObj => arrayObj.Elements.Count > 0 ? arrayObj.Elements[0] : null,
                _ => new MonkeyError($"argument {obj.Type()} to first is not supported")
            };
        }

        public static IMonkeyObject Last(params IMonkeyObject[] args)
        {
            if (args.Length != 1)
            {
                return new MonkeyError($"wrong number of arguments to last");
            }

            var obj = args[0];
            return obj switch
            {
                MonkeyArray arrayObj => arrayObj.Elements.Count > 0 ? arrayObj.Elements.Last() : null,
                _ => new MonkeyError($"argument {obj.Type()} to last is not supported")
            };
        }

        public static IMonkeyObject Rest(params IMonkeyObject[] args)
        {
            if (args.Length != 1)
            {
                return new MonkeyError($"wrong number of arguments to last");
            }

            var obj = args[0];
            return obj switch
            {
                MonkeyArray arrayObj => new MonkeyArray(arrayObj.Elements.Skip(1).ToList()),
                _ => new MonkeyError($"argument {obj.Type()} to last is not supported")
            };
        }

        public static IMonkeyObject Push(params IMonkeyObject[] args)
        {
            if (args.Length <= 1)
            {
                return new MonkeyError($"wrong number of arguments to push ");
            }

            var obj = args[0];
            switch (obj)
            {
                case MonkeyArray arrayObj:
                    var copyList = arrayObj.Elements;
                    for (int i = 1; i < args.Length; i++)
                    {
                        copyList.Add(args[i]);
                    }

                    return new MonkeyArray(copyList);
                default:
                    return new MonkeyError($"argument {obj.Type()} to push is not supported");
            }
        }

        public static IMonkeyObject Puts(params IMonkeyObject[] args)
        {
            foreach (var obj in args)
            {
                Console.WriteLine(obj.Inspect());
            }

            return null;
        }
    }
}