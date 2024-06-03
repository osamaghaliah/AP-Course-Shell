<h1 align="center"> Shell </h1>

This is a simple shell program implemented in C. It provides basic functionality to handle shell commands, manage environment variables, and execute programs.

## Files

- **shell.c**: _Contains the main implementation of the shell program, including command parsing, execution, and handling environment variables._
- **shell.h**: _Header file containing declarations and macros used in `shell.c`._
- **Makefile**: _A makefile to automate the build process of the shell program._

## Features

- **Command execution:** _Supports execution of shell commands and programs._
- **Environment variable management:** _Allows setting and retrieving environment variables._
- **Command history:** _Maintains a history of executed commands._
- **Input handling:** _Supports reading input from the user._

## How to Build

To build the shell program, you need to have GCC installed on your system. Follow the steps below to compile and run the shell:

- _**Open a terminal** and navigate to the directory containing the source files (`shell.c`, `shell.h`, and `Makefile`)._
- _**Build the program** by running the following command:_
   ```
   make
   ```

## How to Run

After building the program, you can run the shell by executing the following command in the terminal:

```
make run
```

Alternatively, you can run the compiled executable directly:

```
./shell
```

## Cleaning Up

To clean up the build files and the executable, run the following command:

```
make clean
```

This will remove all object files and the executable, leaving the source files intact.


## Compiling, Running & Commanding (3 in 1)

Here is a whole usage progress of the shell - the moment it started to compile till we are done using it:

```
Compiling shell.c into an object file...
gcc -Wall -Wextra -g -c shell.c -o shell.o
Compilation of shell.c complete.
Linking object files to form the essence of shell...
gcc -Wall -Wextra -g -o shell shell.o
Creation of shell complete.
Running shell...
./shell
hello: date >> myfile    
hello: cat myfile
Sun 02 Jun 2024 04:17:22 PM IDT
hello: date -u >> myfile
hello: cat myfile
Sun 02 Jun 2024 04:17:22 PM IDT
Sun 02 Jun 2024 01:17:32 PM UTC
hello: wc -l < myfile
2
hello: prompt = hi:
hi: mkdir mydir
hi: cd mydir
Directory has been successfully changed to mydir
hi: pwd
/home/osama/Desktop/322509357_316203405/mydir
hi: touch file1 file2 file3
hi: ls
file1  file2  file3
hi: !!
file1  file2  file3
hi: echo abc xyz
abc xyz 
hi: ls
file1  file2  file3
hi: echo $?
0
hi: ls no_such_file
ls: cannot access 'no_such_file': No such file or directory
hi: ls no_such_file 2> file
hi: ^C
You typed Control-C!
hi: cat > colors.txt
blue
black
red
red
green
blue
green
red
red
blue
hi: cat colors.txt
blue
black
red
red
green
blue
green
red
red
blue
hi: cat colors.txt | cat | cat | cat
blue
black
red
red
green
blue
green
red
red
blue
hi: sort colors.txt | uniq -c | sort -r | head -3
      4 red
      3 blue
      2 green
hi: quit
```
---------------------------------------------------------------------------------------------------

## Submitters:

- **Osama Ghaliah | ID: 322509357**
- **Ibrahim Horani | ID: 316203405**
