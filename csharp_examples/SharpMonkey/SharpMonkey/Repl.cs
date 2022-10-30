using System;
using System.Collections.Generic;
using System.ComponentModel.Design;
using SharpMonkey.VM;

namespace SharpMonkey
{
    public static class Repl
    {
        // for file
        // foreach (string line in File.ReadAllLines(fileName))
        private const string Prompt = ">>";

        public static bool UseVm = true;

        private struct CompilerVMContext
        {
            // for compiler
            public List<IMonkeyObject> CompilerConstantsPool;
            public Dictionary<HashKey, int> CompilerConstantIndex;
            public SymbolTable CompilerSymbolTable;

            // for VM
            public IMonkeyObject[] VmGlobals;
            public bool IsValid;
        }

        public static void Start()
        {
            // for evaluator 在不同的行之间保持状态，需要一个大的Context
            Environment env = new Environment();

            // 同理，对于VM，也需要在REPL的不同行之间维持状态

            var vmContext = new CompilerVMContext
            {
                IsValid = false
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

                if (!UseVm)
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
                        var compiler = vmContext.IsValid
                            ? new Compiler(vmContext.CompilerConstantsPool, vmContext.CompilerConstantIndex,
                                vmContext.CompilerSymbolTable)
                            : new Compiler();
                        compiler.Compile(program);
                        var vm = vmContext.IsValid
                            ? new MonkeyVM(compiler.Bytecode(), vmContext.VmGlobals)
                            : new MonkeyVM(compiler.Bytecode());

                        // 记录状态
                        vmContext.IsValid = true;
                        vmContext.CompilerConstantIndex = compiler.ConstantsPoolIndex;
                        vmContext.CompilerConstantsPool = compiler.ConstantsPool;
                        vmContext.CompilerSymbolTable = compiler.SymbolTable;
                        vmContext.VmGlobals = vm.Globals;

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