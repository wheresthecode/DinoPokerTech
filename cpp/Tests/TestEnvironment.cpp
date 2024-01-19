#include "pch.h"
#include "../PokerTech/PokerTech.h"

class Environment : public ::testing::Environment 
{
public:
	~Environment() override {}

	// Override this to define how to set up the environment.
	void SetUp() override { InitializePokerTech(); }

	// Override this to define how to tear down the environment.
	void TearDown() override { ShutdownPokerTech(); }
};

testing::Environment* const foo_env = testing::AddGlobalTestEnvironment(new Environment);