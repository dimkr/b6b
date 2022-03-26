    {$ssl.client 3 localhost 0}

*ssl.client* creates a TLS client stream, using the file descriptor of a TCP client stream. The optional third argument determines whether or not certificate validation is enabled.

**If the third argument is omitted**, certificate validation is enabled.

    {$ssl.server server.crt server.key}

*ssl.server* creates a TLS server.
