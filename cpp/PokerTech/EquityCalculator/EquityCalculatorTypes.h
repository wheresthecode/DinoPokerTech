#pragma once

#include <functional>
#include "PokerTech/PokerTypes.h"
#include "PokerTech/PokerUtilities.h"

struct EquityCalculationInput
{
    CardMask startingBoardMask;
    CardMask deadCards;
    std::vector<std::vector<CardMask>> expandedPlayerRanges;
};

struct EquityCalculatSingleResult
{
    Fixed16Ratio equity;
    Fixed16Ratio win;
    Fixed16Ratio tie;
    bool operator==(const EquityCalculatSingleResult &rhs) const
    {
        return win == rhs.win && tie == rhs.tie && equity == rhs.equity;
    }
};

struct EquityCalculationOutputPerHand
{
    CardMask cards;
    EquityCalculatSingleResult eq;

    bool operator==(const EquityCalculationOutputPerHand &rhs) const
    {
        return rhs.eq == eq && rhs.cards == cards;
    }
};

struct EquityCalculationOutputPerPlayer
{
    EquityCalculatSingleResult eq;
    std::vector<EquityCalculationOutputPerHand> specificHandResults;

    bool operator==(const EquityCalculationOutputPerPlayer &rhs) const
    {
        return rhs.eq == eq && rhs.specificHandResults == specificHandResults;
    }
};

enum EquityCalculateResult
{
    EquityCalculateResult_Success,
    EquityCalculateResult_Failure,
    EquityCalculateResult_Cancel
};

struct EquityCalculationDiagnostic // Data that is not deterministic and might change depending on the method used to compute.
{
    int totalTimeMs;
    uint64_t handEvaluations;
    uint64_t runOutsEvaluated;
    uint64_t comparisonsPerRunout;
};

struct EquityCalculationOutput
{
    EquityCalculateResult result;
    std::vector<EquityCalculationOutputPerPlayer> playerResults;
    EquityCalculationDiagnostic diagnostic;

    bool operator==(const EquityCalculationOutput &other) const
    {
        return result == other.result && playerResults == other.playerResults;
    }
};

typedef std::function<bool(float)> EquityProgressCallback;

enum class WorkloadEstimateEnum
{
    Instant = 0,
    Low,
    Medium,
    High,
    VeryHigh
};

struct WorkloadEstimate
{
    WorkloadEstimateEnum workload;
    uint64_t handEvaluations;
    uint64_t handComparisons;
};