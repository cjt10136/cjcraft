#pragma once

#include "llvm/SmallVector.h"

#include <vector>

#ifdef NDEBUG
template<typename T, unsigned N = 16>
using Vector = llvm_vecsmall::SmallVector<T, N>;
#else
template<typename T>
using Vector = std::vector<T>;
#endif

template<typename T>
using VectorImpl = llvm_vecsmall::SmallVectorImpl<T>;
