### crashdump ###

Demonstrates how to have a C process (crasher.exe) have a stack trace and minidump taken if it crashes. Both are done by another process (dumper.exe), which is crucial to getting a dump with a sane stack trace.

crasher registers an exception handler with SetUnhandledExceptionFilter. This handler starts dumper and waits for it to exit. It passes its process ID, thread ID, and exception pointers (containing the context record) to dumper in the commandline. The exception pointer is useful because it contains the context of the original exception instead of the current context (inside the exception handler). crasher then waits for dumper to finish (but see miscellaneous 1 below).

On startup, dumper use the process and thread IDs from its commandline to open handles to crasher. Then it uses some calls to ReadProcessMemory to fetch dumper's exception pointers and context record. It then uses this context record to set up the original stack frame and starts walking the stack using StackWalk64. It also uses the Sym* API to load crasher's PDB and other PDBs so that it can print function names, and parameter names and values for each frame. This verbose code is under printStack(), and is mostly composed of interpreting the types of the function parameters and extracting their values into human-readable form.

Lastly, dumper uses MiniDumpWriteDump to write a minidump.

The motivation to have a stack printer separate from a minidump writer is to cover the cases where the user of crasher may be concerned about private data in the dump that they would not like to share with you. With a textual stack trace, they can vet it to strip private data before sending it to you.


### Miscellaneous ###

- Dumper needs to suspend crasher's thread so that it can walk its stack. Although crasher does eventually suspend itself by waiting on dumper's process handle, there is the possibility that it hasn't reached this point before dumper starts walking it, so an explicit SuspendThread is necessary to be safe. This has the disadvantage, though, that dumper needs to resume the thread or terminate the process before it exits, or crasher will remain suspended until the user kills it. Thus dumper needs to be resilient against crashing.

- If dumper links against the default Windows dbghelp.dll (in C:\Windows\System32 / SysWow64), then stack traces can only work if PDBs are available locally or on a network share. That is, symbol servers over HTTP will not work. Unless you want to ship PDBs for your application to every user, you'll have to bundle dbgcore.dll, dbghelp.dll and symsrv.dll from the Windows 10 Debug Tools ("C:\Program Files (x86)\Windows Kits\10\Debuggers\x86" and x64). The crasher project does this in its PostBuildEvent.

- Register values are printed by writing them to a "scratch space" page allocated on crasher and pretending that's where they're being read from. This means the code that pretty-prints the value of a function parameter always needs to dereference a memory location in the crasher process. Currently this page is allocated only when dumper is walking the stack. This won't work if crasher is OOM, in which case it'll be better for crasher to always reserve this space as parts of its startup, and pass its address to dumper when invoking it as another CLI argument.

### License ###

APL 2.0
