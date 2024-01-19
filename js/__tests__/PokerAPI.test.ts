import { afterAll, beforeAll, expect, test } from '@jest/globals';
import { CardRanks, CardSuit, IPokerAPI, WorkLoadEstimateSize, createPokerAPI } from '../src/PokerAPI';
import { ComboSelectorMask, EnabledRange, GetHandName, H2 } from "../src/RangeUtility";

// These tests are focused on making sure the bindings are working correctly,
// not as much on testing the underlying C++ code. The c++ code is tested with a unit test suite

let API: IPokerAPI;

beforeAll(async () => {
  API = await createPokerAPI();
});

afterAll(() => {
  //if (API.IsThreadingSupported()) API.GetEmAPI().PThread.terminateAllThreads();
});

test("IsValidRangeStringTest", () => {
  expect(API.IsValidRangeString("AcAd")).toBe(true);
});

test("CanConvertToNativeRange", () => {

  let range = new EnabledRange();
  range.SetFromSyntaxString("AK");
  const result: string = range.GetSyntaxString();
  expect(result).toBe("AK");
});

function areRangesEqual(r1: EnabledRange, r2: EnabledRange): boolean {
  for (let i = 0; i < r1.hands.length; i++) {
    if (Math.abs(r1.hands[i] - r2.hands[i]) > 0.01)
      return false;
  }
  return true;
}

function testSingleHandConversion(hand: H2) {
  let range = new EnabledRange();
  let convertedRange = new EnabledRange();
  range.clear();
  range.setHandWeight(hand, 1.0);
  expect(range.GetSyntaxString()).toBe(GetHandName(hand));
}

test("SyntaxConversionSuits", () => {
  for (let i = 0; i < CardSuit.kSuitCount; i++)
    for (let j = 0; j < CardSuit.kSuitCount; j++) {
      const hand: H2 = [{ rank: CardRanks.Rank_A, suit: i as CardSuit }, { rank: CardRanks.Rank_K, suit: j as CardSuit }];
      testSingleHandConversion(hand);
    }
});

test("SyntaxConversionSuitsPairs", () => {
  for (let i = 0; i < CardSuit.kSuitCount; i++)
    for (let j = i + 1; j < CardSuit.kSuitCount; j++) {
      const hand: H2 = [{ rank: CardRanks.Rank_A, suit: i as CardSuit }, { rank: CardRanks.Rank_A, suit: j as CardSuit }];
      testSingleHandConversion(hand);
    }
});

test("EnabledRangeSetAndGetBlocks", () => {
  let rAll = new EnabledRange();
  let rSuited = new EnabledRange();
  let rUnsuited = new EnabledRange();
  let rPairs = new EnabledRange();

  const r1 = CardRanks.Rank_A;
  const r2 = CardRanks.Rank_K;
  rAll.setBlockWeight(r1, r2, ComboSelectorMask.All, 1.0);
  rSuited.setBlockWeight(r1, r2, ComboSelectorMask.Suited, 1.0);
  rUnsuited.setBlockWeight(r1, r2, ComboSelectorMask.Unsuited, 1.0);
  rPairs.setBlockWeight(r1, r1, ComboSelectorMask.All, 1.0);

  expect(rAll.getBlockWeight(r1, r2, ComboSelectorMask.All)).toBeCloseTo(1.0, 0.001);
  expect(rSuited.getBlockWeight(r1, r2, ComboSelectorMask.Suited)).toBeCloseTo(1.0, 0.001);
  expect(rUnsuited.getBlockWeight(r1, r2, ComboSelectorMask.Unsuited)).toBeCloseTo(1.0, 0.001);
  expect(rPairs.getBlockWeight(r1, r1, ComboSelectorMask.All)).toBeCloseTo(1.0, 0.001);

  const weightCheck: [number[], number][] = [
    [rAll.getBlockWeights(r1, r2, ComboSelectorMask.All), 16],
    [rSuited.getBlockWeights(r1, r2, ComboSelectorMask.Suited), 4],
    [rUnsuited.getBlockWeights(r1, r2, ComboSelectorMask.Unsuited), 12],
    [rPairs.getBlockWeights(r1, r1, ComboSelectorMask.Pairs), 6]
  ]
  weightCheck.forEach(([indicies, expectedCount]: [number[], number]) => {
    expect(indicies.length).toBe(expectedCount);
    indicies.forEach(e => {
      expect(e).toBeCloseTo(1.0, 0.001);
    });
  });

  for (let i = 0; i < 4; i++)
    for (let j = 0; j < 4; j++) {
      let w = rAll.getHandWeight([{ rank: r1, suit: i }, { rank: r2, suit: j }]);
      let sw = rSuited.getHandWeight([{ rank: r1, suit: i }, { rank: r2, suit: j }]);
      let uw = rUnsuited.getHandWeight([{ rank: r1, suit: i }, { rank: r2, suit: j }]);

      expect(w).toBeCloseTo(1.0, 0.001);
      expect(sw).toBeCloseTo(i === j ? 1.0 : 0.0, 0.001);
      expect(uw).toBeCloseTo(i !== j ? 1.0 : 0.0, 0.001);

      if (i !== j) {
        let pw = rPairs.getHandWeight([{ rank: r1, suit: i }, { rank: r1, suit: j }]);
        expect(pw).toBeCloseTo(1.0, 0.001);
      }
    }
});



