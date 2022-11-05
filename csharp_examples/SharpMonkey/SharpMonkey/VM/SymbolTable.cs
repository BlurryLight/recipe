using System;
using System.Collections.Generic;

namespace SharpMonkey.VM
{
    public enum SymbolScope
    {
        Global,
        Local
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

        private SymbolTable _outer; // parent SymbolTable

        public SymbolTable()
        {
            _store = new Dictionary<string, Symbol>();
            _outer = null;
        }

        public SymbolTable(SymbolTable outer)
        {
            _outer = outer;
            _store = new Dictionary<string, Symbol>();
        }

        public Symbol Define(string name)
        {
            Symbol s = new Symbol(name, _outer == null ? SymbolScope.Global : SymbolScope.Local, NumDefinitions);
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