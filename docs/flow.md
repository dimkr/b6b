
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
