# find all .cpp file
file(GLOB_RECURSE SOURCES "*/*.cpp")

# compile
foreach(SOURCE ${SOURCES})
    get_filename_component(FILENAME ${SOURCE} NAME_WE)
    add_executable(${FILENAME} ${SOURCE})
	target_link_libraries(
        ${FILENAME}
        PRIVATE dv::processing
        		dv::toolkit
        		${libs})
	target_compile_options(
		${FILENAME} 
		PRIVATE "-Werror=deprecated-declarations")
endforeach()
