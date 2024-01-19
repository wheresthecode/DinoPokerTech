#include "PokerTech/pch.h"
#include "PokerTech/PokerTypes.h"
#include "PokerTech/PokerTech.h"
#include "PokerTech/ComboUtilities.h"
#include "PokerTech/PokerUtilities.h"
#include "PokerTech/Utilities.h"
#include "PokerTech/Precompute/PreflopPrecomputeTableAPI.h"
#include "PokerTech/HandRanking.h"
#include "PokerTech/RangeConverters/RangeBuilderSyntaxString.h"
#include "EquityCalculator.h"
#include <assert.h>

#if SUPPORT_THREADS
#include <thread>
#endif

const int kMaxPlayers = 5;

struct HandOutcomeTally
{
	HandOutcomeTally() : tally(), evaluated(0) {}
	inline void tallyWin(int players)
	{
		tally[players - 1]++;
	}
	inline void tallyEvaluated() { evaluated++; }

	bool CalculateResult(float &outEquity, float &outWin, float &outTie) const
	{
		outEquity = 0.0f;
		outWin = 0;
		outTie = 0;
		for (int i = 0; i < ARRAY_SIZE(tally); i++)
		{
			double thisP = (double)tally[i] / (double)evaluated;
			outEquity += thisP / (double)(i + 1);
			if (i == 0)
				outWin = thisP;
			else
				outTie += thisP;
		}

		return true;
	}

	bool CalculateResult(Fixed16Ratio &equity, Fixed16Ratio &outWin, Fixed16Ratio &outTie) const
	{
		float fEquity, fWin, fTie;
		bool r = CalculateResult(fEquity, fWin, fTie);
		equity = fEquity;
		outWin = fWin;
		outTie = fTie;
		return r;
	}

	uint64_t tally[kMaxPlayers];
	uint64_t evaluated;

	HandOutcomeTally &operator+=(const HandOutcomeTally &other)
	{
		this->evaluated += other.evaluated;
		for (int i = 0; i < kMaxPlayers; i++)
			this->tally[i] += other.tally[i];
		return *this;
	}
};

bool CalculateFromTally(const HandOutcomeTally &tally, EquityCalculatSingleResult &output)
{
	float fEquity, fWin, fTie;
	bool r = tally.CalculateResult(fEquity, fWin, fTie);
	output.equity = fEquity;
	output.win = fWin;
	output.tie = fTie;
	return r;
}

struct GlobalWorkGroupInput
{
	int PlayerCount;
	int R;
	int WorkGroupItemCount;
	CardMask startingBoardMask;
	CardMask deadCards;
	std::vector<std::vector<CardMask>> expandedPlayerRanges;
};

struct WorkGroupInput
{
	RunoutIterator::StateType State;
};

struct EquityPerHandContext
{
	EquityPerHandContext() : tally() {}
	// count of how many times we win in each tie situations [win 100%, tie with one other player, tie with two other players, .. etc]
	HandOutcomeTally tally;
};

struct EquityThreadContextPerPlayer
{
	EquityThreadContextPerPlayer() : tally() {}
	HandOutcomeTally tally;
	std::vector<EquityPerHandContext> perHand;
};

struct EquityThreadContext
{
	EquityThreadContext() : comparisonsPerRunout(0), runoutCount(0)
	{
	}
	std::vector<EquityThreadContextPerPlayer> players;
	uint64_t comparisonsPerRunout;
	uint64_t runoutCount;
};

struct ThreadedWorkGroupState
{
	std::vector<WorkGroupInput> inputs;
	// std::vector<WorkGroupResult> results;
	GlobalWorkGroupInput globalWorkGroupInput;
	std::vector<EquityThreadContext> threadedContexts;
};

bool CreateEquityCalculationInput(const std::vector<std::string> &ranges, const std::string &boardString, const std::string &deadCards, EquityCalculationInput &outInput)
{
	const int kPlayerCount = (int)ranges.size();
	outInput.expandedPlayerRanges.resize(ranges.size());
	int maxCombos = 1;
	for (int i = 0; i < ranges.size(); i++)
	{
		HandRange r;
		if (!BuildRangeFromHandSyntaxString(ranges[i], r))
			return false;
		outInput.expandedPlayerRanges[i] = r.ToMaskArray();
	}
	bool validDeadCards = ExplicitStringToCardMask(deadCards, outInput.deadCards);
	bool validCardMask = ExplicitStringToCardMask(boardString, outInput.startingBoardMask);

	if (ranges.size() < 2)
		return false;

	if (outInput.startingBoardMask.Count() > 5)
		return false;

	if (outInput.deadCards.HasOverlappingCards(outInput.startingBoardMask))
		return false;

	return validDeadCards && validCardMask;
}

