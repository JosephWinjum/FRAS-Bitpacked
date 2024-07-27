#ifndef INCLUDED_CFG_RANDOM_ACCESS_V2_SD
#define INCLUDED_CFG_RANDOM_ACCESS_V2_SD

#include "cfg/random_access_v2.hpp"
#include <sdsl/bit_vectors.hpp>
#include <sdsl/util.hpp>

namespace cfg {

/**
 * Indexes a CFG for random access using a bit vector.
 * NOTE: this class requires that the CFG rules are in smallest-expansion-first order.
 **/
class RandomAccessV2SD : public RandomAccessV2
{

private:

    sdsl::sd_vector<> startBitvector;
    sdsl::sd_vector<>::rank_1_type startBitvectorRank;
    sdsl::sd_vector<>::select_1_type startBitvectorSelect;

    sdsl::sd_vector<> expansionBitvector;
    sdsl::sd_vector<>::rank_1_type expansionBitvectorRank;

    uint64_t* expansionSizes;

    void initializeBitvectors()
    {
        sdsl::bit_vector tmpStartBitvector(cfg->textLength, 0);
        // startRule = numRules + CFG::ALPHABET_SIZE
        sdsl::bit_vector tmpExpansionBitvector(cfg->startRule, 0);

        // prepare to compute rule sizes
        uint64_t* ruleSizes = new uint64_t[cfg->startRule];
        for (int i = 0; i < CFG::ALPHABET_SIZE; i++) {
            ruleSizes[i] = 1;
        }
        for (int i = CFG::ALPHABET_SIZE; i < cfg->startRule; i++) {
            ruleSizes[i] = 0;
        }

        // set the start bitvector
        uint64_t pos = 0;
        int c;
        for (int i = 0; i < cfg->startSize; i++) {
            c = CFG::unpack(cfg->startRule, i);
            tmpStartBitvector[pos] = 1;
            pos += ruleSize(ruleSizes, c);
        }
        startBitvector = sdsl::sd_vector<>(tmpStartBitvector);

        // set the expansion bitvector and count the number of unique expansions
        uint64_t previousSize = 1;
        int numExpansions = 1;  // 1 will be in the array but not have a bit set
        for (int i = 0; i < cfg->startRule; i++) {
            if (ruleSizes[i] > previousSize) {
                numExpansions++;
                previousSize = ruleSizes[i];
                tmpExpansionBitvector[i] = 1;
            }
        }
        expansionBitvector = sdsl::sd_vector<>(tmpExpansionBitvector);

        // initialize the expansion array
        expansionSizes = new uint64_t[numExpansions];
        previousSize = 1;
        numExpansions = 0;
        expansionSizes[numExpansions++] = previousSize;
        for (int i = 0; i < cfg->startRule; i++) {
            if (ruleSizes[i] > previousSize) {
                previousSize = ruleSizes[i];
                expansionSizes[numExpansions++] = previousSize;
            }
        }

        // clean up
        delete[] ruleSizes;
    }

    uint64_t ruleSize(uint64_t* ruleSizes, int rule)
    {
        if (ruleSizes[rule] != 0) return ruleSizes[rule];

        int c;
        for (int i = 0; CFG::ruleLengths[rule] > i; i++) {
            c = CFG::unpack(rule, i);
            if (ruleSizes[c] == 0) {
                ruleSize(ruleSizes, c);
            }
            ruleSizes[rule] += ruleSizes[c];
        }

        return ruleSizes[rule];
    }

    void rankSelect(uint64_t i, int& rank, uint64_t& select)
    {
        // i+1 because rank is exclusive [0, i) and we want inclusive [0, i]
        rank = startBitvectorRank.rank(i + 1);
        select = startBitvectorSelect.select(rank);
    }

    uint64_t expansionSize(int rule)
    {
        // i+1 because rank is exclusive [0, i) and we want inclusive [0, i]
        int rank = expansionBitvectorRank.rank(rule + 1);
        return expansionSizes[rank];
    }

public:

    uint64_t memSize()
    {
        //uint64_t numExpansions = sdsl::util::cnt_one_bits(expansionBitvector);
        //uint64_t expansionSize = sizeof(uint64_t) * numExpansions;
        //return expansionSize + sizeof(startBitvector) + sizeof(expansionBitvector);
        uint64_t numExpansions = expansionBitvectorRank.rank(expansionBitvector.size());
        uint64_t expansionSize = sizeof(uint64_t) * numExpansions;

        uint64_t startBitvectorSize = sdsl::size_in_bytes(startBitvector);
        uint64_t startBitvectorRankSize = sdsl::size_in_bytes(startBitvectorRank);
        uint64_t startBitvectorSelectSize = sdsl::size_in_bytes(startBitvectorSelect);

        uint64_t expansionBitvectorSize = sdsl::size_in_bytes(expansionBitvector);
        uint64_t expansionBitvectorRankSize = sdsl::size_in_bytes(expansionBitvectorRank);

        return startBitvectorSelectSize + startBitvectorRankSize + startBitvectorSelectSize +
               expansionBitvectorSize + expansionBitvectorRankSize +
               expansionSize;
    }

    RandomAccessV2SD(CFG* cfg): RandomAccessV2(cfg)
    {
        initializeBitvectors();
        startBitvectorRank = sdsl::sd_vector<>::rank_1_type(&startBitvector);
        startBitvectorSelect = sdsl::sd_vector<>::select_1_type(&startBitvector);
        expansionBitvectorRank = sdsl::sd_vector<>::rank_1_type(&expansionBitvector);
    }

    ~RandomAccessV2SD()
    {
        delete[] expansionSizes;
    };

};

}

#endif
