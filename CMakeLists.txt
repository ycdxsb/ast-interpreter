set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )

add_clang_executable(ast-interpreter
  ASTInterpreter.cpp
  )

target_link_libraries(ast-interpreter
  clangAST
  clangBasic
  clangFrontend
  clangTooling
  )

install(TARGETS ast-interpreter
  RUNTIME DESTINATION bin)
