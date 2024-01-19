import { CardRankToChar, CardRanks, CardSuit, CardSuitToChar, gPokerAPIInternal } from "./PokerAPI";

const kTotalNonPairRankCombos: number = 13 * 6;
const kTotalPairEntries: number = 13;
const kPairCombosPerHand: number = 6;
const kNonPairCombosPerHand: number = 16;
const kTotalPairCombos: number = kTotalPairEntries * kPairCombosPerHand;
const kTotalNonPairCombos: number = kTotalNonPairRankCombos * kNonPairCombosPerHand;
const kTotalRangeEntries: number = kTotalPairCombos + kTotalNonPairCombos;

export type CardSuitPair = [CardSuit, CardSuit];

export enum ComboSelectorMask {
    All,
    Suited,
    Unsuited,
    Pairs,
    Count
};

function GetIndexWithinBlock(suits: CardSuitPair, isPair: boolean): number {
    if (isPair) {
        if (suits[0] > suits[1])
            [suits[0], suits[1]] = [suits[1], suits[0]]
        if (suits[0] === 0)
            return suits[1] - 1;
        if (suits[0] === 1)
            return 3 + suits[1] - 2;
        return 5;
    }
    else {
        return suits[0] * 4 + suits[1];
    }
}

function GetBlockIndex(r1: CardRanks, r2: CardRanks): number {
    if (r1 > r2)
        [r1, r2] = [r2, r1]

    if (r1 === r2) {
        return kTotalNonPairCombos + r1 * kPairCombosPerHand;
    }

    const entriesInPreviousRow: number = (CardRanks.kRankCardCount - r1);
    const entriesFirstRow: number = CardRanks.kRankCardCount - 1;
    const rowAboveCount: number = r1;

    const idx: number = ((entriesFirstRow + entriesInPreviousRow) * rowAboveCount) / 2;
    return (idx + (r2 - r1 - 1)) * kNonPairCombosPerHand;
}

function CreateMaskToSuitCombos(): CardSuitPair[][] {
    const allSuitsCombosIndicies = Array.from(Array(16).keys());
    const allSuitsCombos: CardSuitPair[] = allSuitsCombosIndicies.map(x => [x % 4, Math.floor(x / 4)]);
    const allPairCombos: CardSuitPair[] = [
        [0, 1],
        [0, 2],
        [0, 3],
        [1, 2],
        [1, 3],
        [2, 3],
    ];
    return [
        allSuitsCombos,
        allSuitsCombos.filter(x => x[0] === x[1]),
        allSuitsCombos.filter(x => x[0] !== x[1]),
        allPairCombos
    ];
}


const maskToSuitCombos = CreateMaskToSuitCombos();
const maskToBlockIndicies: number[][] = maskToSuitCombos.map(
    (x, idx) =>
        x.map(y => GetIndexWithinBlock(y, idx === ComboSelectorMask.Pairs))
);


export default function GetSuitComboSelectorMask(mask: ComboSelectorMask): CardSuitPair[] {
    return maskToSuitCombos[mask];
}

export type Card = {
    rank: CardRanks;
    suit: CardSuit;
};

export type H2 = [Card, Card];

export function GetHandName(hand: H2) {
    return `${CardRankToChar[hand[0].rank]}${CardSuitToChar[hand[0].suit]}${CardRankToChar[hand[1].rank]}${CardSuitToChar[hand[1].suit]}`
}

function GetSpecificHandIndex(hand: H2): number {
    const bIdx: number = GetBlockIndex(hand[0].rank, hand[1].rank);
    const subIdx: number = GetIndexWithinBlock([hand[0].suit, hand[1].suit], hand[0].rank === hand[1].rank);
    return bIdx + subIdx;
}

function GetSuitIndicesForSelector(mask: ComboSelectorMask): number[] {
    return maskToBlockIndicies[mask];
}

function ThrowIfInvalidMask(isPair: boolean, mask: ComboSelectorMask) {
    if (isPair && mask !== ComboSelectorMask.Pairs && mask !== ComboSelectorMask.All)
        throw "Trying to get invalid mask for pair";

}

export class EnabledRange {
    hands: number[];

    constructor() {
        this.hands = Array(kTotalRangeEntries).fill(0);
    }

    clone(): EnabledRange {
        let newRange = new EnabledRange();
        newRange.hands = [...this.hands];
        return newRange;
    }

    getBlockWeight(r1: CardRanks, r2: CardRanks, control: ComboSelectorMask): number {
        const m = GetSuitIndicesForSelector(control);
        const bIdx = GetBlockIndex(r1, r2);
        let sum: number = 0;
        for (let i = 0; i < m.length; i++) {
            sum += this.hands[bIdx + m[i]];
        }
        return sum / m.length;
    }

    getBlockWeights(r1: CardRanks, r2: CardRanks, mask: ComboSelectorMask): number[] {

        ThrowIfInvalidMask(r1 === r2, mask)
        const m = GetSuitIndicesForSelector(mask);
        const bIdx = GetBlockIndex(r1, r2);
        return m.map(i => this.hands[bIdx + i]);
    }

    setBlockWeight(r1: CardRanks, r2: CardRanks, mask: ComboSelectorMask, weight: number) {
        ThrowIfInvalidMask(r1 === r2, mask)
        const m = GetSuitIndicesForSelector(mask);
        const bIdx = GetBlockIndex(r1, r2);
        for (let i = 0; i < m.length; i++) {
            this.hands[bIdx + m[i]] = weight;
        }
    }

    getHandWeight(h: H2): number {
        return this.hands[GetSpecificHandIndex(h)];
    }

    setHandWeight(h: H2, weight: number) {
        this.hands[GetSpecificHandIndex(h)] = weight;
    }

    clear() {
        this.hands = Array(kTotalRangeEntries).fill(0);
    }

    ToJson(): string {
        return JSON.stringify(this.hands);
    }

    SetFromSyntaxString(syntaxString: string) {
        this.hands = gPokerAPIInternal!.SyntaxToEnabledRange(syntaxString);
    }

    GetSyntaxString(): string {
        return gPokerAPIInternal!.EnabledRangeToSyntax(this.hands);
    }

}

