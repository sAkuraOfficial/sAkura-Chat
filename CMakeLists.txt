cmake_minimum_required(VERSION 3.5)

project(sAkura_Chat)

add_subdirectory(external/hello_imgui)

hello_imgui_add_app(sAkura_Chat main.cpp main.hpp)
