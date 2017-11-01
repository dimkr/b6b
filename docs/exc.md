
    {$try {
    	{$/ 4 0}
    } {
    	{$stdout writeln $_}
    } {
    	{$stdout writeln {}}
    }}

*try* receives a list of statements and runs it.

If an error condition is triggered, a second list of statements is executed.

Finally, a third list of statements is executed.

**If only one lists of statements has been specified**, error conditions triggered by the first list of statements are silenced.
