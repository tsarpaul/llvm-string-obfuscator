# LLVM String Obfuscator
Hide all your precious strings without touching a single line of your source code, powered by LLVM bytecode manipulation.

![image](https://github.com/tsarpaul/llvm-string-obfuscator/blob/master/image.png)

## How-To
1. Download and compile LLVM, you can follow this guide:
https://llvm.org/docs/GettingStarted.html
2. Copy StringObfuscator to the right LLVM directory and setup cmake:
```
cp -r StringObfuscator <llvm-dir>/llvm/lib/Transforms/StringObfuscator
echo add_subdirectory(StringObfuscator) >> <llvm-dir>/llvm/lib/Transforms/CMakeLists.txt
```
3. Run ```make``` in the LLVM build directory. This will only compile the new pass if you have already compiled LLVM.
4. Generate LLVM bytecode from your binary and run the StringObfuscator pass on it:
```
clang -emit-llvm hello.c -c -o hello.bc
opt -load-pass-plugin=<llvm-dir>/build/lib/LLVMStringObfuscator.so -passes="string-obfuscator-pass" < hello.bc -o out.bc
llc out.bc -o out.s
clang -static out.s -o out
```
5. Leave a like :)
