#include <iostream>
#include <Windows.h>
#include <vector>
#include <TlHelp32.h>
#include <map>

const char *WINDOW_NAME = "Hollow Knight";
const char *MODULE_NAME = "mono.dll";

std::vector<DWORD> playerBaseOffsets = {0x1F50AC, 0x3B4, 0xC, 0x60};

struct hkPlayer
{
	DWORD geos;
	DWORD soul;
	DWORD maxCharms;
	DWORD charms;
	DWORD maxLives;
	DWORD maxLives2;
	DWORD lives;
	DWORD isOnBench;
};

struct hkPlayer baseOffsets = {0x118, 0x120, 0x41C, 0x420, 0x0E8, 0x0EC, 0x0E4, 0x144};

void errorExit(const char *err)
{
	std::cout << err << std::endl;
	Sleep(3000);
	exit(-1);
}

DWORD GetModuleBase(const char *ModuleName, DWORD ProcessId)
{
	MODULEENTRY32 ModuleEntry = {0};
	HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessId);

	if (!SnapShot)
		return 0;

	ModuleEntry.dwSize = sizeof(ModuleEntry);

	if (!Module32First(SnapShot, &ModuleEntry))
		return 0;

	do
	{
		if (strcmp((const char *)ModuleEntry.szModule, ModuleName) == 0)
		{
			CloseHandle(SnapShot);
			return (DWORD)ModuleEntry.modBaseAddr;
		}
	} while (Module32Next(SnapShot, &ModuleEntry));

	CloseHandle(SnapShot);
	return 0;
}

DWORD FindOffsetAddress(HANDLE pHandle, DWORD baseAddress, std::vector<DWORD> offsets)
{
	if (offsets.size() <= 0)
		return baseAddress;

	DWORD address;
	ReadProcessMemory(pHandle, (LPCVOID)(baseAddress + offsets[0]), &address, sizeof(baseAddress), NULL);

	std::vector<DWORD> subOffsets(offsets.begin() + 1, offsets.end());
	return FindOffsetAddress(pHandle, address, subOffsets);
}

int main()
{
	HWND hwnd = FindWindowA(NULL, WINDOW_NAME);
	if (!hwnd)
		return 1;

	DWORD procId;
	GetWindowThreadProcessId(hwnd, &procId);

	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);

	if (!procId)
		return 1;

	DWORD unityPlayerBase = GetModuleBase(MODULE_NAME, procId);
	std::cout << "Module Base Address: 0x" << std::hex << unityPlayerBase << std::endl;

	DWORD playerBase = FindOffsetAddress(handle, unityPlayerBase, playerBaseOffsets);
	std::cout << "Player Base Address: 0x" << std::hex << playerBase << std::endl;

	struct hkPlayer currentPlayer = {
		playerBase + baseOffsets.geos,
		playerBase + baseOffsets.soul,
		playerBase + baseOffsets.maxCharms,
		playerBase + baseOffsets.charms,
		playerBase + baseOffsets.maxLives,
		playerBase + baseOffsets.maxLives2,
		playerBase + baseOffsets.lives,
		playerBase + baseOffsets.isOnBench};

	int geos, soul, charms, maxCharms, lives, maxLives;
	ReadProcessMemory(handle, (LPCVOID)(currentPlayer.geos), &geos, sizeof(int), NULL);
	ReadProcessMemory(handle, (LPCVOID)(currentPlayer.soul), &soul, sizeof(int), NULL);
	ReadProcessMemory(handle, (LPCVOID)(currentPlayer.charms), &charms, sizeof(int), NULL);
	ReadProcessMemory(handle, (LPCVOID)(currentPlayer.maxCharms), &maxCharms, sizeof(int), NULL);
	ReadProcessMemory(handle, (LPCVOID)(currentPlayer.lives), &lives, sizeof(int), NULL);
	ReadProcessMemory(handle, (LPCVOID)(currentPlayer.maxLives), &maxLives, sizeof(int), NULL);

	std::cout << std::dec << "Geos: " << geos << std::endl;
	std::cout << std::dec << "Soul: " << soul << std::endl;
	std::cout << std::dec << "Charms: " << charms << "/" << maxCharms << (charms > maxCharms ? " [OVERCHARMED!]" : "") << std::endl;
	std::cout << std::dec << "Lives: " << lives << "/" << maxLives << std::endl;

	return 0;
}