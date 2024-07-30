cmake_minimum_required(VERSION 3.5)

project(sAkura_Chat)

#解决奇数个数中文在msvc上无法编译utf-8不带bom的代码文件
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

set(CMAKE_CXX_STANDARD 17)

# set(HELLOIMGUI_BUILD_DEMOS 1)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(custom_app_png_icon assets/zh.jpg)
set(HELLOIMGUI_HAS_OPENGL3 ON)
set(HELLOIMGUI_USE_GLFW3 ON)
add_subdirectory(external/hello_imgui)

find_package(freetype CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread json)
find_package(unofficial-lunasvg CONFIG REQUIRED)
find_package(CURL REQUIRED)


#客户端
hello_imgui_add_app(client_sAkura_Chat client.cpp client.hpp extern.cpp extern.hpp)
target_link_libraries(client_sAkura_Chat PRIVATE Boost::system Boost::thread Boost::json)

#服务端
add_executable(server_sAkura_Chat server.cpp server.hpp)
target_link_libraries(server_sAkura_Chat PRIVATE Boost::system Boost::thread Boost::json CURL::libcurl)


# $env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
