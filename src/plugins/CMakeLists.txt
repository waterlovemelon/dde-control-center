# Find the library
find_package(PkgConfig REQUIRED)
find_package(DtkWidget REQUIRED)
find_package(DtkGui REQUIRED)
find_package(DtkCMake REQUIRED)
find_package(PolkitQt5-1)
find_package(Qt5 COMPONENTS
    Core
    Widgets
    Concurrent
    X11Extras
    DBus
    Multimedia
    Svg
    WaylandClient
REQUIRED)
if(${Qt5_VERSION} VERSION_GREATER "5.15.1")
    find_package(Qt5 COMPONENTS
        XkbCommonSupport
        REQUIRED)
endif()
find_package(KF5Wayland QUIET)
pkg_check_modules(WaylandClient REQUIRED wayland-client)
pkg_check_modules(XCB_EWMH REQUIRED xcb-ewmh x11 xext xkbcommon)
pkg_check_modules(DFrameworkDBus REQUIRED dframeworkdbus)
pkg_check_modules(QGSettings REQUIRED gsettings-qt)
pkg_check_modules(Gio-2.0 REQUIRED gio-2.0)

set(DEFINED_LIST
DISABLE_SYS_UPDAT
DISABLE_SYS_UPDATE_SOURCE_CHECK
DISABLE_SYS_UPDATE_MIRRORS
)

foreach(LETTER ${DEFINED_LIST})
    if(DEFINED ${LETTER})
        add_definitions(-D${LETTER})
    endif()
endforeach()

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "sw_64")
    set(BUILD_MACRO
        ${BUILD_MACRO}
        DISABLE_SYS_UPDATE_SOURCE_CHECK
        DISABLE_SYS_UPDATE_MIRRORS
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee")
endif()

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "mips64")
    set(BUILD_MACRO
        ${BUILD_MACRO}
        DISABLE_SYS_UPDATE_SOURCE_CHECK
    )
endif()

## 公用文件
file(GLOB_RECURSE Public_Files
    "../frame/window/gsettingwatcher.cpp"
    "../frame/window/dconfigwatcher.cpp"
    "../frame/window/utils.h"
)

#--------------------------plugin-Update--------------------------
if (NOT DISABLE_SYS_UPDATE)
    message("Complie update plugin")
    set(Plugin_Update_Name dcc-update-plugin)
    file(GLOB_RECURSE Plugin_Update_SRCS
        "update/*.cpp"
        "update/module/update.qrc"
        "update/window/dccv20update.qrc"
        ${Public_Files}
    )

    add_library(${Plugin_Update_Name} SHARED
        ${Plugin_Update_SRCS}
    )

    set(Plugin_Update_Includes
        ${DtkWidget_INCLUDE_DIRS}
        Qt5::Widgets
        Qt5::DBus
        Qt5::Concurrent
        ${QGSettings_INCLUDE_DIRS}
        ${DFrameworkDBus_INCLUDE_DIRS}
        update/module
        update/window
        update/window/widgets
        ../../include
        ../frame
    )

    set(Plugin_Update_Libraries
        dccwidgets
        ${DtkWidget_LIBRARIES}
        Qt5::Widgets
        Qt5::DBus
        Qt5::Concurrent
        ${Dcc_Widgets_Name}
        ${Qt5Multimedia_LIBRARIES}
        ${DFrameworkDBus_LIBRARIES}
        ${QGSettings_LIBRARIES}
    )

    target_include_directories(${Plugin_Update_Name} PUBLIC
        ${Plugin_Update_Includes}
    )

    target_link_libraries(${Plugin_Update_Name} PRIVATE
        ${Plugin_Update_Libraries}
    )

    install(TARGETS ${Plugin_Update_Name} DESTINATION lib/dde-control-center/modules)
endif()


