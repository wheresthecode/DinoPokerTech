#include "pch.h"
#include "../PokerTech/EquityCalculator/EquityCalculator.h"
#include "../PokerTech/PokerUtilities.h"
#include "../PokerTech/PokerTech.h"
#include "Deck.h"

// Come up with various performance tests.
// Estimate complexity
// Run performance tests in several modes: Precomputed preflop, CPU exhaustive, GPU exhaustive, Estimate
// Edge case when there are no possible winners (blockers)

static EquityCalculationInput CreateInput(const std::vector<std::string> &ranges, const std::string &boardString, std::string deadCards = "")
{
	EquityCalculationInput input;
	CreateEquityCalculationInput(ranges, boardString, deadCards, input);
	return input;
}

EquityCalculationOutput CalculateSyncAndAsync(EquityCalculationInput &input)
{
	EquityCalculationOutput asyncResult;
	CalculateEquityThreaded(input, asyncResult);

	EquityCalculationOutput syncResult;
	CalculateEquity(input, syncResult);

	if (!(syncResult == asyncResult))
		throw std::runtime_error("async and sync version are not equal");
	return syncResult;
}

TEST(EquityCalculatorPerformance, FullRunoutSingleThreaded)
{
	EquityCalculationInput input = CreateInput({"AhAc", "AsKs"}, "");
	EquityCalculationOutput output;
	CalculateEquity(input, output);
}

TEST(EquityCalculatorPerformance, FullRunoutThreaded)
{
	EquityCalculationOutput output;
	EquityCalculationInput input = CreateInput({"AhAc", "AsKs"}, "");
	CalculateEquityThreaded(input, output);
}

TEST(EquityCalculatorComplexityEstimate, InvalidInputs)
{
	WorkloadEstimate e;
	ASSERT_FALSE(CalculateComplexityEstimate(-1, {1, 2}, false, e));
	ASSERT_FALSE(CalculateComplexityEstimate(6, {1, 2}, false, e));
	ASSERT_FALSE(CalculateComplexityEstimate(3, {1}, false, e));
	ASSERT_FALSE(CalculateComplexityEstimate(3, {}, false, e));
	ASSERT_FALSE(CalculateComplexityEstimate(3, {2, 0}, false, e));
}

TEST(EquityCalculatorComplexityEstimate, WorkloadEstimates)
{
	WorkloadEstimate e;
	ASSERT_TRUE(CalculateComplexityEstimate(3, {2, 2}, false, e));
	ASSERT_EQ(WorkloadEstimateEnum::Instant, e.workload);

	ASSERT_TRUE(CalculateComplexityEstimate(0, {100, 100}, false, e));
	ASSERT_EQ(WorkloadEstimateEnum::Instant, e.workload);

	ASSERT_TRUE(CalculateComplexityEstimate(0, {2, 2}, true, e));
	ASSERT_EQ(WorkloadEstimateEnum::Low, e.workload);

	ASSERT_TRUE(CalculateComplexityEstimate(3, {100, 2}, false, e));
	ASSERT_EQ(WorkloadEstimateEnum::Low, e.workload);

	ASSERT_TRUE(CalculateComplexityEstimate(0, {2, 2, 1}, false, e));
	ASSERT_EQ(WorkloadEstimateEnum::Medium, e.workload);

	ASSERT_TRUE(CalculateComplexityEstimate(0, {4, 4, 4}, false, e));
	ASSERT_EQ(WorkloadEstimateEnum::High, e.workload);

	ASSERT_TRUE(CalculateComplexityEstimate(3, {100, 100, 20}, false, e));
	ASSERT_EQ(WorkloadEstimateEnum::High, e.workload);

	ASSERT_TRUE(CalculateComplexityEstimate(0, {100, 100, 200}, false, e));
	ASSERT_EQ(WorkloadEstimateEnum::VeryHigh, e.workload);
}

