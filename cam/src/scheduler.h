// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.h"
#include <functional>
class Scheduler {
 public:
 explicit Scheduler(Store&s):store(s){}
 void tick(const QDateTime &now,const std::function<bool(const Event&)> &dispatch);
 private: Store &store;
};
