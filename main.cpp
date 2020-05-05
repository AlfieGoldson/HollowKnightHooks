#include <iostream>
#include <Windows.h>
#include <vector>
#include <TlHelp32.h>
#include <map>

const char *WINDOW_NAME = "Hollow Knight";
const char *MODULE_NAME = "UnityPlayer.dll";

std::vector<DWORD> playerBaseOffsets = {0x000D94E4, 0x40, 0x140};
std::map<const char *, DWORD> playerOffsets = {
	{"geos", 0x118},
	{"maxCharms", 0x41C},
	{"soul", 0x120},
	{"charms", 0x420},
	{"maxLives", 0x0e8},
	{"maxLives2", 0x0ec},
	{"lives", 0x0e4},
	{"isOnBench", 0x144},
};

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

	int geos;
	ReadProcessMemory(handle, (LPCVOID)(playerBase + playerOffsets["geos"]), &geos, sizeof(geos), NULL);
	int soul;
	ReadProcessMemory(handle, (LPCVOID)(playerBase + playerOffsets["soul"]), &soul, sizeof(geos), NULL);

	int charms;
	ReadProcessMemory(handle, (LPCVOID)(playerBase + playerOffsets["charms"]), &charms, sizeof(charms), NULL);
	int maxCharms;
	ReadProcessMemory(handle, (LPCVOID)(playerBase + playerOffsets["maxCharms"]), &maxCharms, sizeof(maxCharms), NULL);

	int lives;
	ReadProcessMemory(handle, (LPCVOID)(playerBase + playerOffsets["lives"]), &lives, sizeof(lives), NULL);
	int maxLives;
	ReadProcessMemory(handle, (LPCVOID)(playerBase + playerOffsets["maxLives"]), &maxLives, sizeof(maxLives), NULL);

	std::cout << std::dec << "Geos: " << geos << std::endl;
	std::cout << std::dec << "Soul: " << soul << std::endl;
	std::cout << std::dec << "Charms: " << charms << "/" << maxCharms << (charms > maxCharms ? " [OVERCHARMED!]" : "") << std::endl;
	std::cout << std::dec << "Lives: " << lives << "/" << maxLives << std::endl;

	return 0;
}