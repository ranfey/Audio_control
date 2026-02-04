cmake_minimum_required(VERSION 3.5)

project(Audio_control LANGUAGES CXX)

#静态编译用
# set(CMAKE_PREFIX_PATH "C:/Qt/Qt5.14.2staticmin")
# if(MSVC)
#     # Use the static C library for all build types
#     foreach(var
#         CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
#         CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
#         CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
#         CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
#       )
#       if(${var} MATCHES "/MD")
#         string(REGEX REPLACE "/MD" "/MT" ${var} "${${var}}")
#       endif()
#     endforeach()
# endif(MSVC)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(RESOURCES resources.qrc)

find_package(Qt5 COMPONENTS Widgets Core Gui REQUIRED WinExtras)


add_executable(Audio_control
  WIN32 
  main.cpp
  src/mainwindow.cpp
  src/mainwindow.h
  src/mainwindow.ui
  src/maintray.cpp
  src/maintray.h
  src/audiocontroller.cpp
  src/audiocontroller.h
  src/sessionrow.cpp
  src/sessionrow.h
  ${RESOURCES}
  icon.rc
)


if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
endif()

target_link_libraries(Audio_control PRIVATE Version Qt5::Core Qt5::Gui Qt5::Widgets Qt5::WinExtras)
