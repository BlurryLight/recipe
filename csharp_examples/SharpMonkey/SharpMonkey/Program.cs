using System;

namespace SharpMonkey
{
    class Program
    {
        static int Main(string[] args)
        {
            Console.CancelKeyPress += delegate(object sender, ConsoleCancelEventArgs args) { Environment.Exit(0); };
            Console.WriteLine("Welcome to monkey");
            Console.WriteLine("Feel free to type commands");
            Repl.Start();
            return 0;
        }
    }
}