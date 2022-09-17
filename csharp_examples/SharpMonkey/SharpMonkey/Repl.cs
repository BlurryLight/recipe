using System;
using System.ComponentModel.Design;

namespace SharpMonkey
{
    public class Repl
    {
        // for file
        // foreach (string line in File.ReadAllLines(fileName))
        private const string Prompt = ">>";

        public static void Start()
        {
            string line;
            while (true)
            {
                Console.Write(Prompt);
                line = Console.ReadLine();
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

                var evaled = Evaluator.Eval(program);
                System.Diagnostics.Debug.Assert(evaled != null);
                Console.WriteLine(evaled.Inspect());
            }
        }
    }
}