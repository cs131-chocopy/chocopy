set(SOURCE_FILES BasicBlock.cpp Constant.cpp Function.cpp GlobalVariable.cpp Instruction.cpp Module.cpp Type.cpp User.cpp Value.cpp IRprinter.cpp chocopy_lightir.cpp Class.cpp)
add_library(ir-optimizer-lib ${SOURCE_FILES})
target_link_libraries(ir-optimizer-lib parser-lib semantic-lib fmt::fmt)

add_executable(ir-optimizer ${SOURCE_FILES})
target_link_libraries(ir-optimizer parser-lib semantic-lib fmt::fmt)
target_compile_definitions(ir-optimizer PUBLIC -DPA3=1)
add_dependencies(ir-optimizer chocopy_stdlib)

include_directories(${PROJECT_SOURCE_DIR}/include/ir-optimizer)
include_directories(${PROJECT_SOURCE_DIR}/include/semantic)
include_directories(${PROJECT_SOURCE_DIR}/include/parser)