void CalculateEquity(const std::vector<std::string> &ranges, const std::string &boardString, const std::string &deadCards, EquityCalculationOutput &outResult)
{
	EquityCalculationInput input;
	if (!CreateEquityCalculationInput(ranges, boardString, deadCards, input))
	{
		outResult.result = EquityCalculateResult_Failure;
	}

	return CalculateEquity(input, outResult);
}

void EvaluateSingleRunout(const std::vector<std::vector<CardMask>> &expandedPlayerRanges, std::vector<std::vector<HandRank>> &expandedRangeRanks, const CardMask &thisBoard, const CardMask &deadCards, EquityThreadContext &ctx)
{
	const int playerCount = (int)expandedPlayerRanges.size();

	// Rank all the hands
	expandedRangeRanks.resize(playerCount);
	for (int i = 0; i < playerCount; i++)
	{
		const std::vector<CardMask> &range = expandedPlayerRanges[i];
		expandedRangeRanks[i].resize(range.size());
		for (int j = 0; j < range.size(); j++)
		{
			expandedRangeRanks[i][j] = Rank5CardHandFrom7Cards(thisBoard | range[j]);
		}
	}

	FixedArray<int, kMaxComboIteratorLists> highestIndicies;
	FixedArray<int, kMaxComboIteratorLists> losingIndicies;
	for (MultiPlayerRangeComboIterator rangeIt(thisBoard, deadCards, expandedPlayerRanges); !rangeIt.IsDone(); rangeIt.Next())
	{
		highestIndicies.clear();
		highestIndicies.push_back(-1);

		// Pick the winner of this combo
		HandRank highestRank = 0;
		for (int i = 0; i < playerCount; i++)
		{
			EquityPerHandContext &perHandCtx = ctx.players[i].perHand[rangeIt.GetState()[i]];
			HandRank rank = expandedRangeRanks[i][rangeIt.GetState()[i]];
			if (rank > highestRank) // so far we're the highest rank
			{
				highestIndicies.clear();
				highestIndicies.push_back(i);
				highestRank = rank;
			}
			else if (rank == highestRank) // tied for highest rank
			{
				highestIndicies.push_back(i);
			}
			perHandCtx.tally.tallyEvaluated();
			ctx.players[i].tally.tallyEvaluated();
		}

		for (int i = 0; i < highestIndicies.size(); i++)
		{
			int pIdx = highestIndicies[i];
			EquityPerHandContext &perHandCtx = ctx.players[pIdx].perHand[rangeIt.GetState()[pIdx]];
			perHandCtx.tally.tallyWin(highestIndicies.size());
			ctx.players[pIdx].tally.tallyWin(highestIndicies.size());
		}
	}
}

bool GetPercentage(uint64_t numerator, uint64_t denominator, Fixed16Ratio &outPercentage)
{
	if (denominator == 0)
		return false;
	outPercentage = (float)((double)numerator / (double)denominator);
	return true;
}

