cmake_minimum_required(VERSION 2.8)

project(unity)

find_package(Qt4 4.6 COMPONENTS QtCore QtNetwork QtGui REQUIRED)
include(${QT_USE_FILE})

find_package(X11 REQUIRED)
include_directories(${X11_X11_INCLUDE_PATH})

add_executable(unity
	main.cpp
	utils.cpp
	grabwindow.cpp
	edgewindow.cpp
	client.cpp
)

target_link_libraries(unity
	${QT_LIBRARIES}
	${X11_X11_LIB}
	${X11_XTest_LIB}
)
