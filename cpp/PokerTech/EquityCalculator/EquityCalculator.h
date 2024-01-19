#pragma once

#include <atomic>
#include <thread>
#include <functional>
#include "PokerTech/PokerTypes.h"
#include "PokerTech/Precompute/PreflopPrecomputeTableAPI.h"
#include "EquityCalculatorTypes.h"

enum EquityCalculateOptions
{
	EquityCalculateOption_DisablePrecompute = 1 << 0,
	EquityCalculateOption_ForceSingleThread = 1 << 1
};

bool CreateEquityCalculationInput(const std::vector<std::string> &ranges, const std::string &boardString, const std::string &deadCards, EquityCalculationInput &outInput);

void CalculateEquityPrecomputed(const EquityCalculationInput &input, const PreflopPrecomputeTable &preflopPrecomputeTable, EquityCalculationOutput &outResult);
// std::vector<double> CalculateEquity(const EquityCalculationInput &input);
void CalculateEquity(const std::vector<std::string> &ranges, const std::string &boardString, const std::string &deadCards, EquityCalculationOutput &outResult);
void CalculateEquity(const EquityCalculationInput &input, EquityCalculationOutput &outResult);
void CalculateEquityThreaded(const EquityCalculationInput &input, EquityCalculationOutput &outResult);

void CalculateEquitySyncAuto(const EquityCalculationInput &input, EquityCalculationOutput &outResult, EquityProgressCallback progressCallback);

void CalculateEquity(const EquityCalculationInput &input, EquityCalculationOutput &outResult, EquityProgressCallback progressCallback);

bool CalculateComplexityEstimate(int boardCardCount, const std::vector<int> &playerRangesCount, bool hasDeadCards, WorkloadEstimate &outEstimate);

#if SUPPORT_THREADS

enum class OperationStatus
{
	InProgress,
	Completed,
	Failed
};

class RefCountedObject
{
public:
	RefCountedObject()
		: m_RefCount(1)
	{
	}
	void AddRef() { m_RefCount++; }
	void Release()
	{
		if (--m_RefCount == 0)
			delete this;
	}
	std::atomic<int> m_RefCount;
};

struct ThreadedWorkGroupState;

class CalculateEquityOperation : public RefCountedObject
{
public:
	CalculateEquityOperation(const EquityCalculationInput &input);
	~CalculateEquityOperation();

	void RequestCancel();
	float GetProgress();
	OperationStatus GetStatus();
	const EquityCalculationOutput &GetResult();

	void WaitForCompletion();

private:
	void CheckMainThreadIntegrate();

	void JoinAndDeleteThreads();

	void ProcessWorkGroups(int contextIndex);

	std::vector<std::thread *> m_Threads;
	ThreadedWorkGroupState *m_WorkGroupState;

	std::atomic<int> m_WorkGroupsCompleted;
	std::atomic<int> m_NextWorkGroupIndex;
	std::atomic<bool> m_CancelRequested;
	std::atomic<bool> m_AllThreadsDone;

	EquityCalculationOutput m_Results;
	OperationStatus m_Status;
};
CalculateEquityOperation *CalculateEquityAsync(const EquityCalculationInput &input);

#endif