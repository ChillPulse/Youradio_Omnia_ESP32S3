// Copyright 2026 Kevin Ahrendt
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/// @file alloc.h
/// @brief Memory allocation macros for FLAC decoder buffers
///
/// Provides FLAC_MALLOC and FLAC_FREE macros with ESP-IDF PSRAM support and
/// configurable memory placement. Can be overridden via compiler defines.

#pragma once

// Pull user build options (Arduino include path contains yoRadio/)
#if __has_include("myoptions.h")
#include "myoptions.h"
#endif

// Include necessary headers based on platform
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#else
#include <cstdlib>
#endif

// ============================================================================
// Memory Allocation Customization
// ============================================================================

// Define memory allocation macros - can be overridden via compiler defines
// Example: -DFLAC_MALLOC=my_malloc -DFLAC_FREE=my_free
// Both FLAC_MALLOC and FLAC_FREE should be defined together as a pair.

#if defined(FLAC_MALLOC) != defined(FLAC_FREE)
#error "FLAC_MALLOC and FLAC_FREE must both be defined or both left undefined."
#endif

#ifndef FLAC_MALLOC
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
#if defined(MICRO_FLAC_MEMORY_PREFER_PSRAM)
#define FLAC_MALLOC(sz)                                                  \
    heap_caps_malloc_prefer(sz, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT, \
                            MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT)
#elif defined(MICRO_FLAC_MEMORY_PREFER_INTERNAL)
#define FLAC_MALLOC(sz)                                                    \
    heap_caps_malloc_prefer(sz, 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT, \
                            MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT)
#elif defined(MICRO_FLAC_MEMORY_PSRAM_ONLY)
#define FLAC_MALLOC(sz) heap_caps_malloc(sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT)
#elif defined(MICRO_FLAC_MEMORY_INTERNAL_ONLY)
#define FLAC_MALLOC(sz) heap_caps_malloc(sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT)
#else
// Default: prefer PSRAM, fall back to internal
#define FLAC_MALLOC(sz)                                                  \
    heap_caps_malloc_prefer(sz, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT, \
                            MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT)
#endif
#else
#define FLAC_MALLOC(sz) malloc(sz)
#endif
#endif

#ifndef FLAC_FREE
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
#define FLAC_FREE(p) heap_caps_free(p)
#else
#define FLAC_FREE(p) free(p)
#endif
#endif
