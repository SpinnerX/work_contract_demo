cmake_minimum_required(VERSION 3.27)
project(Starter CXX)

build_application(
    SOURCES Application.cpp

    PACKAGES
    fmt

    LINK_PACKAGES
    fmt::fmt
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/signal_tree)