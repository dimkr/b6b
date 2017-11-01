
    {$proc append_stuff {
    	{$list.append $. c}
    	{$return $.}
    } {a b}}

*proc* creates a new procedure. If optional private data is specified, it is named *.* and preserved across procedure calls.
