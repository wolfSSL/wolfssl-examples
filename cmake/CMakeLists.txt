CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
project(wolfssl-example)

message("Example cmake project including wolfSSL and using user_settings.h")

# add global define to include user_settings.h
add_compile_definitions(WOLFSSL_USER_SETTINGS)

set(BUILD_SHARED_LIBS OFF)
set(WOLFSSL_EXAMPLES OFF)
set(WOLFSSL_CRYPT_TESTS OFF)
set(WOLFSSL_USER_SETTINGS ON)

if (CONFIG_BIG_ENDIAN)
    set(CMAKE_C_BYTE_ORDER    BIG_ENDIAN)
    set(CMAKE_CXX_BYTE_OREDER BIG_ENDIAN)
else ()
    set(CMAKE_C_BYTE_ORDER    LITTLE_ENDIAN)
    set(CMAKE_CXX_BYTE_OREDER LITTLE_ENDIAN)
endif()

include_directories(include)
add_subdirectory(wolfssl)

target_link_libraries(wolfssl PRIVATE
)

# add in our application
add_executable(hash myApp.c)
target_link_libraries(hash wolfssl)

