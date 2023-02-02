set(HOST_PREFIX riscv64-elf-)
add_custom_target(chocopy_stdlib ALL COMMAND
        ${HOST_PREFIX}gcc -g -Og -march=rv32imac -mabi=ilp32 ${PROJECT_SOURCE_DIR}/src/cgen/stdlib/chocopy_stdlib.S -c -o ${PROJECT_BINARY_DIR}/src/cgen/stdlib/chocopy_stdlib.o
        && ${HOST_PREFIX}gcc -g -march=rv32imac -mabi=ilp32 -Og -std=c11 -xc ${PROJECT_SOURCE_DIR}/src/cgen/stdlib/debug.c -c -o ${PROJECT_BINARY_DIR}/src/cgen/stdlib/debug.o -I/usr/riscv64-linux-gnu/include/
        && ${HOST_PREFIX}gcc -g -march=rv32imac -mabi=ilp32 -Og -std=c11 -xc ${PROJECT_SOURCE_DIR}/src/cgen/stdlib/myscanf.c -c -o ${PROJECT_BINARY_DIR}/src/cgen/stdlib/myscanf.o -I/usr/riscv64-linux-gnu/include/
        && ${HOST_PREFIX}gcc -g -march=rv32imac -mabi=ilp32 -Og -std=c11 -xc ${PROJECT_SOURCE_DIR}/src/cgen/stdlib/myprintf.c -c -o ${PROJECT_BINARY_DIR}/src/cgen/stdlib/myprintf.o -I/usr/riscv64-linux-gnu/include/
        && ${HOST_PREFIX}gcc -g -march=rv32imac -mabi=ilp32 -Og -std=c11 -xc ${PROJECT_SOURCE_DIR}/src/cgen/stdlib/strcat.c -c -o ${PROJECT_BINARY_DIR}/src/cgen/stdlib/strcat.o -I/usr/riscv64-linux-gnu/include/
        && ${HOST_PREFIX}ar rcs ${PROJECT_BINARY_DIR}/libchocopy_stdlib.a ${PROJECT_BINARY_DIR}/src/cgen/stdlib/chocopy_stdlib.o ${PROJECT_BINARY_DIR}/src/cgen/stdlib/myprintf.o ${PROJECT_BINARY_DIR}/src/cgen/stdlib/strcat.o ${PROJECT_BINARY_DIR}/src/cgen/stdlib/myscanf.o ${PROJECT_BINARY_DIR}/src/cgen/stdlib/debug.o)
add_custom_target(chocopy_stdlib_run ALL COMMAND cd ${PROJECT_SOURCE_DIR}/src/cgen/stdlib/
        && bash preprocess linux VERBATIM)
add_dependencies(chocopy_stdlib chocopy_stdlib_run)
