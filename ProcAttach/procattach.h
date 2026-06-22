#pragma once
#include <string>
#include <atomic>

namespace ProcAttachSpace {
	class ProcAttachClass {
	public: 
		static int ProcAttach();
		static void UpdateRankingPointsAndCalculate();
		static void ResetVariables();
	private:
		static DWORD GetProcIDByName(const std::wstring procName);
		static uint32_t PointerReaderChain(uintptr_t pointer, int layers, ...);
		static void UpdateCalcStruct(uint16_t a, uint16_t b);
	};
};