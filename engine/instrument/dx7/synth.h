/*
 * Copyright 2012 Google Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cstdlib>
#include <cmath>

#include <iostream>
#include <algorithm>
#include <memory>
#include <string>

namespace dx7 {

const static int LG_N = 6;
const static int N = (1 << LG_N);

#define QER(n,b) ( ((float)n)/(1<<b) )

template<typename T, size_t size, size_t alignment = 16>
class AlignedBuf {
 public:
  T *get() {
    return (T *)((((intptr_t)storage_) + alignment - 1) & -alignment);
  }
 private:
  unsigned char storage_[size * sizeof(T) + alignment];
};

}
