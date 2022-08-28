namespace SharpMonkey
{
    public class Parser
    {
        private Lexer Lexer;
        private Token _curToken;
        private Token _peekToken;

        public Parser(Lexer lexer)
        {
            Lexer = lexer;
            // 需要调用两次才能让CurToken指向第一个Token
            NextToken();
            NextToken();
        }

        public void NextToken()
        {
            _curToken = _peekToken;
            _peekToken = Lexer.NextToken();
        }

        Ast.IStatement ParseStatement()
        {
            return null;
        }
        public Ast.MonkeyProgram ParseProgram()
        {
            var program = new Ast.MonkeyProgram();
            while (_curToken.Type != Constants.Eof)
            {
                var statement = ParseStatement();
                if (statement != null)
                {
                    program.Statements.Add(statement);
                }
                this.NextToken();
            }

            return program;
        }
    }
}