static void Reduce(int playerCount, const ThreadedWorkGroupState &state, EquityCalculationOutput &result)
{
	EquityThreadContext reduceContext;
	reduceContext.players.resize(playerCount);
	for (int i = 0; i < playerCount; i++)
	{
		int rangeCount = (int)state.threadedContexts[0].players[i].perHand.size();
		reduceContext.players[i].perHand.resize(rangeCount);
	}

	for (int i = 0; i < state.threadedContexts.size(); i++)
	{
		const EquityThreadContext &ctx = state.threadedContexts[i];
		reduceContext.runoutCount += ctx.runoutCount;
		for (int j = 0; j < playerCount; j++)
		{
			EquityThreadContextPerPlayer &reduceP = reduceContext.players[j];
			const EquityThreadContextPerPlayer &srcP = ctx.players[j];
			reduceP.tally += srcP.tally;

			int rangeCount = (int)reduceP.perHand.size();
			for (int k = 0; k < rangeCount; k++)
				reduceP.perHand[k].tally += srcP.perHand[k].tally;
		}
	}

	result.result = (reduceContext.players[0].tally.evaluated != 0) ? EquityCalculateResult_Success : EquityCalculateResult_Failure;
	if (result.result == EquityCalculateResult_Failure)
		return;

	result.playerResults.resize(playerCount);
	for (int i = 0; i < playerCount; i++)
	{
		EquityCalculationOutputPerPlayer &perPlayerOutput = result.playerResults[i];

		CalculateFromTally(reduceContext.players[i].tally, perPlayerOutput.eq);

		int rangeCount = (int)reduceContext.players[i].perHand.size();
		perPlayerOutput.specificHandResults.reserve(rangeCount);
		for (int j = 0; j < rangeCount; j++)
		{
			const EquityPerHandContext &perHand = reduceContext.players[i].perHand[j];
			EquityCalculationOutputPerHand outPerHand;
			outPerHand.cards = state.globalWorkGroupInput.expandedPlayerRanges[i][j];
			if (CalculateFromTally(perHand.tally, outPerHand.eq))
				perPlayerOutput.specificHandResults.push_back(outPerHand);
		}
	}
}

void InitializeThreadedWorkGroupState(const EquityCalculationInput &input, int threadedContextCount, ThreadedWorkGroupState &state)
{
	int cardsAvailable = input.startingBoardMask.Count();
	int cardsRemaining = 5 - cardsAvailable;
	uint64_t estimatedComputations = ComboCountNChooseR(52 - cardsAvailable, cardsRemaining);

	const uint64_t workGroupSize = 16 * 1024; // 256;
	int workGroupCount = (int)((estimatedComputations + workGroupSize - 1) / workGroupSize);

	state.inputs.resize(workGroupCount);
	// state.results.resize(workGroupCount);

	state.globalWorkGroupInput.R = cardsRemaining;
	state.globalWorkGroupInput.deadCards = input.deadCards;
	state.globalWorkGroupInput.startingBoardMask = input.startingBoardMask;
	state.globalWorkGroupInput.expandedPlayerRanges = input.expandedPlayerRanges;
	state.globalWorkGroupInput.WorkGroupItemCount = workGroupSize;
	state.globalWorkGroupInput.PlayerCount = (int)input.expandedPlayerRanges.size();

	int curWorkGroup = 0;

	RunoutIterator runoutIt(input.startingBoardMask & input.deadCards, cardsRemaining);
	// walk through all workgroups and save iterator state.
	for (int i = 0; i < workGroupCount; i++)
	{
		WorkGroupInput &wgi = state.inputs[i];
		wgi.State = runoutIt.GetState();

		// Can we calculate the iterator state without walking the iterator??
		for (int j = 0; j < workGroupSize && !runoutIt.IsDone(); j++)
			runoutIt.Next();

		assert((i == (workGroupCount - 1)) == runoutIt.IsDone());
	}

	state.threadedContexts.resize(threadedContextCount);
	for (int i = 0; i < threadedContextCount; i++)
	{
		state.threadedContexts[i].players.resize(input.expandedPlayerRanges.size());
		for (int j = 0; j < state.threadedContexts[i].players.size(); j++)
		{
			state.threadedContexts[i].players[j].perHand.resize(input.expandedPlayerRanges[j].size());
		}
	}
}

static bool ExecuteWorkGroups(const GlobalWorkGroupInput &globalWorkGroupInput, const WorkGroupInput *inputs, int workGroupCount, EquityThreadContext &ctx, EquityProgressCallback callback)
{
	// Perform all the work
	long runoutProcessCount = 0;
	long totalExpectedRunouts = workGroupCount * globalWorkGroupInput.WorkGroupItemCount;
	for (int i = 0; i < workGroupCount; i++)
	{
		const WorkGroupInput &wgi = inputs[i];
		RunoutIterator runoutIt(globalWorkGroupInput.startingBoardMask & globalWorkGroupInput.deadCards, globalWorkGroupInput.R, &wgi.State);
		std::vector<std::vector<HandRank>> expandedRangeRanks;
		for (int j = 0; j < globalWorkGroupInput.WorkGroupItemCount && !runoutIt.IsDone(); j++, runoutIt.Next())
		{
			CardMask thisBoard = runoutIt.GetMask() | globalWorkGroupInput.startingBoardMask;
			EvaluateSingleRunout(globalWorkGroupInput.expandedPlayerRanges, expandedRangeRanks, thisBoard, globalWorkGroupInput.deadCards, ctx);
			if (callback && (runoutProcessCount++ % 100) == 0)
			{
				if (!callback((float((double)runoutProcessCount / (double)totalExpectedRunouts))))
					return false;
			}
		}
	}
	return true;
}

