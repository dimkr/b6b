> The doer alone learneth. (Friedrich Nietzsche)

# Metaphysics

In **b6b**, all objects are represented as strings:

    Hello!

> All things are water. (Thales)

Lists are also represented as strings; the list items are delimited by whitespace:

    This is a list with 7 items

List items which contain whitespace can be enclosed in braces to inform **b6b** they should be treated as one list item:

    {This is the first item} of a list

> It differs in different substances in virtue of its rarefaction and
condensation. (Anaximenes)

Everything in **b6b** is an object, including code. A piece of **b6b** code is a list of strings. Each string is a **b6b** statement which instructs **b6b** to perform a certain operation. For example, the following script is a list of three statements:

    {$local a [$+ 1 2]}
    {$stdout writeln {a b}}
    {$exit 1}

# Object Types

There are two kinds of objects in **b6b**:

1. Anonymous objects
2. Named objects

Named objects have both a value and a name, while anonymous objects do not have a name. To obtain the value of a named object, prefix its name with *$*.

# Statements

Each **b6b** statement is a list of objects. The first object in a statement is invoked, while the objects that follow are passed to it as parameters.

For example, the following script invokes the *sleep* procedure (a named object) to pause for *2* (an anonymous object) seconds:

    {$sleep 2}

# Return Values

Every procedure has a return value. The return value of the previously executed procedure is called *_*. For example, the following script prints *7*:

    {$+ 4 3}
    {$stdout writeln $_}

# Invocation

In addition, a statement can invoke another statement by enclosing it in brackets:

    {$stdout writeln [$+ 4 3]}

When this statement is invoked, **b6b** invokes the internal statement *$+ 4 3*, assigns its return value in the outer statement and invokes *$stdout writeln 7*.

> Great things are ... done by a series of small things brought together. (Vincent Van Gogh)

# Naming Objects

Anonymous objects can be assigned a name using either *local* or *global*:

    {$local sum [$+ 3 4]}
    {$stdout writeln $sum}

Global objects are accessible by all procedures, while local objects cannot be used outside of the procedure that created them.

# Procedures

New procedures can be created using *proc*, which receives the procedure name and a list of statements:

    {$proc say_hello {
    	{$stdout writeln Hello!}
    	{$stdout writeln [$+ 4 3]}
    }}

    {$say_hello}


The return value of a procedure is the return value of its last statement, or the object passed to *return*:

    {$proc say_hello {
    	{$return [$+ 4 3]}
    	{$stdout writeln {this statement is not executed}}
    }}

    {$say_hello}

# Parameters

A procedure may access its parameters through objects named *1*, *2*, etc':

    {$proc say_hello {
    	{$stdout writeln $1}
    	{$stdout writeln [$+ 4 3]}
    }}

    {$say_hello Hello!}

# Streams

Streams are external sources of data. For example, both files and timers are represented as streams.

Typically, a stream can be read from or written to.

    {$local a [$open /etc/hosts]}
    {$stdout writeln [$a read]}

    {$local a [$open file.txt w]}
    {$stdout writeln Hello!}

# Threads

**b6b** allows multiple scripts to be executed in parallel. *spawn* starts a script in a separate thread of execution:

    {$spawn {
    	{$loop {
    		{$stdout writeln 1}
    	}}
    }}

    {$loop {
    	{$stdout writeln 2}
    }}


Since **b6b** allows only one thread to run at a given moment, it is inefficient to spawn threads to perform many short background operations. In such cases, use *co*:

    {$loop {
    	{$co {
    		{$map i [$range 10 20] {
    			{$stdout writeln $i}
    		}}
    	}}

    	{$map i [$range 30 40] {
    		{$stdout writeln $i}
    	}}
    }}

The list of statements passed to *co* is put in a queue and a single thread repeatedly pops one item from that queue and runs it, until the queue is empty.

# Event Loops

Event loops wait for events to occur and invoke an event handling procedure for each event. For example, an event loop can *read* a request from a network connection, *write* a response and terminate the connection, when a timer fires.

Event loops simplify development of programs that operate on multiple streams concurrently.

Event loops are created using *evloop* and managed using *add*, *remove* and *update*. In addition, procedures can be invoked after a given interval using *after* or periodically, using *every*.

    {$global loop [$evloop]}

    {$proc on_ping {
    	{$stdout writeln {ping result:}}
    	{$stdout write [$1 read]}
    	{$loop after 10 $after_ping}
    }}

    {$proc after_ping {
    	{$stdout writeln {after ping}}
    }}

    {$proc on_tick {
    	{$loop add [$list.index [$sh {ls -la} 2]] $on_ls {} {}}
    }}

    {$proc on_ls {
    	{$stdout writeln {files:}}
    	{$stdout write [$1 read]}
    }}

    {$proc ctrl_c {
    	{$stdout writeln bye}
    	{$exit}
    }}

    {$loop add [$list.index [$sh {ping 127.0.0.1}] 2] $on_ping {} {}}
    {$loop every 3 $on_tick}
    {$loop add [$signal $SIGINT] $ctrl_c {} {}}
    {$loop wait -1}
