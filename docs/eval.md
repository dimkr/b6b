    {$eval {[$+ 4 5]}}

*eval* evaluates an expression.

    {$repr {hello world}}

*repr* returns a human-readable, printable representation of an object.

    {$call {
    	{$stdout writeln a}
    	{$stdout writeln b}
    }}

*call* executes a list of statements.