void CalculateEquity(const EquityCalculationInput &input, EquityCalculationOutput &outResult)
{
	CalculateEquity(input, outResult, (EquityProgressCallback)0);
}

void CalculateEquity(const EquityCalculationInput &input, EquityCalculationOutput &outResult, EquityProgressCallback callback)
{
	ThreadedWorkGroupState state;
	InitializeThreadedWorkGroupState(input, 1, state);

	if (!ExecuteWorkGroups(state.globalWorkGroupInput, &state.inputs[0], (int)state.inputs.size(), state.threadedContexts[0], std::move(callback)))
	{
		outResult.result = EquityCalculateResult_Cancel;
		return;
	}

	Reduce((int)input.expandedPlayerRanges.size(), state, outResult);
}

void CalculateEquitySyncAuto(const EquityCalculationInput &input, EquityCalculationOutput &outResult, EquityProgressCallback progressCallback)
{
	if (input.expandedPlayerRanges.size() == 2 && input.startingBoardMask.Count() == 0 && !input.deadCards.HasCards() && GetGlobalPrecomputeTable() != NULL)
	{
		// This should be so fast that we don't need a progress update
		CalculateEquityPrecomputed(input, *GetGlobalPrecomputeTable(), outResult);
		return;
	}
#if SUPPORT_THREADS
	CalculateEquityThreaded(input, outResult);
#else
	CalculateEquity(input, outResult, std::move(progressCallback));
#endif
}

void CalculateEquityPrecomputed(const EquityCalculationInput &input, const PreflopPrecomputeTable &preflopPrecomputeTable, EquityCalculationOutput &outResult)
{
	if (input.startingBoardMask.v != 0)
		throw std::invalid_argument("Cannot use precomputed preflop table when community cards are present");

	if (input.expandedPlayerRanges.size() != 2)
		throw std::invalid_argument("Can only use precalculated equity calculator for 2 players");

	if (input.deadCards.Count() > 0)
		throw std::invalid_argument("precalculated equity not supported with dead cards");

	std::vector<CardSet> playerHands[2];

	for (int i = 0; i < 2; i++)
	{
		playerHands[i].reserve(input.expandedPlayerRanges[i].size());
		for (int j = 0; j < input.expandedPlayerRanges[i].size(); j++)
			playerHands[i].push_back(CardSet(input.expandedPlayerRanges[i][j]));
	}

	double playerEquityWin[2] = {0, 0};
	double playerEquityTie[2] = {0, 0};

	struct SPerHandInfo
	{
		double win, tie;
		int count;
	};
	std::vector<SPerHandInfo> perHandWin[2];
	perHandWin[0].resize(input.expandedPlayerRanges[0].size());
	perHandWin[1].resize(input.expandedPlayerRanges[1].size());
	int count = 0;
	for (int i = 0; i < input.expandedPlayerRanges[0].size(); i++)
	{
		for (int j = 0; j < input.expandedPlayerRanges[1].size(); j++)
		{
			if ((input.expandedPlayerRanges[0][i] & input.expandedPlayerRanges[1][j]) == 0)
			{
				PrecomputeResult r = PreflopPrecomputeTable_GetValue(&preflopPrecomputeTable, playerHands[0][i], playerHands[1][j]);

				perHandWin[0][i].count++;
				perHandWin[0][i].win += r.p1WinEquity;
				perHandWin[0][i].tie += r.tie;

				perHandWin[1][j].count++;
				perHandWin[1][j].win += r.p2WinEquity;
				perHandWin[1][j].tie += r.tie;

				playerEquityWin[0] += r.p1WinEquity;
				playerEquityTie[0] += r.tie;
				playerEquityWin[1] += r.p2WinEquity;
				playerEquityTie[1] += r.tie;
				count++;
			}
		}
	}

	outResult.playerResults.resize(2);
	for (int i = 0; i < 2; i++)
	{
		EquityCalculationOutputPerPlayer &pr = outResult.playerResults[i];
		pr.eq.win = (float)(playerEquityWin[i] / (double)count);
		pr.eq.tie = (float)(playerEquityTie[i] / (double)count);
		pr.eq.equity = (float)((playerEquityTie[i] / 2 + playerEquityWin[i]) / (double)count);
		pr.specificHandResults.reserve(perHandWin[i].size());
		for (int j = 0; j < perHandWin[i].size(); j++)
		{
			SPerHandInfo &phd = perHandWin[i][j];
			if (phd.count != 0)
			{
				EquityCalculationOutputPerHand h;
				h.cards = input.expandedPlayerRanges[i][j];
				h.eq.win = (float)(phd.win / (double)phd.count);
				h.eq.tie = (float)(phd.tie / (double)phd.count);
				h.eq.equity = (float)((phd.win + phd.tie / 2) / (double)phd.count);
				pr.specificHandResults.push_back(h);
			}
		}
	}

	outResult.diagnostic.handEvaluations = count;
	outResult.diagnostic.runOutsEvaluated = 0;
	outResult.diagnostic.comparisonsPerRunout = 0;
	outResult.result = outResult.diagnostic.handEvaluations == count ? EquityCalculateResult_Success : EquityCalculateResult_Failure;
}

