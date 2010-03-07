cmake_minimum_required (VERSION 2.8)
project (evtouch_calibrator) 

INCLUDE ( FindX11 )
set(LIBS ${LIBS} ${X11_LIBRARIES})
include_directories(${X11_INCLUDE_DIRS})

add_executable (evtouch_calibrator calibrator.c)
target_link_libraries(evtouch_calibrator ${LIBS})

INSTALL(TARGETS evtouch_calibrator DESTINATION bin )
