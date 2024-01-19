
let createPokerAPIInternal = require("./PokerTechWasm.js");

export interface IEmbindClass {
  delete: () => void;
}

export interface SingleEquityResult {
  win: number;
  tie: number;
  equity: number;
}

export interface SpecificHandResults {
  cards: string;
  eq: SingleEquityResult;
}

export interface EquityPlayerResult {
  eq: SingleEquityResult;
  specificHandResults: SpecificHandResults[];
}

export interface EquityCalcResult {
  playerResults: EquityPlayerResult[];
  diagnostic?: any;
  result: string;
}

export enum CardRanks {
  Rank_2 = 0,
  Rank_3,
  Rank_4,
  Rank_5,
  Rank_6,
  Rank_7,
  Rank_8,
  Rank_9,
  Rank_10,
  Rank_J,
  Rank_Q,
  Rank_K,
  Rank_A,
  kRankCardCount,
  kRankInvalid = -1
}

export const CardRankToChar = [
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  'T',
  'J',
  'Q',
  'K',
  'A',
];

export enum CardSuit {
  Club = 0,
  Spade,
  Diamond,
  Heart,
  kSuitCount,
  kCardSuitInvalid = -1
};

export const CardSuitToChar = [
  'c',
  's',
  'd',
  'h'
];

export enum RankComboCategory {
  All = 0,
  Suited,
  Unsuited
}

export interface IEnabledRange extends IEmbindClass {
  Clear(): () => void;
  GetRankRatio: () => (r1: CardRanks, r2: CardRanks, combos: RankComboCategory) => number;
  SetCombos: () => (r1: CardRanks, r2: CardRanks, combos: RankComboCategory, weight: number) => void;
  CalculateRangeString: () => string;
}

export interface IPokerAPI {
  CalculateEquity: (ranges: string[], board: string, deadCards: string, onProgressUpdate: (progress: number) => boolean) => Promise<EquityCalcResult>;
  CalculateComplexityEstimate: (boardCount: number, rangeHandCounts: number[], hasDeadCards: boolean) => WorkloadEstimate;
  GetRangeStringHandCount: (range: string) => number;
  IsValidRangeString: (range: string) => boolean;
  IsValidBoardString: (board: string) => boolean;
  CreateEnabledRange: () => IEnabledRange;
}

// Theses are used by the utility functions within package and generally not used by the end user
export interface IPokerAPIInternal extends IPokerAPI {
  SyntaxToEnabledRange: (syntaxString: string) => number[];
  EnabledRangeToSyntax: (rangeArray: number[]) => string;
  IsThreadingSupported: () => boolean;
  GetEmAPI: () => {};
}

export enum WorkLoadEstimateSize {
  Instant,
  Low,
  Medium,
  High,
  VeryHigh
}

export interface WorkloadEstimate {
  workload: WorkLoadEstimateSize;
  handEvaluations: number;
  handComparisons: number;
}

export let gPokerAPIInternal: IPokerAPIInternal | null = null;

let gCurrentLoadPromise: Promise<IPokerAPI> | null = null;

export async function createPokerAPI(libPath?: string): Promise<IPokerAPI> {

  if (gCurrentLoadPromise === null)
    gCurrentLoadPromise = loadPokerLibrary(libPath);

  return gCurrentLoadPromise
}

