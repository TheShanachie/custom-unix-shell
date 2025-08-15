# Unix Shell

This project contains a simple homemade Unix shell. This project provides simple implementation of basic bash commands and full usage of the Linux (Posix) environment. The shell is the heart of
the command-line interface, and thus is central to the Unix/C programming
environment.  Knowing how the shell itself is built is crucial to built shell 
proficiency. This shell is a modified version of the Unix Shell project from
[Operating Systems: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/).

## Overview

Enclosed is an implemtation of a *command line interpreter (CLI)* or,
as it is more commonly known, a *shell*. The shell should operate in this
basic way: when you type in a command (in response to its prompt), the shell
creates a child process that executes the command you entered and then prompts
for more user input when it has finished.

## Running the Program Environment

A `Makefile` is provided that will perform the following:

  * **compile** This will compile the `lsh.c` file using gcc.
  * **doc** Rebuild the README.md file that will rebuild as a PDF file.
  * **test** will run a set of unit tests against the program if you are just doing a normal make file compile.
  * **testJetbrains** If you are doing development in CLion (by JetBrains) then use this. The test script provides one extra step and copies the executable from the development subdirectory and places it at the level of the test script.
  * **clean** This will remove the executable and any modified files.

## Program Specifications

### Basic Shell: `lsh`

The shell can be invoked with either no arguments or a single argument;
anything else is an error. Here is the no-argument way:

```
prompt> ./lsh
lsh> 
```

The mode above is called *interactive* mode, and allows the user to type
commands directly. The shell also supports a *batch mode*, which instead reads
input from a batch file and executes commands from therein. Here is how you
run the shell with a batch file named `batch.txt`:

```
prompt> ./lsh batch.txt
```

The shell is very simple (conceptually): it runs in a while loop, repeatedly
asking for input to tell it what command to execute. It then executes that
command. The loop continues indefinitely, until the user types the built-in
command `exit`, at which point it exits. That's it!

## Structure

### Paths

**Important:** Note that the shell itself does not *implement* `ls` or other
commands (except built-ins). All it does is find those executables in one of
the directories specified by `path` and create a new process to run them.

### Built-in Commands

* `exit`: When the user types `exit`, the shell will simply call the `exit`
  system call with 0 as a parameter. 

* `cd`: `cd` always take one argument, changing directory.

* `path`: The `path` command takes 0 or more arguments, with each argument
  separated by whitespace from the others. A typical usage would be like this:
  `lsh> path /bin /usr/bin`, which would add `/bin` and `/usr/bin` to the
  search path of the shell. 

### Redirection

The shell also supports redirection and parallel commands through `>` and
a `&` character, similarly to the bash shell. The exact format of redirection 
is a command (and possibly some arguments)followed by the redirection symbol 
followed by a filename. Multiple redirection operators or multiple files to 
the right of the redirection sign are errors. If the `output` file exists
before you run some program, the shell will simplyoverwrite it (after truncating it).
an example for running parallel commands is as follows:

```
lsh> cmd1 & cmd2 args1 args2 & cmd3 args1
```

### Program Errors

**The one and only error message.** This will print one and only error
message whenever encountering an error of any type:

```
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
```