bool CalculateComplexityEstimate(int boardCardCount, const std::vector<int> &playerRangesCount, bool hasDeadCards, WorkloadEstimate &outEstimate)
{
	if (boardCardCount < 0 || boardCardCount > 5)
		return false;
	if (playerRangesCount.size() < 2)
		return false;

	int cardsToCome = 5 - boardCardCount;

	uint64_t runoutCount = cardsToCome != 0 ? ComboCountNChooseR(52 - boardCardCount, cardsToCome) : 1;

	uint64_t comparisonsPerRunout = 1;
	uint64_t handEvaluationsPerRunout = 0;
	for (int i = 0; i < playerRangesCount.size(); i++)
	{
		comparisonsPerRunout *= playerRangesCount[i];
		handEvaluationsPerRunout += playerRangesCount[i];
	}

	if (comparisonsPerRunout == 0)
		return false;

	// will use precomputed data
	bool canUsePrecompute = !hasDeadCards && boardCardCount == 0 && playerRangesCount.size() == 2;
	if (canUsePrecompute)
	{
		outEstimate.handComparisons = handEvaluationsPerRunout;
		outEstimate.handEvaluations = 0;
	}
	else
	{
		outEstimate.handComparisons = runoutCount * comparisonsPerRunout;
		outEstimate.handEvaluations = runoutCount * handEvaluationsPerRunout;
	}

	int c52c5 = ComboCountNChooseR(52, 5);
	int c49c2 = ComboCountNChooseR(49, 2);
	int compareThresholds[(int)WorkloadEstimateEnum::VeryHigh] = {
		c49c2 * 4,	// Instant
		c52c5 * 4,	// Low
		c52c5 * 32, // Medium
		c52c5 * 128 // High
	};
	int evalThresholds[(int)WorkloadEstimateEnum::VeryHigh] = {
		c49c2 * 4,	// Instant
		c52c5 * 4,	// Low
		c52c5 * 32, // Medium
		c52c5 * 128 // High
	};

	outEstimate.workload = WorkloadEstimateEnum::VeryHigh;

	for (int i = 0; i < ARRAY_SIZE(evalThresholds); i++)
	{
		if (outEstimate.handComparisons <= compareThresholds[i] && outEstimate.handEvaluations <= evalThresholds[i])
		{
			outEstimate.workload = (WorkloadEstimateEnum)i;
			break;
		}
	}
	return true;
}

#if SUPPORT_THREADS

void CalculateEquityThreaded(const EquityCalculationInput &input, EquityCalculationOutput &outResult)
{
	CalculateEquityOperation *op = new CalculateEquityOperation(input);
	op->WaitForCompletion();
	assert(op->GetStatus() == OperationStatus::Completed);
	outResult = op->GetResult();
	op->Release();
}

