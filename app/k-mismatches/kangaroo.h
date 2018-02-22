#pragma once
#include "utility/array.h"
#include "utility/mismatches.h"
#include "utility/string.h"

Array<Mismatches> kangaroo(unsigned k, const String& P, const String& T, const Array<bool>& skip);
Array<Mismatches> kangaroo(unsigned k, const String& P, const String& T);
