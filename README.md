# LLVM String Obfuscator
Hide all your precious strings without touching a single line of your source code, powered by LLVM bytecode manipulation.

![image](https://github.com/tsarpaul/llvm-string-obfuscator/blob/master/image.png)

## How-To
1. Install llvm
```sudo apt install llvm```

2. Build the StringObfuscator library:
```
mkdir build
cd build
cmake ..
make
```

4. Generate LLVM bytecode from your binary and run the StringObfuscator pass on it:
```
clang -emit-llvm hello.c -c -o hello.bc
opt -load-pass-plugin=<string-obfuscator-dir>/build/lib/LLVMStringObfuscator.so -passes="string-obfuscator-pass" < hello.bc -o out.bc
llc out.bc -o out.s
clang -static out.s -o out
```
5. Leave a like :)
