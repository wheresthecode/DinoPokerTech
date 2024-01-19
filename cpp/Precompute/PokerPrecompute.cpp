#include <iostream>
#include "../PokerTech/PokerTypes.h"
#include "../PokerTech/Precompute/PreflopPrecomputeTableBuilder.h"
#include <filesystem>
#include <sstream>

using namespace std;
namespace fs = std::filesystem;

// This is the main entry point for the precompute table generator.
// The output directory is specified in the makefile and is meant to be run on the same machine
// it was built on.
int main(int argc, char *args[])
{
	unsigned int rankCount = kRankCardCount;
	unsigned int bitsPerValue = 16;
	fs::path outDir = PRECOMPUTE_DIRECTORY;
	std::ostringstream filenameBuilder;
	filenameBuilder << "preflop_precompute_" << bitsPerValue << "_" << rankCount << ".bin";
	std::string outFilename = filenameBuilder.str();
	fs::path fullPath = outDir;
	fullPath /= filenameBuilder.str();

	if (!outDir.empty() && !fs::is_directory(outDir))
	{
		cerr << "Directory " << outDir << "does not exist" << endl;
		return 1;
	}

	cout << "Generating precompute table at " << fullPath << endl;

	std::function<void(int, int)> callback = [](int complete, int total)
	{
		std::cout << complete << "/" << total << '\r';
	};

	bool result = PrecomputeCalculateProcess(fullPath.generic_string(), "PrecomputeCache", 16, rankCount, callback);

	if (!result)
	{
		std::cerr << "Failed to generate preocmpute table" << endl;
		return 1;
	}
	return 0;
}