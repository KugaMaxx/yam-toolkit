pybind11_add_module(
    _lib_toolkit
    MODULE _lib_toolkit.cpp
)

target_compile_features(
    _lib_toolkit
    PRIVATE cxx_std_20
)

target_link_libraries(
    _lib_toolkit
    PRIVATE dv::toolkit
            dv::processing
)

# set_target_properties(
#     _lib_toolkit
#     PROPERTIES
#     LIBRARY_OUTPUT_DIRECTORY
#     ${Python3_SITEARCH}/lib
# )

# install(TARGETS _lib_toolkit DESTINATION ${Python3_SITEARCH})