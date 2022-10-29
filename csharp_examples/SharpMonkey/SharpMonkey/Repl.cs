using System;
using System.Collections.Generic;
using System.ComponentModel.Design;
using SharpMonkey.VM;

namespace SharpMonkey
{
    public class Repl
    {
        // for file
        // foreach (string line in File.ReadAllLines(fileName))
        private const string Prompt = ">>";

        public static bool _useVM = true;

        private struct CompilerVMContext
        {
            // for compiler
            public List<IMonkeyObject> CompilerConstantsPool;
            public Dictionary<HashKey, int> CompilerConstantIndex;
            public SymbolTable CompilerSymbolTable;

            // for VM
            public IMonkeyObject[] VMGlobals;
            public bool isValid;
        }

        public static void Start()
        {
            // for evaluator 在不同的行之间保持状态，需要一个大的Context
            Environment env = new Environment();

            // 同理，对于VM，也需要在REPL的不同行之间维持状态

            CompilerVMContext VMContext = new CompilerVMContext
            {
                isValid = false
            };
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
                        // 继承状态 
                        var compiler = VMContext.isValid
                            ? new Compiler(VMContext.CompilerConstantsPool, VMContext.CompilerConstantIndex,
                                VMContext.CompilerSymbolTable)
                            : new Compiler();
                        compiler.Compile(program);
                        var vm = VMContext.isValid
                            ? new MonkeyVM(compiler.Bytecode(), VMContext.VMGlobals)
                            : new MonkeyVM(compiler.Bytecode());

                        // 记录状态
                        VMContext.isValid = true;
                        VMContext.CompilerConstantIndex = compiler.ConstantsPoolIndex;
                        VMContext.CompilerConstantsPool = compiler.ConstantsPool;
                        VMContext.CompilerSymbolTable = compiler.SymbolTable;
                        VMContext.VMGlobals = vm.Globals;

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