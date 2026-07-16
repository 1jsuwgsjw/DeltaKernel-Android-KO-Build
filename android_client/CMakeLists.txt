# CTF|认证
cmake_minimum_required(VERSION 3.22)
project(delta_android_client LANGUAGES CXX)
add_executable(delta_client delta_client.cpp)
target_compile_features(delta_client PRIVATE cxx_std_17)

