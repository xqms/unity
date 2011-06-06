cmake_minimum_required(VERSION 2.8)

project(unity)

find_package(X11 REQUIRED)
include_directories(${X11_X11_INCLUDE_PATH})

set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")

add_executable(unity
	main.cpp
	utils.cpp
	grabwindow.cpp
	edgewindow.cpp
	client.cpp
	injector.cpp
)

target_link_libraries(unity
	${X11_X11_LIB}
	${X11_XTest_LIB}
)
