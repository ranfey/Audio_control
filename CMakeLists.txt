cmake_minimum_required(VERSION 3.5)

project(Audio_control LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(RESOURCES resources.qrc)
# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(Qt5 COMPONENTS Widgets Core Gui REQUIRED WinExtras)

if(ANDROID)
  add_library(Audio_control SHARED
    WIN32 
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    maintray.cpp
    maintray.h
    icon.rc
  )
else()
  add_executable(Audio_control
    WIN32 
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    maintray.cpp
    maintray.h
    ${RESOURCES}
    icon.rc
  )
endif()

target_link_libraries(Audio_control PRIVATE Version Qt5::Core Qt5::Gui Qt5::Widgets Qt5::WinExtras)
