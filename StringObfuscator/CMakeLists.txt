add_llvm_library( LLVMStringObfuscator MODULE
  StringObfuscator.cpp

  PLUGIN_TOOL
  opt
)
