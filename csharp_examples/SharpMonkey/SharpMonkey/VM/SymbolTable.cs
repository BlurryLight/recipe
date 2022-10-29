using System;
using System.Collections.Generic;

namespace SharpMonkey.VM
{
    public enum SymbolScope
    {
        Global
    }

    public struct Symbol
    {
        public string Name;
        public SymbolScope Scope;
        public int Index;
    }

    public class SymbolTable
    {
        private Dictionary<string, Symbol> _store;
        public int NumDefinitions => _store.Count;

        public SymbolTable()
        {
            _store = new Dictionary<string, Symbol>();
        }

        public Symbol Define(string name)
        {
            Symbol s = new Symbol()
            {
                Name = name,
                Index = NumDefinitions,
                Scope = SymbolScope.Global
            };
            _store.Add(name, s);
            return s;
        }

        public Symbol Resolve(string name)
        {
            return _store[name];
        }
    }
}