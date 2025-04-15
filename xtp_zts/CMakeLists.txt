cmake_minimum_required(VERSION 3.10)
project("zts")

add_definitions('-g')
add_definitions('-Wall')
add_definitions('-O3')
add_definitions('-std=c++20')
add_definitions('-Wno-div-by-zero')
add_definitions('-Wdeprecated-declarations')
add_definitions('-Wunused-variable')

# Set project version
set(ZTS_VERSION_MAJOR "0")
set(ZTS_VERSION_MINOR "1")
set(ZTS_VERSION_PATCH "0")
set(ZTS_VERSION_EXTRA "")
set(ZTS_VERSION "${ZTS_VERSION_MAJOR}.${ZTS_VERSION_MINOR}")
set(ZTS_VERSION_FULL "${ZTS_VERSION_MAJOR}.${ZTS_VERSION_MINOR}.${ZTS_VERSION_PATCH}${ZTS_VERSION_EXTRA}")

# number deci for expression
add_definitions(-DNUMBER_DECI=10000)

option(ZTS_DEBUG "build application with debug" OFF) # OFF is the default

if(ZTS_DEBUG)
    # for code  #ifdef DEBUG ... #endif
    add_compile_definitions( ZTS_DEBUG )
endif(ZTS_DEBUG) 


# Configure header
#configure_file(
#    ${PROJECT_SOURCE_DIR}/include/utility/version.h.in
#    ${PROJECT_SOURCE_DIR}/include/utility/version.h
#
#    )

option(USE_CCACHE "" OFF)                                                                                           
set(CCACHE_OPTIONS "" CACHE STRING "options for ccache")                                                            
# or cmake -DZTS_DEBUG=ON -DCMAKE_CXX_COMPILER_LAUNCHER=ccache  ../                                                      
if (USE_CCACHE)                                                                                                     
    find_program(CCACHE_PROGRAM ccache)                                                                             
    if(CCACHE_PROGRAM)                                                                                              
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")                                                          
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")                                                         
        message("Looking ccache library")                                                                  
    endif()                                                                                                 
endif()

#---------------------------
# pkg-config library
#---------------------------
find_package(PkgConfig REQUIRED)
if(PkgConfig_FOUND)
    message ("Looking PkgConfig found")
else()
    message (FATAL_ERROR "Cannot find PkgConfig: sudo apt-get install pkg-config")
endif()

#---------------------------
# librdkafka library
#---------------------------
if (DEFINED ENV{LIBRDKAFKA_INCLUDE_DIR})
    set(LIBRDKAFKA_INCLUDE_DIR $ENV{LIBRDKAFKA_INCLUDE_DIR})
else ()
    find_file(LIBRDKAFKA_HEADER
        NAMES rdkafka.h
        HINTS /usr/include/librdkafka /usr/local/include/librdkafka /opt/homebrew/include/librdkafka)

    cmake_path(GET LIBRDKAFKA_HEADER PARENT_PATH LIBRDKAFKA_INCLUDE_DIR)
    cmake_path(GET LIBRDKAFKA_INCLUDE_DIR PARENT_PATH LIBRDKAFKA_INCLUDE_DIR)
endif ()

if (DEFINED ENV{LIBRDKAFKA_LIBRARY_DIR})
    set(LIBRDKAFKA_LIBRARY_DIR $ENV{LIBRDKAFKA_LIBRARY_DIR})
else ()
    find_library(LIBRDKAFKA_LIB
        NAMES rdkafka
        HINTS /usr/lib /usr/local/lib /opt/homebrew/lib )

    cmake_path(GET LIBRDKAFKA_LIB PARENT_PATH LIBRDKAFKA_LIBRARY_DIR)
endif ()

if (EXISTS "${LIBRDKAFKA_INCLUDE_DIR}/librdkafka/rdkafka.h")
    message(STATUS "librdkafka include directory: ${LIBRDKAFKA_INCLUDE_DIR}")
else ()
    message(FATAL_ERROR "Could not find headers for librdkafka!")
endif ()

if (EXISTS "${LIBRDKAFKA_LIBRARY_DIR}/librdkafka.a" OR EXISTS "${LIBRDKAFKA_LIBRARY_DIR}/librdkafka.so" OR EXISTS "${LIBRDKAFKA_LIBRARY_DIR}/rdkafka.lib" )
    message(STATUS "librdkafka library directory: ${LIBRDKAFKA_LIBRARY_DIR}")
else ()
    message(FATAL_ERROR "Could not find library for librdkafka! apt-get install -y librdkafka++1 librdkafka-dev librdkafka1 ")
endif ()


#  apt-get install -y  libpthreadpool-dev libpthreadpool0  zlib1g-dev zlib1g
## thread
find_package(Threads REQUIRED)
if(Threads_FOUND)
    message ("Looking Threads found")
else()
    message (FATAL_ERROR "Cannot find Threads, apt-get install -y  libpthreadpool-dev libpthreadpool0")
endif()

EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )


set(LIB_EXT "so")
if( ${ARCHITECTURE} STREQUAL "x86_64" )
    message( STATUS "X86 Architecture: ${ARCHITECTURE}" )
    set(LIB_EXT "so")
else()
    message( STATUS "ARM Architecture: ${ARCHITECTURE}" )
    set(LIB_EXT "dylib")
endif()



#---------------------------
# api library
#---------------------------
find_library(ZTS_LIBRARY_QUOTE NAMES  libxtpquoteapi.${LIB_EXT}   HINTS "${PROJECT_SOURCE_DIR}/api")

if(NOT ZTS_LIBRARY_QUOTE)
    message (FATAL_ERROR "Cannot find zts")
else()
    message ("Looking zts quote found: ${ZTS_LIBRARY_QUOTE}")
endif()

find_library(ZTS_LIBRARY_TRADER NAMES  libxtptraderapi.${LIB_EXT}  HINTS "${PROJECT_SOURCE_DIR}/api")

if(NOT ZTS_LIBRARY_TRADER)
    message (FATAL_ERROR "Cannot find zts")
else()
    message ("Looking zts trader found: ${ZTS_LIBRARY_TRADER}")
endif()

if(WIN32)
    set(ADDITIONAL_FLEX_FLAGS "--wincompat")
else()
    set(ADDITIONAL_FLEX_FLAGS "")
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    )

file(GLOB SOURCES
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/kafka/*.cpp"

        "${PROJECT_SOURCE_DIR}/src/FeedPack/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/BookPack/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/Toolkit/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/utility/*.cpp"
    )

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/api
    /opt/homebrew/include
    )

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    pthread

    ${ZTS_LIBRARY_QUOTE}
    ${ZTS_LIBRARY_TRADER}

     ${LIBRDKAFKA_LIB}
    rdkafka++
    )

set(ALL_FILE "coredumps")

# # target zoo is always built
add_custom_target( clean-all 
    COMMAND echo "clean core dump files ${ALL_FILE}"
    COMMAND rm -f /tmp/coredumps/* 
    COMMAND ccache -C
    COMMAND make clean
    )
