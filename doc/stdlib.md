# Interpreters

    {[$b6b {a b}] call {{$stdout writeln $@}}}

**b6b** creates a new interpreter with given global scope parameters. The interpreter object provides a 'call' method, which runs a list of statements.

# Object Names

    {$local number 5}

*local* assigns a name to object, in the local scope.

    {$global number 5}

*global* assigns a name to object, in the global scope.

    {$export number}
    {$export number 5}

*export* assigns a name to object, in the calling scope. If no object is specified, *export* uses the object with the same name in the local scope.

# Flow Control

    {$if [$== 4 3] {
    	{$stdout writeln a}
    } {
    	{$stdout writeln b}
    }}

*if* executes a list of statements if a condition is true. Otherwise, if the condition is false and a second list of statements was specified, it is executed instead.

    {$return}
    {$return 5}

*return* stops the execution of the currently running procedure and sets its return value. If no return value is specified, the procedure returns an empty string.

    {$continue}

*continue* stops the current iteration of a loop.

    {$break}

*break* stops the execution of a loop.

    {$throw}
    {$throw {error message}}

*throw* triggers an error condition. Error conditions are propagated until caught by a *try* block, or until they reach the global scope. In the latter case, the script exits immediately with a non-zero exit code.

    {$exit}
    {$exit 1}

*exit* stops the execution of the script.

    {$yield}

*yield* switches to another thread, if there is one.

*yield* should be called only by threads that repeatedly check for a certain condition, which is guaranteed to be false unless another thread performs some action. For example, a thread that runs a loop which removes an item from a list and performs a certain action on it, should call *yield* every time the list is empty, since no item will be added to the list until another thread has a chance to do so. The *yield* call improves efficiency because it allows the polling thread to postpone the next iteration of its polling loop, when the polled condition is guaranteed to be false if polled again immediately.

# Strings

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

# Lists

    {$list.new a b c {d e}}

*list.new* creates a new list containing its arguments.

    {$list.len {a b c}}

*list.len* returns the length of a list.

    {$list.append $list f}

*list.append* appends an item to a list.

    {$list.extend $list {f g}}

*list.extend* adds all items of a list to an existing list.

    {$list.index {a b c} 0}

*list.index* returns the list item at a given index.

    {$list.range {a b c d e f} 2 4}

*list.range* returns the items of a list, between two indices.

    {$list.in a {a b c}}

*list.in* determines whether an item belongs to a list.

    {$list.pop $list 1}

*list.pop* removes the item at a given index from a list.

    {$choice {a b c}}

*choice* returns a pseudo-random list item.

# Dictionaries

    {$dict.get {human 2 dog 4} snake 0}

*dict.get* returns the value associated with a dictionary key. If the key is not found and a fallback value was specified, that value is returned. Otherwise, an error condition is triggered.

    {$dict.set $legs snake 0}

*dict.set* associates a value with a dictionary key. If the key already exists, the value associated with it is replaced.

    {$dict.unset $legs snake}

*dict.unset* removes a key from a dictionary.

# Loops

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

# Procedures

    {$proc append_stuff {
    	{$list.append $. c}
    	{$return $.}
    } {a b}}

*proc* creates a new procedure. If optional private data is specified, it is named *.* and preserved across procedure calls.

# Exceptions

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

# Evaluation

    {$eval {[$+ 4 5]}}

*eval* evaluates an expression.

    {$repr {hello world}}

*repr* returns a human-readable, printable representation of an object.

    {$call {
    	{$stdout writeln a}
    	{$stdout writeln b}
    }}

*call* executes a list of statements.

# Threading

    {$spawn {
    	{$loop {
    		{$stdout writeln a}
    	}}
    }}

*spawn* executes a list of statements in a new thread.

    {$co {
    	{$map i [$range 1 20] {
    		{$stdout writeln $i}
    	}}
    }}

*co* puts a list of statements in a queue. If the queue is empty, *co* starts a thread that calls each item in the queue, then exits.

**The statements must not block.**