async function loadPokerLibrary(libPath?: string): Promise<IPokerAPI> {

  let emAPI: any = null;

  const params = {
    locateFile: (path: string, scriptDir: string) => { return libPath ?? (scriptDir + path); }
  }

  emAPI = await createPokerAPIInternal(params);
  emAPI.InitializePokerLib();

  function vectorToArray(v: any) {
    let result: any[] = [];
    for (let i = 0; i < v.size(); i++) {
      result.push(v.get(i));
    }
    return result;
  }

  function arrayToStringVector(stringArray: any) {
    var r = new emAPI.StringList();
    for (var i = 0; i < stringArray.length; i++) {
      r.push_back(stringArray[i]);
    }
    return r;
  }

  function arrayToIntVector(intArray: any) {
    var r = new emAPI.IntList();
    for (var i = 0; i < intArray.length; i++) {
      r.push_back(intArray[i]);
    }
    return r;
  }

  function arrayToFloatVector(floatArray: number[]) {
    var r = new emAPI.FloatList();
    for (var i = 0; i < floatArray.length; i++) {
      r.push_back(floatArray[i]);
    }
    return r;
  }

  function IsThreadingSupported() {
    return "PThread" in emAPI;
  }

  /*function EquityCalculateAsync(ranges, board, token, onProgressUpdate) {
    if (!IsThreadingSupported())
      throw "The current API was not compiled with threading enabled";
    return new Promise((accept, reject) => {
      let rangeList = arrayToStringVector(ranges);
      let op = new emAPI.CalculateEquityEmOperation(rangeList, board);
      let progress = -1.0;
      let callback = { cb: null, intr: null };
      callback.cb = () => {
        if (op.status == emAPI.OperationStatus.InProgress) {
          if (onProgressUpdate && progress != op.progress) {
            progress = op.progress;
            onProgressUpdate(progress);
            //setTimeout(callback.cb, 33);
          }
        } else {
          if (op.status == emAPI.OperationStatus.Failed) {
            console.log("Failed!");
            reject();
          } else if (op.status == emAPI.OperationStatus.Completed) {
            console.log("Done!");
            r = vectorToArray(op.results);
            accept(r);
          }
          clearInterval(callback.intr);

          op.delete();
        }
      };
      callback.intr = setInterval(callback.cb, 33);
    });
  }*/

  function isPromise(p: any | null) {
    if (
      typeof p === "object" &&
      typeof p.then === "function" &&
      typeof p.catch === "function"
    ) {
      return true;
    }

    return false;
  }

  async function CalculateEquity(
    ranges: string[],
    board: string,
    deadCards: string,
    onProgressUpdate: (progress: number) => boolean
  ): Promise<EquityCalcResult> {
    let rangesList = arrayToStringVector(ranges);
    let progressCallback = (p: number) => {
      if (onProgressUpdate) {
        let v = onProgressUpdate(p);
        if (v == false) return false;
      }
      return true;
    };
    let resultV = emAPI.CalculateEquity(
      rangesList,
      board,
      deadCards,
      progressCallback
    );
    if (isPromise(resultV)) resultV = await resultV;

    let obj: EquityCalcResult = JSON.parse(resultV);
    return obj;
  }

  function CalculateComplexityEstimate(boardCardCount: number, playerHandRangesCount: number[], hasDeadCards: boolean): WorkloadEstimate {
    let rangesList = arrayToIntVector(playerHandRangesCount);
    let resultV = emAPI.CalculateComplexityEstimate(boardCardCount, rangesList, hasDeadCards);
    if (resultV == "") throw "Invalid input estimate";
    return JSON.parse(resultV) as WorkloadEstimate;
  }

  function SyntaxToEnabledRange(syntaxString: string): number[] {
    let vectorFloats = emAPI.SyntaxToEnabledRange(syntaxString);
    const r = vectorToArray(vectorFloats);
    vectorFloats.delete();
    return r;
  }

  function EnabledRangeToSyntax(values: number[]): string {
    const fv = arrayToFloatVector(values);
    const r = emAPI.EnabledRangeToSyntax(fv);
    fv.delete();
    return r;
  }

  const pokerAPI: IPokerAPI = {
    CalculateEquity,
    CalculateComplexityEstimate,
    GetRangeStringHandCount: (s) => emAPI.GetRangeStringHandCount(s),
    IsValidRangeString: (s) => emAPI.IsValidRangeString(s),
    IsValidBoardString: (s) => emAPI.IsValidBoardString(s),
    CreateEnabledRange: () => emAPI.CreateEnabledRange()
  };

  gPokerAPIInternal =
  {
    ...pokerAPI,
    IsThreadingSupported,
    GetEmAPI: () => emAPI,
    SyntaxToEnabledRange,
    EnabledRangeToSyntax,
    //CalculateEquitySync: (ranges : string[], board : string) => CalculateEquitySync(ranges, board),
  };


  return gPokerAPIInternal;
}
