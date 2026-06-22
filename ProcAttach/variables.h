#pragma once
#include <iostream>

namespace CalculatorVars {

     static const char* GRADE[6] = {
        "RN",
        "R4",
        "R3",
        "R2",
        "R1",
        "RS / R1",
    };

    static const int CURRENT_SCREEN = 0x004cb888; //0xce787 FOR RANKING POINT SCREEN
    static const int EL_LIFE_RECORD = 0x4A3F7C; // add 0x00000074 THEN 0x00000160 FOR RANKING POINTS
};

struct CalcStruct {

    int current_rank = 1000;
    int current_points = 0;
    int grade = 0;

    float odds_for_rank1_from_r1 = 0.0f;
    float odds_for_rank6_from_r1 = 0.0f;
    float odds_for_rank1_from_rs = 0.0f;
    float odds_for_rank6_from_rs = 0.0f;

    float odds_for_rank1_general = 0.0f;
    float odds_for_rank6_general = 0.0f;

    float odds_for_rs = 0.0f;
    float odds_for_r1 = 0.0f;
    float odds_for_r2 = 0.0f;
    float odds_for_r3 = 0.0f;

    int points_rank1 = 0;
    int points_rank6 = 0;
    int points_rs = 0;
    int points_r1 = 0;
    int points_r2 = 0;
    int points_r3 = 0;
};

inline CalcStruct calcstruct;