#include <Windows.h>
#include <iostream>
#include <thread>
#include <Psapi.h>
#include <TlHelp32.h>
#include "variables.h"
#include "procattach.h"
#include <string>

// PROCESS ATTACH FUNCTIONS
const std::wstring PCSX2_PROC_NAME = L"pcsx2-qt.exe";

// PROCATTACH BEGINS HERE

uintptr_t eeMemBase = 0;
HANDLE hProc;

DWORD ProcAttachSpace::ProcAttachClass::GetProcIDByName(const std::wstring procName) {
    DWORD prodID = 0; // init proc id

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // snapshot of current processes open

    PROCESSENTRY32W process; // object to hold information about a single proc
    process.dwSize = sizeof(process); // doesnt work without this

    if (Process32FirstW(snap, &process)) {
        do {
            if (!_wcsicmp(process.szExeFile, procName.c_str())) { // compares exe file names, from snapshot to provided name
                prodID = process.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &process)); // do loop till there is no longer another process in snapshot
    }
    CloseHandle(snap); // close handle, free from memory
    return prodID;
}

void ProcAttachSpace::ProcAttachClass::UpdateRankingPointsAndCalculate() {
    uintptr_t INGAME_CURRENT_SCREEN = eeMemBase + CalculatorVars::CURRENT_SCREEN;
    uintptr_t INGAME_EL_LIFE_RECORD = eeMemBase + CalculatorVars::EL_LIFE_RECORD;

    uint32_t ingame_current_screen;

    uint16_t current_ranking_points;
    uint16_t current_rank;

    ReadProcessMemory(
        hProc,
        (LPCVOID)(INGAME_CURRENT_SCREEN),
        &ingame_current_screen,
        sizeof(ingame_current_screen),
        nullptr
    );

    if (ingame_current_screen == 0xce787) {

        uint32_t life_record_ptr = 0;

        ReadProcessMemory(
            hProc,
            (LPCVOID)(INGAME_EL_LIFE_RECORD),
            &life_record_ptr,
            sizeof(life_record_ptr),
            nullptr
        );

        uint32_t ranking_data_ptr = 0;

        ReadProcessMemory(
            hProc,
            (LPCVOID)(eeMemBase + life_record_ptr + 0x74),
            &ranking_data_ptr,
            sizeof(ranking_data_ptr),
            nullptr
        );

        ReadProcessMemory(
            hProc,
            (LPCVOID)(eeMemBase + ranking_data_ptr + 0x160),
            &current_ranking_points,
            sizeof(current_ranking_points),
            nullptr
        );

        ReadProcessMemory(
            hProc,
            (LPCVOID)(eeMemBase + ranking_data_ptr + 0x015c),
            &current_rank,
            sizeof(current_rank),
            nullptr
        );

        UpdateCalcStruct(current_ranking_points, current_rank);
    }
}

void ProcAttachSpace::ProcAttachClass::UpdateCalcStruct(uint16_t current_ranking_points, uint16_t current_rank) {

    // updating struct and calculating everything
    calcstruct.current_points = current_ranking_points;
    calcstruct.current_rank = current_rank;

    int grade_poins = 500;

    if (current_rank <= 50) {
        calcstruct.grade = 5;
    }
    else if (current_rank <= 300 && current_rank > 50) {
        grade_poins = 200;
        calcstruct.grade = 4;
    }
    else if (current_rank <= 500 && current_rank > 300) {
        grade_poins = 100;
        calcstruct.grade = 3;
    }
    else if (current_rank <= 800 && current_rank > 500) {
        grade_poins = 50;
        calcstruct.grade = 2;
    }
    else if (current_rank == 1000) {
        grade_poins = 10;
        calcstruct.grade = 0;
    }
    else {
        grade_poins = 20;
        calcstruct.grade = 1;
    }

    calcstruct.points_rank1 = 10800 <= current_ranking_points ? 0 : 10800 - current_ranking_points;
    calcstruct.points_rank6 = 8700 <= current_ranking_points ? 0 : 8700 - current_ranking_points;
    calcstruct.points_rs = 4300 <= current_ranking_points ? 0 : 4300 - current_ranking_points;
    calcstruct.points_r1 = 2300 <= current_ranking_points ? 0 : 2300 - current_ranking_points;
    calcstruct.points_r2 = 1150 <= current_ranking_points ? 0 : 1150 - current_ranking_points;
    calcstruct.points_r3 = 369 <= current_ranking_points ? 0 : 369 - current_ranking_points;

    calcstruct.odds_for_rank1_from_r1 = 10800 <= current_ranking_points ? 0.0f : ((float)(10800 - current_ranking_points) / 200);
    calcstruct.odds_for_rank6_from_r1 = 8700 <= current_ranking_points ? 0.0f : ((float)(8700 - current_ranking_points) / 200);
    calcstruct.odds_for_rank1_from_rs = 10800 <= current_ranking_points ? 0.0f : ((float)(10800 - current_ranking_points) / 500);
    calcstruct.odds_for_rank6_from_rs = 8700 <= current_ranking_points ? 0.0f : ((float)(8700 - current_ranking_points) / 500);
    calcstruct.odds_for_rs = 4300 <= current_ranking_points ? 0.0f : ((float)(4300 - current_ranking_points) / grade_poins);
    calcstruct.odds_for_r1 = 2300 <= current_ranking_points ? 0.0f : ((float)(2300 - current_ranking_points) / grade_poins);
    calcstruct.odds_for_r2 = 1150 <= current_ranking_points ? 0.0f : ((float)(1150 - current_ranking_points) / grade_poins);
    calcstruct.odds_for_r3 = 369 <= current_ranking_points ? 0.0f : (((float)(369) - current_ranking_points) / grade_poins);
}

