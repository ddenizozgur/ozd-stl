cmake_minimum_required(VERSION 3.20)
project(ozd-stl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OZD_COMMON_COMPILE_OPTIONS
    /W4
    /EHs-c-        # -fno-exceptions
    /GR-           # -fno-rtti
)

set(OZD_COMMON_LINK_OPTIONS
    /NODEFAULTLIB:libcpmt.lib
    /NODEFAULTLIB:msvcprt.lib
)

function(add_ozd_example TARGET_NAME OUTPUT_NAME SOURCE_DIR)
    add_executable(${TARGET_NAME} ${SOURCE_DIR}/main.cpp)

    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}
    )

    target_compile_options(${TARGET_NAME} PRIVATE
        ${OZD_COMMON_COMPILE_OPTIONS}
    )

    target_link_options(${TARGET_NAME} PRIVATE
        ${OZD_COMMON_LINK_OPTIONS}
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/Bin"
        OUTPUT_NAME "${OUTPUT_NAME}"
    )

    # Optional: make the VS debugger start in the source folder so that
    # relative paths like ../Assets also work when debugging.
    set_target_properties(${TARGET_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}"
    )
endfunction()

# DX11Example → DX11Example.exe (needs ../Assets and ../Shaders)
add_ozd_example(DX11Example DX11Example DX11Example)

# zExample → Example.exe (no asset dependencies)
add_ozd_example(zExample Example zExample)