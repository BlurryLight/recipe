﻿using System;
using System.Collections.Generic;
using System.Linq;
using NUnit.Framework;
using SharpMonkey;
using SharpMonkey.VM;

namespace SharpMonkeyTest
{
    using Instructions = List<byte>;
    using Opcode = Byte;

    public class OpCodeTest
    {
        [SetUp]
        public void Setup()
        {
        }

        [Test]
        public void TestMakeFunc()
        {
            var testTable = new List<(Opcode op, List<int> operands, List<byte> expected)>
            {
                new((byte) OpConstants.OpConstant, new List<int> {65534},
                    new List<byte> {(byte) OpConstants.OpConstant, 0xFF, 0XFE}),
                new((byte) OpConstants.OpAdd, new List<int>(),
                    new List<byte> {(byte) OpConstants.OpAdd}),
                new((byte) OpConstants.OpPop, new List<int>(),
                    new List<byte> {(byte) OpConstants.OpPop}),
                new((byte) OpConstants.OpSub, new List<int>(),
                    new List<byte> {(byte) OpConstants.OpSub}),
                new((byte) OpConstants.OpMul, new List<int>(),
                    new List<byte> {(byte) OpConstants.OpMul}),
                new((byte) OpConstants.OpDiv, new List<int>(),
                    new List<byte> {(byte) OpConstants.OpDiv}),
            };

            foreach (var tt in testTable)
            {
                var instruction = OpcodeUtils.MakeBytes(tt.op, tt.operands.ToArray());
                Assert.AreEqual(tt.expected.Count, instruction.Count);
                for (int i = 0; i < instruction.Count; i++)
                {
                    Assert.AreEqual(tt.expected[i], instruction[i]);
                }
            }
        }
    }
}