TEST(EquityCalculator, WhenNoEvaluationsAreValid_ReturnsInvalid)
{
	EquityCalculationInput input = CreateInput({"AhAc", "AsKs"}, "", "Ks");
	EquityCalculationOutput output = CalculateSyncAndAsync(input);
	ASSERT_EQ(EquityCalculateResult_Failure, output.result);
}

TEST(EquityCalculator, WhenRunoutComplete_EquityCalculated)
{
	EquityCalculationInput input;
	CreateEquityCalculationInput({"AA", "KK"}, "6s5s4h3c2h", "", input);
	EquityCalculationOutput output;
	CalculateEquity(input, output);
	ASSERT_NEAR(output.playerResults[0].eq.equity, 0.5f, 0.0001f);
	ASSERT_NEAR(output.playerResults[1].eq.equity, 0.5f, 0.0001f);
	ASSERT_NEAR(output.playerResults[0].eq.tie, 1.0f, 0.0001f);
	ASSERT_NEAR(output.playerResults[1].eq.tie, 1.0f, 0.0001f);
	ASSERT_NEAR(output.playerResults[0].eq.win, 0.0f, 0.0001f);
	ASSERT_NEAR(output.playerResults[1].eq.win, 0.0f, 0.0001f);
}

TEST(EquityCalculator, WhenDeadCardProvided_EquityChanges)
{
	EquityCalculationInput input;
	CreateEquityCalculationInput({"AA", "KsQs"}, "AsJsJh3c", "", input);
	EquityCalculationOutput noBlockerResult = CalculateSyncAndAsync(input);

	CreateEquityCalculationInput({"AA", "KsQs"}, "AsJsJh3c", "Ts", input);
	EquityCalculationOutput blockerResult = CalculateSyncAndAsync(input);

	ASSERT_NEAR(blockerResult.playerResults[1].eq.win, 0.0f, 0.0001f);
	ASSERT_NEAR(noBlockerResult.playerResults[1].eq.win, 1.0f / (float)(52 - 8), 0.0001f);
}

// TEST(EquityCalculatorPerformance, SyncAsyncCompare)
//{
//	EquityCalculationInput input;
//	CreateEquityCalculationInput({ "AhAc", "AsKs" }, "", input);
//	AssertAsyncAndSync(input);
// }
//
// TEST(EquityCalculatorPerformance, Preflop_SpecficHandVsSpecificHand)
//{
//	EquityCalculationInput input;
//	CreateEquityCalculationInput({ "AhAc", "AsKs" }, "", input);
//
//	std::vector<double> result = CalculateEquity(input);
//	for (int i = 0; i < 2; i++)
//	{
//		std::cout << "Player " << i << ": " << result[i] << std::endl;
//	}
// }
/*
TEST(EquityCalculatorPerformance, Preflop_MultipleHandsSmall)
{
	std::vector<double> result = CalculateEquity({ "AA", "KK" }, "");
	for (int i = 0; i < 2; i++)
		std::cout << "Player " << i << ": " << result[i] << std::endl;
}

TEST(EquityCalculatorPerformance, Preflop_MultipleHandsMedium)
{
	std::vector<double> result = CalculateEquity({ "QQ+, AQ+", "KK" }, "");
	for (int i = 0; i < 2; i++)
		std::cout << "Player " << i << ": " << result[i] << std::endl;
}*/

/*TEST(EquityCalculatorPerformance, PostFlop_MultipleHandsMedium)
{
	std::vector<double> result = CalculateEquity({ "QQ+, AQ+", "AJ+, AA", "45s, A3s, 33, 22, 4s5s, 5s6s, 6s7s, 8s9s, 9sTs, QsTs+, Ks9s+,"}, "As 2s 3d");
	for (int i = 0; i < 3; i++)
		std::cout << "Player " << i << ": " << result[i] << std::endl;
}*/

TEST(EquityCalculatorPrecompute, WhenNotTwoPlayers_Throws)
{
	EquityCalculationOutput output;
	EquityCalculationInput input = CreateInput({"AhAc", "AsKs", "2s3s"}, "");
	EXPECT_THROW({ CalculateEquityPrecomputed(input, *GetGlobalPrecomputeTable(), output); }, std::invalid_argument);
}

