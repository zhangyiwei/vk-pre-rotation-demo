#pragma once

#include <android/log.h>

#define LOG_TAG "VKDEMO"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ASSERT(cond)                                                                           \
    if (!(cond)) {                                                                             \
        __android_log_assert(#cond, LOG_TAG, "Error: " #cond " at " __FILE__ ":%d", __LINE__); \
    }
