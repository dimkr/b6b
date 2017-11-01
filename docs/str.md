    {$str.len abcd}

*str.len* returns the length of a string.

    {$str.index abcd 2}

*str.index* returns the character at a given index within a string.

    {$str.range abcd 0 2}

*str.range* returns a substring, identified by two indexes within a string.

    {$str.join , {a b c}}

*str.join* joins a list of strings, with a delimiter.

    {$str.split a,b,c ,}

*str.split* splits a string, by a delimiter.

    {$str.expand {a\tb}}

*str.expand* expands escape sequences in a string.

    {$str.in bc abcd}

*str.in* determines whether a string is a subset of another string.

    {$rtrim {a b  }}

*rtrim* removes trailing whitespace from a string.

    {$ltrim {a b  }}

*ltrim* removes leading whitespace from a string.
