# kikuyu

Kikuyu is a basic NMOS 6502 Emulator implemented in C.

# Features
- Lightweight, cycle-accurate NMOS 6502 emulator in C99
- Supports all 151 legal NMOS 6502 opcodes
- Can be compiled for GUI or CLI modes
- Includes minimal test suite for opcodes
- Includes a debugger

# Usage


## Emulator


### Building
Kikuyu can be run in CLI or GUI mode. Each mode is serviced through two unique executables that you must compile.

In the root directory of the project, compile and run the emulator in CLI mode with,

```
make cli
./kikuyu_cli path_to_bin
```
or for GUI mode,

```
make gui
./kikuyu_gui path_to_bin
```

where `path_to_bin` is some 6502 assembled binary the emulator runs.

**NOTE**: GUI mode requires [Raylib](https://github.com/raysan5/raylib) libraries are installed with your system's software package manager.


### Passing Arguments
You can view valid command-line arguments for the program by specifying the `--help` command when starting the CLI/GUI variants. For example,

```
./kikuyu_gui --help
```


### Cleaning
You can clean any object/executable files made during the compilation with,

```
make clean
```

## Opcode Test Suite

Compile the test suite and run all tests with,

```
make test
./run_tests
```

The `./run_tests` script runs JSON tests for all opcodes when no arguments are specified. Opcodes (in hexadecimal) can be specified as arguments to perform specific opcode tests only. For example,

```
./run_tests a2 a0
```

would test `LDX #` and `LDY #` in that order.

Test cases are pulled from the [65x02 Processor Tests](https://github.com/SingleStepTests/65x02) repository, which takes up some 1.2GiB. You can clean this out with,

```
make clean-tests
```

# Libraries Used
- [Raylib](https://github.com/raysan5/raylib)
  - **Must be installed manually**
  - Used for GUI rendering
- [Raygui](https://github.com/raysan5/raygui)
  - Packaged with source files
  - Used for GUI rendering   
- [cJSON](https://github.com/DaveGamble/cJSON)
  - Packaged with source files
  - Used for parsing JSON test cases

# References

- [6502 Instruction Set](https://www.masswerk.at/6502/6502_instruction_set.html)
  - Contains details about the 6502 ISA
  - Explains addressing modes, register purposes, memory, etc.
- [65x02 Processor Tests](https://github.com/SingleStepTests/65x02)
  - Language agnostic JSON test cases for each 6502 opcode
  - Details exact data/address bus and R/W statuses per clock cycle
- [6502_cpu.txt](https://www.nesdev.org/6502_cpu.txt)
  - Cycle-by-cycle breakdown of each instruction/addressing mode
