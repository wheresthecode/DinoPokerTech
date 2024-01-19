#include "TypesToJson.h"
#include "../external/nlohmann/json.hpp"
#include "../PokerTech/EquityCalculator/EquityCalculatorTypes.h"

using json = nlohmann::json;

NLOHMANN_JSON_SERIALIZE_ENUM(EquityCalculateResult, {
                                                        {EquityCalculateResult_Success, "success"},
                                                        {EquityCalculateResult_Failure, "failure"},
                                                        {EquityCalculateResult_Cancel, "cancel"},
                                                    });

void to_json(json &j, const CardMask &p)
{
    j = json(CardMaskToString(p));
}

void from_json(const json &j, CardMask &p)
{
    assert(false);
}

double ToTwoDecimalPlaces(double d)
{

    int i;

    if (d >= 0)

        i = static_cast<int>(d * 1000 + 0.5);

    else

        i = static_cast<int>(d * 1000 - 0.5);

    return (i / 1000.0);
}

void to_json(json &j, const Fixed16Ratio &r)
{
    j = json(ToTwoDecimalPlaces(r.ToFloat()));
}

void from_json(const json &j, Fixed16Ratio &r)
{
    double v;
    j.get_to(v);
    r.FromFloat((float)v);
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EquityCalculatSingleResult, equity, win, tie);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EquityCalculationOutputPerHand, cards, eq);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EquityCalculationOutputPerPlayer, eq, specificHandResults);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EquityCalculationDiagnostic, totalTimeMs, handEvaluations, runOutsEvaluated, comparisonsPerRunout);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EquityCalculationOutput, result, playerResults, diagnostic);

std::string EquityCalculationOutputToJson(EquityCalculationOutput &output)
{

    json j(output);
    return j.dump();
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WorkloadEstimate, workload, handEvaluations, handComparisons);

std::string ComplexityEstimateToJson(const WorkloadEstimate &workload)
{
    json j(workload);
    return j.dump();
}
