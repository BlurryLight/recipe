using System.Collections.Generic;
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
                {"a", new Symbol() {Index = 0, Name = "a", Scope = SymbolScope.Global}},
                {"b", new Symbol() {Index = 1, Name = "b", Scope = SymbolScope.Global}}
            };

            var testCase = new SymbolTable();
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
                {"a", new Symbol() {Index = 0, Name = "a", Scope = SymbolScope.Global}},
                {"b", new Symbol() {Index = 1, Name = "b", Scope = SymbolScope.Global}}
            };

            var testCase = new SymbolTable();
            testCase.Define("a");
            testCase.Define("b");

            Assert.AreEqual(expected["a"], testCase.Resolve("a"));
            Assert.AreEqual(expected["b"], testCase.Resolve("b"));
        }
    }
}