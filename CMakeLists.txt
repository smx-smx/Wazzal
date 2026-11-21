cmake_minimum_required(VERSION 3.15)
project(Wazzal LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Wazzal is a Windows DirectX 8 game and can only be built on Windows.")
endif()

enable_language(RC)

set(WAZZAL_SOURCES
    Stars.cpp
    func.cpp
    dsutil.cpp
    dxutil.cpp
    StdAfx.cpp
    compat/d3dx8.cpp
    Stars.rc
)

add_executable(Wazzal WIN32 ${WAZZAL_SOURCES})

target_include_directories(Wazzal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/compat
)

target_compile_definitions(Wazzal PRIVATE WIN32 _WINDOWS)

set_target_properties(Wazzal PROPERTIES
    CXX_STANDARD 98
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
    OUTPUT_NAME "Wazzal"
)

if(MSVC)
    # The Microsoft toolchain needs the DirectX 8.1 SDK for d3d8.h/d3dx8.
    # Point DXSDK_DIR at a DirectX 8.1 SDK installation to use MSVC.
    if(DEFINED ENV{DXSDK_DIR} AND EXISTS "$ENV{DXSDK_DIR}/Include/d3d8.h")
        target_include_directories(Wazzal PRIVATE "$ENV{DXSDK_DIR}/Include")
        target_link_directories(Wazzal PRIVATE "$ENV{DXSDK_DIR}/Lib/x86")
    else()
        message(FATAL_ERROR
            "MSVC requires the DirectX 8.1 SDK (d3d8.h/d3dx8.h). "
            "Either install it and set DXSDK_DIR, or build with MinGW-w64 i686.")
    endif()
    target_compile_options(Wazzal PRIVATE /EHsc /wd4996)
    target_link_libraries(Wazzal PRIVATE d3d8.lib dxguid.lib dinput8.lib dsound.lib
        strmiids.lib quartz.lib ole32.lib winmm.lib)
else()
    target_compile_options(Wazzal PRIVATE -fpermissive -Wno-write-strings)
    target_link_options(Wazzal PRIVATE -static -static-libgcc -static-libstdc++)
    target_link_libraries(Wazzal PRIVATE
        d3d8 dxguid dxerr8 dinput8 dsound strmiids quartz ole32 winmm uuid)
endif()

install(TARGETS Wazzal RUNTIME DESTINATION .)
