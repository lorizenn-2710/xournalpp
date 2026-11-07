// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "math/TeachingMath.h"
class Control;
namespace teaching {
// Both UI and integration probe call the same generator + atomic insertion path.
void insert(Control*, const Request&);
void showTools(Control*);
}  // namespace teaching
