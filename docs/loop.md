
    {$map {a b} [$range 0 9] {
    	{$+ $a $b}
    }}

*map* receives a list of object names and a list, then repeatedly runs a list of statements with local named objects that specify the next items of the list. The return value of *map* is a list of values returned by all iterations of the loop.

If an iteration of the loop calls *continue*, its return value is ignored.

    {$loop {
    	{$stdout writeln a}
    }}

*loop* repeatedly runs a list of statements.

    {$range 1 5}

*range* returns a list of all whole numbers in a given range.
