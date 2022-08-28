using System;

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
                for (var token = lexer.NextToken(); token.Type != Constants.Eof; token = lexer.NextToken())
                {
                    // 10个字符宽，左对齐                               // 十个字符宽，右对齐
                    Console.WriteLine($"TokenType: {token.Type,-10} Literal: {token.Literal,10}");
                }
            }
        }
    }
}