using System;
using System.IO;

namespace SharpMonkey
{
    class Program
    {
        static int Main(string[] args)
        {
            Console.CancelKeyPress += delegate(object sender, ConsoleCancelEventArgs args)
            {
                System.Environment.Exit(0);
            };
            // C#的Args[0]不是程序名，程序名需要用下面一行取得。
            // C#的args从参数直接开始
            // Environment.GetCommandLineArgs()[0];
            if (args.Length == 1)
            {
                string scriptContent = File.ReadAllText(args[0]);
                Parser parser = new Parser(new Lexer(scriptContent));
                var program = parser.ParseProgram();
                if (parser.Errors.Count > 0)
                {
                    foreach (var msg in parser.Errors)
                    {
                        Console.WriteLine(msg);
                    }
                }

                Environment env = new Environment();
                var evaled = Evaluator.Eval(program, env);
                if (evaled is MonkeyError)
                {
                    Console.WriteLine(evaled.Inspect());
                    return -1;
                }

                return 0;
            }

            // REPL mode

            // -e evaluator
            if (args.Length == 2)
            {
                if (args[1].Contains("evaluator"))
                {
                    Repl._useVM = false;
                }
            }

            Console.WriteLine("Welcome to monkey");
            Console.WriteLine("Feel free to type commands");
            Console.WriteLine($"You are in mode {(Repl._useVM ? "VM" : "Evaluator")} ");
            Repl.Start();
            return 0;
        }
    }
}