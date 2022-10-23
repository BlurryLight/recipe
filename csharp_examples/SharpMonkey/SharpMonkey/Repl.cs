﻿using System;
using System.ComponentModel.Design;
using SharpMonkey.VM;

namespace SharpMonkey
{
    public class Repl
    {
        // for file
        // foreach (string line in File.ReadAllLines(fileName))
        private const string Prompt = ">>";

        private static bool _useVM = true;

        public static void Start()
        {
            Environment env = new Environment();
            while (true)
            {
                Console.Write(Prompt);
                var line = Console.ReadLine();
                if (line == null)
                    break;
                Lexer lexer = new Lexer(line);
                /* print all lexical token
                for (var token = lexer.NextToken(); token.Type != Constants.Eof; token = lexer.NextToken())
                {
                    // 10个字符宽，左对齐                               // 十个字符宽，右对齐
                    Console.WriteLine($"TokenType: {token.Type,-10} Literal: {token.Literal,10}");
                }
                */

                Parser parser = new Parser(lexer);
                var program = parser.ParseProgram();
                if (parser.Errors.Count > 0)
                {
                    foreach (var msg in parser.Errors)
                    {
                        Console.WriteLine(msg);
                    }
                }

                if (!_useVM)
                {
                    var evaled = Evaluator.Eval(program, env);
                    if (evaled != null)
                        Console.WriteLine(evaled.Inspect());
                }
                else
                {
                    try
                    {
                        var compiler = new Compiler();
                        compiler.Compile(program);
                        var vm = new MonkeyVM(compiler.Bytecode());
                        vm.Run();
                        var evaled = vm.LastPoppedStackElem();
                        Console.WriteLine(evaled.Inspect());
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine(e);
                    }
                }
            }
        }
    }
}