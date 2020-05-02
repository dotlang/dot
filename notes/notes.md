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


Another option was to use golang's toolchain (link, asm, ...) but they are not standard assembler and are too tightly coupled to the way go compiler workd (quote: `The problem is that the Go compiler does not generate actual assembly. It generates something that looks a lot like assembly but contains a lot of pseudo-instructions that will be expanded by the linker.` https://stackoverflow.com/questions/23789951/easy-to-read-golang-assembly-output)

Idea: As discussed [here](https://docs.google.com/document/d/1D13QhciikbdLtaI67U6Ble5d_1nsI4befEd6_k1z91U/view) it may be worth having our own object file format. In this case we can have some optimizations to have a better linker. TinyCC has its own assembler and linker. So does golang.
