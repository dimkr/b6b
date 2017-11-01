
    {$dict.get {human 2 dog 4} snake 0}

*dict.get* returns the value associated with a dictionary key. If the key is not found and a fallback value was specified, that value is returned. Otherwise, an error condition is triggered.

    {$dict.set $legs snake 0}

*dict.set* associates a value with a dictionary key. If the key already exists, the value associated with it is replaced.

    {$dict.unset $legs snake}

*dict.unset* removes a key from a dictionary.
