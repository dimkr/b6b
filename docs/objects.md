    {$local number 5}

*local* assigns a name to object, in the local scope.

    {$global number 5}

*global* assigns a name to object, in the global scope.

    {$export number}
    {$export number 5}

*export* assigns a name to object, in the calling scope. If no object is specified, *export* uses the object with the same name in the local scope.
