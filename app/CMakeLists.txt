cmake_minimum_required(VERSION 3.4.1)

add_library(vkdemo
            SHARED
            src/main/cpp/main.cpp
            src/main/cpp/VkHelper.cpp)

add_library(native_app_glue
            STATIC
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

add_definitions("-DVK_USE_PLATFORM_ANDROID_KHR -DGLM_FORCE_SIZE_T_LENGTH -DGLM_FORCE_RADIANS")

set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -std=c++17")

set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_include_directories(vkdemo
                           PRIVATE
                           ${ANDROID_NDK}/sources/android/native_app_glue)

add_subdirectory(glm)

target_link_libraries(vkdemo
                      android
                      native_app_glue
                      vulkan
                      glm
                      log)