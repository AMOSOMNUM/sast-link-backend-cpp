cmake_minimum_required(VERSION 3.14)

project(SAST_Link LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Sql HttpServer)

file(GLOB_RECURSE SOURCE_FILES
    ./src/*.cpp
)

file(GLOB_RECURSE HEADER_FILES
    ./include/*.h
    ./include/*.hpp
)

qt_add_resources(QRC_FILE sast_link.qrc)

add_executable(SAST_Link
    main.cpp
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${QRC_FILE}
)

target_include_directories(SAST_Link PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(SAST_Link PRIVATE Qt6::Core Qt6::Sql Qt6::HttpServer)

install(TARGETS SAST_Link
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
