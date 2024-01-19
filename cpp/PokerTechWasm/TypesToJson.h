#pragma once
#include "../PokerTech/EquityCalculator/EquityCalculatorTypes.h"

std::string ComplexityEstimateToJson(const WorkloadEstimate &workload);

std::string EquityCalculationOutputToJson(EquityCalculationOutput &output);
