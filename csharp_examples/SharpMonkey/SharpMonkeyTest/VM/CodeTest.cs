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
                    new List<byte> {(byte) OpConstants.OpConstant, 0xFF, 0XFE})
            };

            foreach (var tt in testTable)
            {
                var instuction = OpcodeUtils.MakeBytes(tt.op, tt.operands.ToArray());
                Assert.AreEqual(tt.expected.Count, instuction.Count);
                for (int i = 0; i < instuction.Count; i++)
                {
                    Assert.AreEqual(tt.expected[i], instuction[i]);
                }
            }
        }
    }
}