void ProcAttachSpace::ProcAttachClass::ResetVariables() {
    calcstruct.current_rank = 1000;
    calcstruct.current_points = 0;
    calcstruct.grade = 0;

    calcstruct.odds_for_rank1_from_r1 = 0.0f;
    calcstruct.odds_for_rank6_from_r1 = 0.0f;
    calcstruct.odds_for_rank1_from_rs = 0.0f;
    calcstruct.odds_for_rank6_from_rs = 0.0f;
    calcstruct.odds_for_rs = 0.0f;
    calcstruct.odds_for_r1 = 0.0f;
    calcstruct.odds_for_r2 = 0.0f;
    calcstruct.odds_for_r3 = 0.0f;

    calcstruct.points_rank1 = 0;
    calcstruct.points_rank6 = 0;
    calcstruct.points_rs = 0;
    calcstruct.points_r1 = 0;
    calcstruct.points_r2 = 0;
    calcstruct.points_r3 = 0;
}

// Main initializer
int ProcAttachSpace::ProcAttachClass::ProcAttach() {

    // ACTUALLY GETTING ALL PATH AND STUFF

    DWORD procID = GetProcIDByName(PCSX2_PROC_NAME); // gets procID from const str

    if (!procID) { // checks if empty
        return -1;
    }

    hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID); // open process with all perms
    HMODULE hMods[1024]; // array for modules that will come with PCSX2 process
    DWORD cbNeeded; // number of bytes

    EnumProcessModules(hProc, hMods, sizeof(hMods), &cbNeeded); // returns mods from proc

    wchar_t procPath[MAX_PATH];
    GetModuleFileNameExW(hProc, NULL, procPath, MAX_PATH); // returns path for pcsx2-qt.exe

    uintptr_t pcsx2BaseAddress = (uintptr_t)hMods[0]; // gets base address of the main process of PCSX2, that is the main .exe file

    HMODULE pcsx2LocalCopy = LoadLibraryExW(procPath, NULL, DONT_RESOLVE_DLL_REFERENCES); // makes local copy of .exe to work on

    if (!pcsx2LocalCopy) {
        return -1;
    }

    uintptr_t eeMemExportAddress = (uintptr_t)GetProcAddress(pcsx2LocalCopy, "EEmem"); // gets export address of EE memory

    // main offset calculation part

    uintptr_t eeMemExportOffset = eeMemExportAddress - (uintptr_t)pcsx2LocalCopy; // basically offset of EE memory
    uintptr_t eeMemRemoteAddress = pcsx2BaseAddress + eeMemExportOffset; // finally finds real address of EE mem (usually cuz its a pointer)

    // get actual addresses

    eeMemBase = 0;
    ReadProcessMemory(hProc, (LPVOID)eeMemRemoteAddress, &eeMemBase, sizeof(uintptr_t), nullptr); // reads process memory from PCSX2 itself

    return 1;
}
