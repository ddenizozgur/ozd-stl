cmake_minimum_required(VERSION 3.20)
project(ozd-stl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(add_ozd_example TARGET_NAME OUTPUT_NAME SOURCE_DIR)
    add_executable(${TARGET_NAME} ${SOURCE_DIR}/main.cpp)

    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/Bin"
        OUTPUT_NAME "${OUTPUT_NAME}"
    )

    # Optional: make the VS debugger start in the source folder
    set_target_properties(${TARGET_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}"
    )
endfunction()

# DX11Example → DX11Example.exe
add_ozd_example(DX11Example DX11Example DX11Example)

# zExample → Example.exe
add_ozd_example(zExample Example zExample)