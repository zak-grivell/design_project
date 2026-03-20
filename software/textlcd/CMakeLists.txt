# Copyright (c) 2026 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(textlcd STATIC
    TextLCD.cpp
)

target_include_directories(textlcd PUBLIC .)
target_link_libraries(textlcd PUBLIC mbed-os)
