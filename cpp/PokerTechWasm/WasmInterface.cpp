// This file exposes various interfaces that can be called from javascript
#include <emscripten/bind.h>
#include <emscripten.h>
#include <iostream>
#include <chrono>
#include "../PokerTech/EquityCalculator/EquityCalculator.h"
#include "../PokerTech/PokerUtilities.h"
#include "../PokerTech/PokerTech.h"
#include "../PokerTech/EnabledRange.h"
#include "../PokerTech/StringUtility.h"
#include "../PokerTech/RangeConverters/RangeBuilderSyntaxString.h"
#include "TypesToJson.h"

using namespace emscripten;

unsigned long long GetTickCount()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

#if SUPPORT_THREADS
class CalculateEquityEmOperation
{
public:
    CalculateEquityEmOperation(const std::vector<std::string> &ranges, const std::string &boardString)
    {
        EquityCalculationInput input;
        if (!CreateEquityCalculationInput(ranges, boardString, input))
            throw std::invalid_argument("Invalid Input");

        m_Op = CalculateEquityAsync(input);
    }
    ~CalculateEquityEmOperation()
    {
        m_Op->WaitForCompletion();
        m_Op->Release();
        m_Op = NULL;
    }

    void RequestCancel() { m_Op->RequestCancel(); }
    float GetProgress() const { return m_Op->GetProgress(); }
    OperationStatus GetStatus() const { return m_Op->GetStatus(); }
    std::vector<double> GetResult() const { return m_Op->GetResult(); }

    std::string GetBoard() const { return m_Board; }
    std::vector<std::string> GetRanges() const { return m_Ranges; }

private:
    CalculateEquityOperation *m_Op;
    std::string m_Board;
    std::vector<std::string> m_Ranges;
};
#endif

std::string CalculateEquity_Wrap(const std::vector<std::string> &ranges, const std::string &boardString, const std::string &deadCards)
{
    EquityCalculationInput input;
    if (!CreateEquityCalculationInput(ranges, boardString, deadCards, input))
        throw std::invalid_argument("Invalid Input");

    EquityCalculationOutput output;
    CalculateEquitySyncAuto(input, output, 0);
    if (output.result != EquityCalculateResult_Success)
        throw std::invalid_argument("CalculationFailed");

    return EquityCalculationOutputToJson(output);
};

std::string CalculateEquityAsyncify_Wrap(const std::vector<std::string> &ranges, const std::string &boardString, const std::string &deadCards, emscripten::val progressCallback)
{

    unsigned long long lastSleep = GetTickCount();
    std::function<bool(float)> callback = [&](float progress)
    {
        unsigned long long next = GetTickCount();
        if (next - lastSleep > 33)
        {
            emscripten_sleep(0);
            lastSleep = next;
        }

        emscripten::val result = progressCallback(progress);
        return result.as<bool>();
    };
    // wrap progressCallback in a std callback lambda
    EquityCalculationInput input;
    EquityCalculationOutput output;
    if (!CreateEquityCalculationInput(ranges, boardString, deadCards, input))
        throw std::invalid_argument("Invalid Input");

    CalculateEquitySyncAuto(input, output, std::move(callback));
    if (output.result != EquityCalculateResult_Success)
        throw "Operation was cancelled or was invalid";

    return EquityCalculationOutputToJson(output);
}

int GetRangeStringDetailsEmWrap(const std::string &rangeString)
{
    HandRange range;
    return BuildRangeFromHandSyntaxString(rangeString, range) ? range.size() : -1;
}

bool IsValidRangeStringEmWrap(std::string rangeString)
{
    HandRange range;
    return BuildRangeFromHandSyntaxString(rangeString, range) && !range.empty();
}

bool IsValidBoardStringEmWrap(std::string boardString)
{
    return IsValidBoardString(boardString);
}

void InitializePokerLib()
{
    InitializePokerTech();
}

std::string EnabledRangeToSyntax(std::vector<float> &enabledRange)
{
    if (enabledRange.size() != kTotalRangeEntries)
        throw std::invalid_argument(string_format("range array must be of size %d, was %d", kTotalRangeEntries, enabledRange.size()));

    EnabledRange range;
    range.SetFromFloatArray(&enabledRange[0], enabledRange.size());
    return range.CalculateRangeSyntaxString();
}

std::vector<float> SyntaxToEnabledRange(std::string syntaxRange)
{
    EnabledRange range;
    if (!range.ConvertFromRangeString(syntaxRange))
        std::invalid_argument("Invalid syntax string text");
    std::vector<float> floatValues;
    floatValues.resize(kTotalRangeEntries);
    range.GetRangeAsFloatArray(&floatValues[0], floatValues.size());
    return floatValues;
}

std::string CalculateComplexityEstimateEmWrap(int boardCardCount, const std::vector<int> &rangeCounts, bool hasDeadCards)
{
    WorkloadEstimate e;
    if (!CalculateComplexityEstimate(boardCardCount, rangeCounts, hasDeadCards, e))
        return "";
    return ComplexityEstimateToJson(e);
}

EMSCRIPTEN_BINDINGS(my_module)
{
    function("CalculateEquitySync", &CalculateEquity_Wrap);
    function("IsValidRangeString", &IsValidRangeStringEmWrap);
    function("IsValidBoardString", &IsValidBoardStringEmWrap);
    function("GetRangeStringHandCount", &GetRangeStringDetailsEmWrap);
    function("CalculateComplexityEstimate", &CalculateComplexityEstimateEmWrap);
    function("CalculateEquity", &CalculateEquityAsyncify_Wrap);
    function("InitializePokerLib", &InitializePokerLib);
    function("SyntaxToEnabledRange", &SyntaxToEnabledRange);
    function("EnabledRangeToSyntax", &EnabledRangeToSyntax);

    register_vector<std::string>("StringList");
    register_vector<float>("FloatList");
    register_vector<double>("vector<double>");
    register_vector<int>("IntList");

#if SUPPORT_THREADS
    class_<CalculateEquityEmOperation>("CalculateEquityEmOperation")
        .constructor<std::vector<std::string>, std::string>()
        .property("status", &CalculateEquityEmOperation::GetStatus)
        .property("results", &CalculateEquityEmOperation::GetResult)
        .property("progress", &CalculateEquityEmOperation::GetProgress)
        .function("requestCancel", &CalculateEquityEmOperation::RequestCancel);

    enum_<OperationStatus>("OperationStatus")
        .value("InProgress", OperationStatus::InProgress)
        .value("Completed", OperationStatus::Completed)
        .value("Failed", OperationStatus::Failed);
#endif
}