test("RangeCalculateSyncTest", async () => {
  const board = "Ks 9c Tc";
  const ranges = ["KcQc", "QsJs"];
  let resultV = await API.CalculateEquity(ranges, board, "", () => { return true });
  expect(resultV.playerResults[0].eq.win).toBeCloseTo(0.3878, 3);
  expect(resultV.playerResults[0].eq.tie).toBeCloseTo(0.06969, 3);
  expect(resultV.playerResults[1].eq.win).toBeCloseTo(0.5424, 3);
  expect(resultV.playerResults[1].eq.tie).toBeCloseTo(0.06969, 3);
});

// Convert hand range to NativeEnabledRange. 
// Then Convert to EnabledRange. 
// Then Convert back to NativeEnabledRange. Then convert back to syntax.

/*test("CreateAndDeleteRange", () => {
  let range: any = API.CreateEnabledRange();
  range.SetCombos(CardRanks.Rank_2, CardRanks.Rank_3, RankComboCategory.All, 0.5);
  let r: number = range.GetRankRatio(CardRanks.Rank_2, CardRanks.Rank_3, RankComboCategory.All);
  console.log(range.CalculateRangeString());
  //let r: number = range.GetRankRatio(0, 1, 0);
  range.delete();
})*/


/*
test("RangeCalculateSyncTestPreflop", async () => {
  board = "";
  ranges = ["22+,A2+", "TT+, AQ+"];
  let resultV = await API.CalculateEquity(ranges, board, () => { });

  //expect(resultV[0]).toBeCloseTo(0.7239, 3);
  //expect(resultV[1]).toBeCloseTo(0.276, 3);
});* /

/*test("RangeCalcaulateAsyncifyTest", async () => {
  board = ""; // "Ac 9d Ts";
  ranges = ["AA", "QsJs"]; //ranges = ["TT+", "QJ, AA, A9, AT, T9", "TT+"];
  let lastP = 0.0;
  result = await API.EquityCalcaulateAsyncify(ranges, board, null, (p) => {
    if (p > lastP + 0.1) {
      console.log(p);
      lastP = p;
    }
  });
  console.log(result);
}, 90000);*/

// Only enabled if we are using pthreads

/*test("RangeCalcaulateAsyncTest", async () => {
  if (API.IsThreadingSupported()) {
    board = ""; //"Ac 9d Ts";
    ranges = ["AA", "QsJs"];
    let lastP = 0.0;
    result = await API.EquityCalculateAsync(ranges, board, null, (p) => {
      //console.log(p);
      if (p > lastP + 0.1) {
        //console.log(p);
        lastP = p;
      }
    });
    console.log(result);
  } else {
    expect(() => API.EquityCalcaulateAsync()).toThrow("");
  }
}, 90000);*/

test("GetRangeStringHandCount", () => {
  expect(API.GetRangeStringHandCount("AA")).toBe(6);
  expect(API.GetRangeStringHandCount("KK+")).toBe(12);
  expect(API.GetRangeStringHandCount("K")).toBe(-1);
});

test("EstimateComplexity", () => {
  expect(API.CalculateComplexityEstimate(3, [2, 2], false).workload).toBe(WorkLoadEstimateSize.Instant);
  expect(API.CalculateComplexityEstimate(0, [20, 20], false).workload).toBe(WorkLoadEstimateSize.Instant);
  expect(API.CalculateComplexityEstimate(0, [20, 20], true).workload).toBe(WorkLoadEstimateSize.VeryHigh);
});
