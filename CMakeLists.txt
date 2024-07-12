cmake_minimum_required(VERSION 3.0)

project(
        CompArchLab3
        VERSION 1.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
        CompArchLab3
        main.cpp
        bin/ASMtoMC.cpp
        bin/ASMtoMC.hpp
        src/Cache.hpp
        src/CacheUtils.hpp
        src/Memory.hpp
        src/MemoryUtils.hpp
        src/EmulatorUtils.hpp
        src/Emulator.cpp
        src/Emulator.hpp
)