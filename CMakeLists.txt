cmake_minimum_required(VERSION 3.10)
project(nvflux VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra)
add_compile_definitions(_POSIX_C_SOURCE=200809L)

# ── static library shared by the binary and the test suite ──────────────────
add_library(nvflux_core STATIC
    src/nvidia.c
    src/profile.c
    src/state.c
)
target_include_directories(nvflux_core PUBLIC include src)

# ── main executable ──────────────────────────────────────────────────────────
add_executable(nvflux src/main.c)
target_link_libraries(nvflux PRIVATE nvflux_core)
target_include_directories(nvflux PRIVATE include)

# ── installation rules ───────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS nvflux
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ             GROUP_EXECUTE
            WORLD_READ             WORLD_EXECUTE
            SETUID)
install(FILES man/nvflux.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        RENAME nvflux.1)

# ── tests ────────────────────────────────────────────────────────────────────
enable_testing()
add_executable(test_parse tests/test_parse.c)
target_link_libraries(test_parse PRIVATE nvflux_core)
target_include_directories(test_parse PRIVATE include src)
add_test(NAME parse_tests COMMAND test_parse)
set_tests_properties(parse_tests PROPERTIES TIMEOUT 10)
