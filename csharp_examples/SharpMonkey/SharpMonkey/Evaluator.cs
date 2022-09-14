using System.Collections.Generic;

namespace SharpMonkey
{
    public class Evaluator
    {

        public static MonkeyObject EvalStatments(List<Ast.IStatement> stmts)
        {
            MonkeyObject result = null;
            foreach (var statement in stmts)
            {
                result = Eval(statement);
            }

            return result;
        }
        public static MonkeyObject Eval(Ast.INode node)
        {
            switch (node) // switch on type
            {

                case Ast.MonkeyProgram program:
                    return EvalStatments(program.Statements);
                case Ast.ExpressionStatement stmt:
                    return Eval(stmt.Expression);
                case Ast.IntegerLiteral val:
                    return new MonkeyInteger(val.Value);
                case Ast.BooleanLiteral val:
                    // a small optimization. will never create new Boolean object
                    return val.Value ? MonkeyBoolean.TrueObject : MonkeyBoolean.FalseObject;
                    
                    
            }

            return null;
        }
    }
}