TEST(EquityCalculatorPrecompute, WhenBoardNotEmpty_Throws)
{
	EquityCalculationOutput output;
	EquityCalculationInput input = CreateInput({"AhAc", "AsKs", "2c3c"}, "6s2s3s");
	EXPECT_THROW({
		CalculateEquityPrecomputed(input, *GetGlobalPrecomputeTable(), output);
	},
				 std::invalid_argument);
}
const float thresh = (4.0f / (float)0xffff);
static void ASSERT_EQRESULT_CLOSE(const EquityCalculatSingleResult &r1, const EquityCalculatSingleResult &r2)
{
	ASSERT_NEAR((float)r1.equity, (float)r2.equity, thresh);
	ASSERT_NEAR((float)r1.win, (float)r2.win, thresh);
	ASSERT_NEAR((float)r1.tie, (float)r2.tie, thresh);
}

TEST(EquityCalculatorPrecompute, PrecomputedEqualsComputed)
{
	const int kHandCount = 10;
	Deck deck;

	CardSet playerHands[2];
	for (int i = 0; i < kHandCount; i++)
	{
		deck.DealCardSet(playerHands[0], 2);
		deck.DealCardSet(playerHands[1], 2);

		EquityCalculationInput input;
		input.expandedPlayerRanges.resize(2);
		input.expandedPlayerRanges[0].push_back(playerHands[0].ToMask());
		input.expandedPlayerRanges[1].push_back(playerHands[1].ToMask());
		input.startingBoardMask = (CardMask)0;

		EquityCalculationOutput preOutput;
		EquityCalculationOutput calcOutput;
		CalculateEquityPrecomputed(input, *GetGlobalPrecomputeTable(), preOutput);
		CalculateEquity(input, calcOutput);

		ASSERT_EQ(preOutput.result, calcOutput.result);

		for (int i = 0; i < 2; i++)
		{
			ASSERT_EQRESULT_CLOSE(preOutput.playerResults[i].eq, calcOutput.playerResults[i].eq);
			ASSERT_EQ(preOutput.playerResults[i].specificHandResults.size(), calcOutput.playerResults[i].specificHandResults.size());
			for (int j = 0; j < preOutput.playerResults[i].specificHandResults.size(); j++)
			{
				const EquityCalculationOutputPerHand &hP = preOutput.playerResults[i].specificHandResults[j];
				const EquityCalculationOutputPerHand &hC = calcOutput.playerResults[i].specificHandResults[j];
				ASSERT_EQRESULT_CLOSE(hP.eq, hC.eq);
				ASSERT_EQ(hP.cards, hC.cards);
			}
		}

		deck.ReturnCardSet(playerHands[0]);
		deck.ReturnCardSet(playerHands[1]);
	}
	// Pick X random hands
	// use precomputedata
	// use nonprecomputedata
	// make sure they're the same
}

TEST(EquityCalculatorPrecompute, PairPrecompute_WhenSuitDominated_IsLowerEquity)
{
}

TEST(EquityCalculatorInput, InvalidInputs)
{
	EquityCalculationInput input;
	ASSERT_FALSE(CreateEquityCalculationInput({"AA", "KK"}, "2s3c4s", "2s", input));
	ASSERT_FALSE(CreateEquityCalculationInput({"AA", "KK"}, "2s3c4s5c6s7c", "", input));
	ASSERT_FALSE(CreateEquityCalculationInput({"INVALID", "KK"}, "", "", input));
	ASSERT_FALSE(CreateEquityCalculationInput({"AA"}, "", "", input));
	ASSERT_FALSE(CreateEquityCalculationInput({"AA", "KK"}, "INVALID", "", input));
	ASSERT_FALSE(CreateEquityCalculationInput({"AA", "KK"}, "", "INVALID", input));
}