find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module NumPy)

add_subdirectory(external/pybind11 EXCLUDE_FROM_ALL)
add_subdirectory(src)
