# Dependencies

For now in addition to the main programming language in which compiler is being developed below are our dependencies:
- **nasm**: To assemble compiler output and generate object file
- **lld**: (From LLVM) to link object files and produce executables

We picked `lld` over GNU's `ld` because it is [faster](https://lld.llvm.org/), although it has a large volume of dependencies including llvmlib.

If we decide to produce self-sufficient compiler that does not need people install dependencies we can use `ldd` command to 
find out dependencies of each of these and include them in the compiler distributables.

Each of above items has some runtime dependencies (`.so` files), that need to be there if someone wants to run the compiler.

For now, in order to set up the compiler you need to:

1. Clone this repository
2. Install nasm via `sudo apt-get install nasm`
3. Install lld via `sudo apt-get install lld`
