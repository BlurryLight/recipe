using System;
using System.Collections.Generic;

namespace SharpMonkey.VM
{
    public enum SymbolScope : byte
    {
        Builtin,
        Global,
        Local,
        Free
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
        public List<Symbol> FreeSymbolsToPush;
        public int NumDefinitions => _store.Count;

        private SymbolScope _scope;

        private SymbolTable _outer; // parent SymbolTable
        public SymbolTable Outer => _outer;

        public SymbolTable(SymbolScope scope)
        {
            _store = new Dictionary<string, Symbol>();
            _scope = scope;
            _outer = null;
            FreeSymbolsToPush = new List<Symbol>();
        }

        public SymbolTable(SymbolTable outer, SymbolScope scope)
        {
            _outer = outer;
            _store = new Dictionary<string, Symbol>();
            if (scope < SymbolScope.Global)
            {
                throw new ArgumentException("BuiltinScope should have No parents");
            }

            FreeSymbolsToPush = new List<Symbol>();
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

        public Symbol DefineFree(Symbol original)
        {
            // 这里的逻辑非常绕。。
            // 当内层闭包的函数体编译时，会碰见无法解析的符号，从上层的作用域寻找到符号后，会通过DefineFree来标志这个符号是一个自由变量
            // _freeSymbols里保存的是上层作用域的符号，这样在调用内层的闭包时，上层才能正确压入闭包所需要的符号

            // example:
            // fn(a) { fn(b) {a + b}} 
            // fn(b)()执行时，除了通过GetLocal获取b变量，必须从某个地方取得a变量的值
            // 因此外部必须在执行fn(b)之前往栈里压入a的值
            // 所以freeSymbols要记录a的值在上层作用域的索引，以正确执行 OpGetLocal，压入正确的值
            var freeSymbol = new Symbol(original.Name, SymbolScope.Free, FreeSymbolsToPush.Count);
            _store[original.Name] = freeSymbol;
            FreeSymbolsToPush.Add(original);
            return freeSymbol;
        }

        public Symbol Resolve(string name)
        {
            bool found = _store.TryGetValue(name, out Symbol val);
            if (found) return val;

            if (_outer == null)
            {
                throw new KeyNotFoundException($"Variable {name} is not defined!");
            }

            var foundSymbol = _outer.Resolve(name);
            if (foundSymbol.Scope == SymbolScope.Global || foundSymbol.Scope == SymbolScope.Builtin) return foundSymbol;
            return DefineFree(foundSymbol);
        }
    }
}