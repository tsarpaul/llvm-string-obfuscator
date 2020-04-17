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

3. Generate LLVM bytecode from your binary and run the StringObfuscator pass on it:
```
clang -emit-llvm hello.c -c -o hello.bc
opt -load-pass-plugin=<string-obfuscator-dir>/build/lib/LLVMStringObfuscator.so -passes="string-obfuscator-pass" < hello.bc -o out.bc
llc out.bc -o out.s
clang -static out.s -o out
```

For Rust:
```
rustc hello.rs --emit=llvm-bc -o hellor.bc
opt -load-pass-plugin=/home/polka/Documents/Code/llvm-string-obfuscator/build/StringObfuscator/libLLVMStringObfuscator.so -passes="string-obfuscator-pass" < ~/Documents/Code/llvm-string-obfuscator/examples/hellor.bc -o out.bc
llc out.bc -o out.s
ruststd=$(basename $(ls /usr/lib/rustlib/x86_64-unknown-linux-gnu/lib/libstd*.so) | sed 's/lib//g' | sed 's/\.so//g')
clang out.s -L/usr/lib/rustlib/x86_64-unknown-linux-gnu/lib -l$ruststd -o out
```
4. Leave a like :)
