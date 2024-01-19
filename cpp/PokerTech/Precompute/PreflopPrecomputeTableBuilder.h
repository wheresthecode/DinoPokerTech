#pragma once
#include <string>
#include <functional>

// For simplicity the precompute table builder is located within the PokerTech project.
// This makes it so we can test everything with a single test suite.
typedef std::function<void(int, int)> PrecomputeProgressCallback;
bool PrecomputeCalculateProcess(const std::string &outputPrecomputeFile, const std::string &cacheDirectory, int bitsPerValue, int rankCount, PrecomputeProgressCallback callback);
