find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

add_subdirectory(external/pybind11)
add_subdirectory(src)
