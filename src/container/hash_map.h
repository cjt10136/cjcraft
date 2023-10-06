#pragma once

#include "unordered_dense/unordered_dense.h"

template<class Key, class T>
using HashMap = ankerl::unordered_dense::map<Key, T>;