void CalculateEquityOperation::ProcessWorkGroups(int contextIndex)
{
	EquityThreadContext &ctx = m_WorkGroupState->threadedContexts[contextIndex];
	const int totalWorkGroups = (int)m_WorkGroupState->inputs.size();
	const GlobalWorkGroupInput &gwgi = m_WorkGroupState->globalWorkGroupInput;
	while (true)
	{
		int nextWorkIndex = m_NextWorkGroupIndex.fetch_add(1);
		if (nextWorkIndex >= totalWorkGroups)
			break;

		if (m_CancelRequested)
			break;

		const WorkGroupInput &wgi = m_WorkGroupState->inputs[nextWorkIndex];
		RunoutIterator runoutIt(gwgi.startingBoardMask & gwgi.deadCards, gwgi.R, &wgi.State);
		std::vector<std::vector<HandRank>> expandedRangeRanks;
		for (int j = 0; j < gwgi.WorkGroupItemCount && !runoutIt.IsDone(); j++, runoutIt.Next())
		{
			CardMask thisBoard = runoutIt.GetMask() | gwgi.startingBoardMask;
			EvaluateSingleRunout(gwgi.expandedPlayerRanges, expandedRangeRanks, thisBoard, gwgi.deadCards, ctx);
		}
		if (m_WorkGroupsCompleted.fetch_add(1) == (totalWorkGroups - 1))
		{
			Reduce(gwgi.PlayerCount, *m_WorkGroupState, m_Results);
			m_AllThreadsDone = true;
		}
	}
}

CalculateEquityOperation *CalculateEquityAsync(const EquityCalculationInput &input)
{
	return new CalculateEquityOperation(input);
}

CalculateEquityOperation::CalculateEquityOperation(const EquityCalculationInput &input) : m_WorkGroupsCompleted(0),
																						  m_NextWorkGroupIndex(0),
																						  m_CancelRequested(false),
																						  m_AllThreadsDone(false),
																						  m_Status(OperationStatus::InProgress)
{
	const int kThreadCount = 12;
	m_WorkGroupState = new ThreadedWorkGroupState();
	InitializeThreadedWorkGroupState(input, kThreadCount, *m_WorkGroupState); // Could also be done from a thread if it turns out to be slow

	// While threads are running we need to have a reference
	AddRef();

	int threadCount = std::min((int)kThreadCount, (int)m_WorkGroupState->inputs.size());

	m_Threads.resize(threadCount);
	for (int i = 0; i < threadCount; i++)
		m_Threads[i] = new std::thread([i, this]
									   { this->CalculateEquityOperation::ProcessWorkGroups(i); });
}

CalculateEquityOperation::~CalculateEquityOperation()
{
	assert(m_Threads.size() == 0);
	delete m_WorkGroupState;
}

void CalculateEquityOperation::JoinAndDeleteThreads()
{
	if (m_Threads.size() != 0)
	{
		for (int i = 0; i < (int)m_Threads.size(); i++)
		{
			m_Threads[i]->join();
			delete m_Threads[i];
		}
		m_Threads.clear();
		this->Release(); // Now the threads are no longer running so we can delete this reference
	}
}

void CalculateEquityOperation::WaitForCompletion()
{
	JoinAndDeleteThreads();
}

void CalculateEquityOperation::CheckMainThreadIntegrate()
{
	if (m_Status == OperationStatus::InProgress)
	{
		if (m_AllThreadsDone)
		{
			JoinAndDeleteThreads();
			m_Status = m_CancelRequested ? OperationStatus::Failed : OperationStatus::Completed;
		}
	}
}

void CalculateEquityOperation::RequestCancel()
{
	CheckMainThreadIntegrate();
	if (m_Status == OperationStatus::InProgress)
		m_CancelRequested = true;
}

float CalculateEquityOperation::GetProgress()
{
	return (float)((double)m_WorkGroupsCompleted / (double)m_WorkGroupState->inputs.size());
}

OperationStatus CalculateEquityOperation::GetStatus()
{
	CheckMainThreadIntegrate();
	return m_Status;
}

const EquityCalculationOutput &CalculateEquityOperation::GetResult()
{
	CheckMainThreadIntegrate();
	assert(GetStatus() == OperationStatus::Completed);
	return m_Results;
}

#endif
