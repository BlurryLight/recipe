using System;
using System.Collections.Generic;
using System.Linq;
using NUnit.Framework;
using SharpMonkey;
using SharpMonkey.VM;

namespace SharpMonkeyTest
{
    using Instructions = List<byte>;
    using Opcode = Byte;

    public class CompilerTest
    {
        [SetUp]
        public void Setup()
        {
        }

        public struct CompilerTestCase
        {
            public string input;
            public List<Object> expectedConstants;
            public List<Instructions> expectedInstructions;
        };

        // 把多条OpCode的Bytecode连接到一起放到一个连续的buffer里
        private Instructions concatInstructions(List<Instructions> linesOfInstruction)
        {
            var res = new Instructions();
            foreach (var line in linesOfInstruction)
            {
                res.AddRange(line);
            }

            return res;
        }

        public static Ast.MonkeyProgram Parse(string input)
        {
            var p = new Parser(new Lexer(input));
            return p.ParseProgram();
        }


        [Test]
        public void TestReadOperands()
        {
            var testTable = new List<(Opcode op, List<int> operands, int bytesRead)>
            {
                new((byte) OpConstants.OpConstant, new List<int>() {65534}, 2),
                new((byte) OpConstants.OpAdd, new List<int>(), 0)
            };

            foreach (var tc in testTable)
            {
                var instruction = OpcodeUtils.MakeBytes(tc.op, tc.operands.ToArray());
                var def = OpcodeUtils.Lookup(tc.op);
                var (operandsRead, n) = OpcodeUtils.ReadOperands(instruction.Skip(1).ToList(), def);
                Assert.AreEqual(tc.bytesRead, n);
                for (int i = 0; i < operandsRead.Count; i++)
                {
                    Assert.AreEqual(tc.operands[i], operandsRead[i]);
                }
            }
        }

        [Test]
        public void TestDecodeInstructionString()
        {
            var instructions = new List<Instructions>
            {
                OpcodeUtils.MakeBytes(OpConstants.OpAdd),
                OpcodeUtils.MakeBytes(OpConstants.OpConstant, 2),
                OpcodeUtils.MakeBytes(OpConstants.OpConstant, 65534)
            };
            var expected = $"0000 OpAdd NULL\n" +
                           $"0001 OpConstant 2\n" +
                           $"0004 OpConstant 65534\n";
            var concatted = concatInstructions(instructions);
            Assert.AreEqual(expected, OpcodeUtils.DecodeInstructions(concatted));
        }

        [Test]
        public void TestIntegerArithmetic()
        {
            var testTable = new List<CompilerTestCase>();

            var newCase = new CompilerTestCase
            {
                input = "1 + 2",
                expectedConstants = new List<Object> {1, 2},
                expectedInstructions = new List<Instructions>
                {
                    OpcodeUtils.MakeBytes(OpConstants.OpConstant, 0),
                    OpcodeUtils.MakeBytes(OpConstants.OpConstant, 1),
                    OpcodeUtils.MakeBytes(OpConstants.OpAdd)
                }
            };
            testTable.Add(newCase);

            foreach (var testCase in testTable)
            {
                var program = Parse(testCase.input);
                var compiler = new Compiler();
                compiler.Compile(program);

                var bytecode = compiler.Bytecode();
                TestInstructions(testCase.expectedInstructions, bytecode.Instructions);
                TestConstants(testCase.expectedConstants, bytecode.Constants);
            }
        }

        private void TestConstants(List<Object> testCaseExpectedConstants, List<IMonkeyObject> bytecodeConstants)
        {
            Assert.AreEqual(testCaseExpectedConstants.Count, bytecodeConstants.Count);
            for (var i = 0; i < testCaseExpectedConstants.Count; i++)
            {
                var constant = testCaseExpectedConstants[i];
                var actual = bytecodeConstants[i];
                switch (constant)
                {
                    case int:
                    case long:
                        long val = Convert.ToInt64(constant);
                        TestIntegerObject(val, actual);
                        break;
                    default:
                        Assert.Fail("should not be here");
                        return;
                }
            }
        }

        public static void TestIntegerObject(long constant, IMonkeyObject actual)
        {
            var obj = actual as MonkeyInteger;
            Assert.NotNull(obj);
            Assert.AreEqual(constant, obj.Value, $"obj.value {obj.Value} is not expected {constant} ");
        }

        private void TestInstructions(List<Instructions> testCaseExpectedInstructions,
            Instructions bytecodeInstructions)
        {
            var concatted = concatInstructions(testCaseExpectedInstructions);
            Assert.AreEqual(concatted.Count, bytecodeInstructions.Count,
                $"Expected bytes:\n{OpcodeUtils.DecodeInstructions(concatted)}" +
                $"Actual bytes:\n{OpcodeUtils.DecodeInstructions(bytecodeInstructions)}"
            );
            for (int i = 0; i < concatted.Count; i++)
            {
                Assert.AreEqual(concatted[i], bytecodeInstructions[i]);
            }
        }
    }
}