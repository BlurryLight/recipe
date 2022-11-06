﻿using System.Collections.Generic;
using NUnit.Framework;
using SharpMonkey.VM;

namespace SharpMonkeyTest.VM
{
    public class SymbolTableTest
    {
        [Test]
        public void TestSymbolDefineGlobal()
        {
            var expected = new Dictionary<string, Symbol>
            {
                {"a", new Symbol("a", SymbolScope.Global, 0)},
                {"b", new Symbol("b", SymbolScope.Global, 1)},
            };

            var testCase = new SymbolTable(SymbolScope.Global);
            var symbolA = testCase.Define("a");
            Assert.AreEqual(1, testCase.NumDefinitions);
            var symbolB = testCase.Define("b");
            Assert.AreEqual(2, testCase.NumDefinitions);
            Assert.AreEqual(expected["a"], symbolA);
            Assert.AreEqual(expected["b"], symbolB);
        }

        [Test]
        public void TestSymbolResolveGlobal()
        {
            var expected = new Dictionary<string, Symbol>
            {
                {"a", new Symbol("a", SymbolScope.Global, 0)},
                {"b", new Symbol("b", SymbolScope.Global, 1)},
            };

            var testCase = new SymbolTable(SymbolScope.Global);
            testCase.Define("a");
            testCase.Define("b");

            Assert.AreEqual(expected["a"], testCase.Resolve("a"));
            Assert.AreEqual(expected["b"], testCase.Resolve("b"));
        }

        [Test]
        public void TestSymbolLocals()
        {
            var expected = new Dictionary<string, Symbol>
            {
                {"a", new Symbol("a", SymbolScope.Global, 0)},
                {"b", new Symbol("b", SymbolScope.Global, 1)},
                {"c", new Symbol("c", SymbolScope.Local, 0)},
                {"d", new Symbol("d", SymbolScope.Local, 1)},
                {"e", new Symbol("e", SymbolScope.Local, 0)},
                {"f", new Symbol("f", SymbolScope.Local, 1)},
            };

            var globalTable = new SymbolTable(SymbolScope.Global);
            var firstScopeTable = new SymbolTable(globalTable, SymbolScope.Local);
            var secondScopeTable = new SymbolTable(firstScopeTable, SymbolScope.Local);

            var symbolA = globalTable.Define("a");
            var symbolB = globalTable.Define("b");
            var symbolC = firstScopeTable.Define("c");
            var symbolD = firstScopeTable.Define("d");
            var symbolE = secondScopeTable.Define("e");
            var symbolF = secondScopeTable.Define("f");

            Assert.AreEqual(2, globalTable.NumDefinitions);
            Assert.AreEqual(2, firstScopeTable.NumDefinitions);
            Assert.AreEqual(2, secondScopeTable.NumDefinitions);

            Assert.AreEqual(expected["a"], symbolA);
            Assert.AreEqual(expected["b"], symbolB);
            Assert.AreEqual(expected["c"], symbolC);
            Assert.AreEqual(expected["d"], symbolD);
            Assert.AreEqual(expected["e"], symbolE);
            Assert.AreEqual(expected["f"], symbolF);

            Assert.AreEqual(expected["a"], secondScopeTable.Resolve("a"));
            Assert.AreEqual(expected["b"], secondScopeTable.Resolve("b"));
            Assert.AreEqual(expected["c"], secondScopeTable.Resolve("c"));
            Assert.AreEqual(expected["d"], secondScopeTable.Resolve("d"));
            Assert.AreEqual(expected["e"], secondScopeTable.Resolve("e"));
            Assert.AreEqual(expected["f"], secondScopeTable.Resolve("f"));
        }

        [Test]
        public void TestSymbolBuiltin()
        {
            var expected = new Dictionary<string, Symbol>
            {
                {"a", new Symbol("a", SymbolScope.Builtin, 0)},
                {"b", new Symbol("b", SymbolScope.Builtin, 1)},
                {"c", new Symbol("c", SymbolScope.Builtin, 2)},
                {"d", new Symbol("d", SymbolScope.Builtin, 3)},
            };

            var builtinSymbolTable = new SymbolTable(SymbolScope.Builtin);
            var firstScopeTable = new SymbolTable(builtinSymbolTable, SymbolScope.Local);
            var secondScopeTable = new SymbolTable(firstScopeTable, SymbolScope.Local);

            foreach (var pair in expected)
            {
                builtinSymbolTable.DefineBuiltin(pair.Value.Index, pair.Key);
            }

            foreach (var table in new[] {builtinSymbolTable, firstScopeTable, secondScopeTable})
            {
                Assert.AreEqual(expected["a"], table.Resolve("a"));
                Assert.AreEqual(expected["b"], table.Resolve("b"));
                Assert.AreEqual(expected["c"], table.Resolve("c"));
                Assert.AreEqual(expected["d"], table.Resolve("d"));
            }
        }
    }
}