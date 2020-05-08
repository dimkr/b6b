# Overview

**b6b** is a simple, dynamic, procedural and reflective scripting language inspired by [Tcl](http://www.tcl.tk/), [CPython](http://www.python.org/), Lisp, shell scripting and various ideas or views in western philosophy.

**b6b** follows five design principles:

1. Everything is an object
2. Every object is a string
3. Every object is a procedure
4. Every statement is a procedure call
5. Transparent parallelism

# Use Cases

**b6b** is especially useful as:

1. A more powerful, yet simple alternative to shell scripting
2. A thin layer above C and system calls, which provides exceptions, garbage collection, easy parallelism and a dynamic type system
3. An embedded interpreter in a big software project
4. An extremely lightweight scripting language, for rapid development of fault-tolerant embedded software

# Design

**b6b** is implemented as a library, *libb6b*. This library exposes a rich API that allows applications to embed **b6b**. For example, **b6b** also includes an interactive interpreter application built on top of *libb6b*.

Each **b6b** interpreter uses two kinds of threads:

1. The "interpreter thread", the thread which calls the *libb6b* API to create an interpreter instance and run a script through it
2. A pool of "offloading threads", spawned during the initialization of the interpreter

**b6b** implements multi-threading using fibers (or, green threads) while a simple, round-robin scheduler relies on voluntary preemption to prevent starvation. **All these threads run on a single native thread: the interpreter thread.**

Therefore, **b6b**'s implementation of multi-threading does not benefit from SMP, but it is lock-free, lightweight, safe (for example, safe for embedding in heavily multi-threaded applications) and simple, which are far more important design considerations for the uses cases **b6b** shines at.

As many system calls are non-blocking by nature, most operations in **b6b** are non-blocking as well. However, under multi-threaded scenarios, the interpreter thread **delegates blocking or slow operations** (such as compression or file creation) to an offloading thread, while the interpreter thread continues to run other fibers.

Since multiple script threads may perform blocking operations **at the same time** and the number of offloading threads is smal, this offloading mechanism is mostly useful in situations such as an event-based server, where the only blocking operation is waiting for more events, while previous events are dealt with in coroutines.

# License

**b6b** is free and unencumbered software released under the terms of the [Apache License Version 2.0](https://www.apache.org/licenses/LICENSE-2.0); see *COPYING* for the license text.

For a list of its authors and contributors, see *AUTHORS*.
