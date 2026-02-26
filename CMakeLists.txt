cmake_minimum_required(VERSION 3.10)

project(nvflux VERSION 1.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wformat-security)
endif()

# Include directories
include_directories(include)

# Main executable
add_executable(nvflux 
    src/main.c
    src/nvflux.c
    src/hw.c
    src/state.c
)
target_compile_definitions(nvflux PRIVATE NVFLUX_VERSION="${PROJECT_VERSION}")

# Tests (enabled by default)
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()
    
    add_executable(test_nvflux 
        tests/test_nvflux.c 
        src/nvflux.c
        src/hw.c
        src/state.c
    )
    target_include_directories(test_nvflux PRIVATE include)
    
    add_test(
        NAME nvflux_unit_tests
        COMMAND test_nvflux
    )
    
    # Helpful test output
    set_tests_properties(nvflux_unit_tests PROPERTIES
        FAIL_REGULAR_EXPRESSION "FAIL:"
    )
endif()

# Install targets (optional, installer script handles this)
# Uncomment if you want `make install` support:
# install(TARGETS nvflux DESTINATION bin)
# install(FILES man/nvflux.1 DESTINATION share/man/man1)


