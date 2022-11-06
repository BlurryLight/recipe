using System;
using System.IO;
using SharpMonkey.VM;

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

            // benchmark
            if (args.Length == 1 && args[0] == "benchmark")
            {
                Benchmark();
                return 0;
            }

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

                Console.WriteLine("We are in VM mode");
                var compiler = new Compiler();
                compiler.Compile(program);
                var vm = new MonkeyVM(compiler.Bytecode());
                vm.Run();
                return 0;
            }

            // REPL mode

            // -e evaluator
            if (args.Length == 2)
            {
                if (args[1].Contains("evaluator"))
                {
                    Repl.UseVm = false;
                }
            }

            Console.WriteLine("Welcome to monkey");
            Console.WriteLine("Feel free to type commands");
            Console.WriteLine($"You are in mode {(Repl.UseVm ? "VM" : "Evaluator")} ");
            Repl.Start();
            return 0;
        }

        private static void Benchmark()
        {
            string fib = @"
            let fibonacci = fn(x)
            {
                if(x == 0 || x == 1)
                {
                    return x;
                }
                
                return fibonacci(x - 1) + fibonacci(x - 2);};
                fibonacci(15);
            ";

            Parser parser = new Parser(new Lexer(fib));
            var program = parser.ParseProgram();

            Console.WriteLine("VM Test");
            var watch = new System.Diagnostics.Stopwatch();

            watch.Start();
            var compiler = new Compiler();
            compiler.Compile(program);
            var vm = new MonkeyVM(compiler.Bytecode());
            for (int i = 0; i < 100; i++)
            {
                vm.Run();
            }

            watch.Stop();

            // On My PC: 130ms
            Console.WriteLine($"VM Execution Time: {watch.ElapsedMilliseconds} ms");

            watch.Start();
            for (int i = 0; i < 100; i++)
            {
                Environment env = new Environment();
                Evaluator.Eval(program, env);
            }

            watch.Stop();

            // On My PC: 350ms
            Console.WriteLine($"Evaluator Execution Time: {watch.ElapsedMilliseconds} ms");
        }
    }
}