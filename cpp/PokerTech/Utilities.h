#pragma once

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define CASSERT_ARRAY_SIZE(a, size) static_assert(ARRAY_SIZE(a) == (int)(size));
#define ALIGN_SIZE(size, alignment) (size + ((size % alignment) != 0 ? (alignment - (size % alignment)) : 0))
#define CLAMP(v, _min, _max) (std::min(_max, std::max(v, _min)))

#ifdef _MSC_VER
#include <intrin.h>
#define bit_popcount64 __popcnt64
#elif defined(__GNUC__) || defined(__clang__)
#define bit_popcount64 __builtin_popcountll
#else
inline int bit_popcount64(uint64_t v)
{
	int r = 0;
	for (int i = 0; i < 64; i++)
		if ((((uint64_t)1 << i) & v) != 0)
			r++;
	return r;
}
#endif

bool ReadEntireFile(const char *path, std::vector<uint8_t> &outData);

bool WriteDataToFile(const char *path, const void *data, int size);
bool WriteDataToFile(const std::string &path, std::vector<uint8_t> data);

bool CreateDirectory(const char *path);