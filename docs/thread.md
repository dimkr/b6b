
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
