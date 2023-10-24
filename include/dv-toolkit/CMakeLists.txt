# Create an interface library
add_library(toolkit INTERFACE)
add_library(dv::toolkit ALIAS toolkit)

# Specifies include directories
target_include_directories(
    toolkit
    INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
	toolkit
	INTERFACE ${OpenCV_LIBS}
			  dv::processing
              Eigen3::Eigen
)
