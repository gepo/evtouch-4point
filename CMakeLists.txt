cmake_minimum_required (VERSION 2.8)
project (evtouch_calibrator) 

INCLUDE ( FindX11 )
set(LIBS ${LIBS} ${X11_LIBRARIES})
include_directories(${X11_INCLUDE_DIRS})

if (NOT DEFINED DISABLE_HAL)
    find_path ( HAL_INCLUDE_PATHS NAMES libhal.h
        PATH_SUFFIXES hal 
    )
    find_path ( DBUS_INCLUDE_PATHS NAMES dbus/dbus.h
        PATH_SUFFIXES dbus-1.0)

    include_directories(${HAL_INCLUDE_PATHS}
                        ${DBUS_INCLUDE_PATHS})
#hack path to hal includes in Ubuntu
    include_directories(/usr/lib/dbus-1.0/include)

    find_library ( HAL_LIBRARY hal )
#find_library ( DBUS_LIBRARY dbus )
    set(LIBS ${LIBS} ${HAL_LIBRARY})# ${DBUS_LIBRARY})
else()
    add_definitions(-DDISABLE_HAL)
endif()

add_executable (evtouch_calibrator calibrator.c)
target_link_libraries(evtouch_calibrator ${LIBS})

INSTALL(TARGETS evtouch_calibrator DESTINATION bin )
