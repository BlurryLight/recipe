using System;
using System.IO;
using SharpMonkey.VM;

namespace SharpMonkey
{
    class Program
    {
        /*
         */
        static int Main(string[] args)
        {
            Console.CancelKeyPress += delegate(object sender, ConsoleCancelEventArgs args)
            {
                System.Environment.Exit(0);
            };

            var helper = @"  Program.exe benchmark  // bench \n
           Program.exe -e evaluator file // evaluator mode \n
           Program.exe -e vm file // vm mode \n
           Program.exe -e evaluator // evaluator repl \n
           Program.exe -e vm // vm repl \n
           Program.exe file // vm mode\n";
            if (args.Length > 3)
            {
                Console.WriteLine(helper);
                return 0;
            }

            // benchmark
            if (args.Length == 1 && args[0] == "benchmark")
            {
                Benchmark();
                return 0;
            }

            if (args.Length == 1 || (args.Length == 3 && args[0] == "-e" && args[1] == "vm"))
            {
                string scriptContent = File.ReadAllText(args[^1]);
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


            if ((args.Length == 3 && args[0] == "-e" && args[1] == "evaluator"))
            {
                Console.WriteLine("We are in Evaluator mode");
                string scriptContent = File.ReadAllText(args[^1]);
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
                    Repl.UseVm = false;
                }

                Console.WriteLine("Welcome to monkey");
                Console.WriteLine("Feel free to type commands");
                Console.WriteLine($"You are in mode {(Repl.UseVm ? "VM" : "Evaluator")} ");
                Repl.Start();
            }

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