set(HOST_PREFIX riscv64-elf-)
add_custom_target(chocopy_stdlib ALL COMMAND
        ${HOST_PREFIX}gcc -march=rv32imac -mabi=ilp32 -std=gnu11 ${PROJECT_SOURCE_DIR}/src/cgen/stdlib/stdlib.c -c -o ${PROJECT_BINARY_DIR}/src/cgen/stdlib/chocopy_stdlib.o -I/usr/riscv64-linux-gnu/include/
        && ${HOST_PREFIX}ar rcs ${PROJECT_BINARY_DIR}/libchocopy_stdlib.a ${PROJECT_BINARY_DIR}/src/cgen/stdlib/chocopy_stdlib.o)
