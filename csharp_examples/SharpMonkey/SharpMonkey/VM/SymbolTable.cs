using System;
using System.Collections.Generic;

namespace SharpMonkey.VM
{
    public enum SymbolScope : byte
    {
        Builtin,
        Global,
        Local,
    }

    public class Symbol
    {
        protected bool Equals(Symbol other)
        {
            return Name == other.Name && Scope == other.Scope && Index == other.Index;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != this.GetType()) return false;
            return Equals((Symbol) obj);
        }

        public override int GetHashCode()
        {
            return HashCode.Combine(Name, (int) Scope, Index);
        }

        public readonly string Name;
        public readonly SymbolScope Scope;
        public readonly int Index;

        public Symbol(string name, SymbolScope scope, int index)
        {
            Name = name;
            Scope = scope;
            Index = index;
        }

        // MSDN shows how to creat user-defined equals
        // https://learn.microsoft.com/en-us/dotnet/csharp/programming-guide/statements-expressions-operators/how-to-define-value-equality-for-a-type
        public static bool operator ==(Symbol left, Symbol right)
        {
            if (left is null)
            {
                if (right is null)
                {
                    return true;
                }

                // Only the left side is null.
                return false;
            }

            return left.Equals(right);
        }

        public static bool operator !=(Symbol left, Symbol right)
        {
            return !(left == right);
        }
    }

    public class SymbolTable
    {
        private Dictionary<string, Symbol> _store;
        public int NumDefinitions => _store.Count;

        private SymbolScope _scope;

        private SymbolTable _outer; // parent SymbolTable
        public SymbolTable Outer => _outer;

        public SymbolTable(SymbolScope scope)
        {
            _store = new Dictionary<string, Symbol>();
            _scope = scope;
            _outer = null;
        }

        public SymbolTable(SymbolTable outer, SymbolScope scope)
        {
            _outer = outer;
            _store = new Dictionary<string, Symbol>();
            if (scope < SymbolScope.Global)
            {
                throw new ArgumentException("BuiltinScope should have No parents");
            }

            _scope = scope;
        }

        public Symbol Define(string name)
        {
            Symbol s = new Symbol(name, _scope, NumDefinitions);
            _store.Add(name, s);
            return s;
        }

        public Symbol DefineBuiltin(int index, string name)
        {
            if (_scope != SymbolScope.Builtin)
            {
                throw new Exception("DefineBuiltin is only available for BuiltinScope");
            }

            Symbol s = new Symbol(name, SymbolScope.Builtin, index);
            _store.Add(name, s);
            return s;
        }

        public Symbol Resolve(string name)
        {
            bool found = _store.TryGetValue(name, out Symbol val);
            if (found) return val;

            if (_outer == null)
            {
                throw new KeyNotFoundException($"Variable {name} is not defined!");
            }

            return _outer.Resolve(name);
        }
    }
}