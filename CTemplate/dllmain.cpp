
#include "pch.h"

// BE AWARE ===v
// in order to reference other projects you need to add:
// $(SolutionDir)Common;%(AdditionalIncludeDirectories)
// to project properties -> c/c++ -> additional include directories
#include "memedit.h"
#include "hooker.h"
#include "Common.h"
#include "winhooks.h"
#include "ExampleHooks.h"
#include <cstdint>
#include <iostream>
#include <VirtualizerSDK/VirtualizerSDK.h>
//#pragma optimize("", off)
//#pragma optimize("", on)

using namespace std;
							
#define CRC_MAPLESTORY		0x8CA914E8

ULONG crc32_table[256];
ULONG ulPolynomial = 0x04c11db7;
ULONG Reflect(ULONG ref, char ch)
{
	ULONG value(0);
	for (int i = 1; i < (ch + 1); i++)
	{
		if (ref & 1)
			value |= 1 << (ch - i);
		ref >>= 1;
	}
	return value;
}

long FileSize(FILE* input)
{
	long fileSizeBytes;
	fseek(input, 0, SEEK_END);
	fileSizeBytes = ftell(input);
	fseek(input, 0, SEEK_SET);
	return fileSizeBytes;
}

void InitCrcTable()
{
	for (int i = 0; i <= 0xFF; i++)
	{
		crc32_table[i] = Reflect(i, 8) << 24;
		for (int j = 0; j < 8; j++)
			crc32_table[i] = (crc32_table[i] << 1) ^ (crc32_table[i] & (1 << 31) ? ulPolynomial : 0);
		crc32_table[i] = Reflect(crc32_table[i], 32);
	}

}

unsigned int Get_CRC(unsigned char* buffer, ULONG bufsize)
{
	ULONG  crc(0xffffffff);
	int len;
	len = bufsize;
	for (int i = 0; i < len; i++)
		crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ buffer[i]];

	return crc ^ 0xffffffff;
}

uint32_t check_for_crc(char* filename)
{
	FILE* fp1;
	fp1 = fopen(filename, "rb");
	if (!ferror(fp1))
	{
		char crccheck[256];
		long bufsize = FileSize(fp1), result;
		unsigned char* buffer = new unsigned char[bufsize];
		result = fread(buffer, 1, bufsize, fp1);
		fclose(fp1);
		return Get_CRC(buffer, bufsize);
	}
	return 0;
}

template <typename I> string n2hexstr(I w, size_t hex_len = sizeof(I) << 1) {
	static const char* digits = "0123456789ABCDEF";
	string rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}

VOID WINAPI ShowConsole()
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	char cc[128];
	sprintf_s(cc, "Client: %i", GetCurrentProcessId());
	SetConsoleTitleA(cc);
}

// executed after the client is unpacked
VOID MainFunc()
{
	/* 
	VIRTUALIZER_TIGER_WHITE_START
	WriteValue<BYTE>(0x040A910, 0x55);
	WriteValue<BYTE>(0x040A92E, 0x50);
	VIRTUALIZER_TIGER_WHITE_END
	return;
	*/

	return;
}

#pragma region EntryThread

static Common* CommonHooks;

// main thread
VOID MainProc()
{
	/*
	VIRTUALIZER_FISH_RED_START
	InitCrcTable();
	char exe[] = "MapleStory.exe";
	string h = n2hexstr(CRC_MAPLESTORY);
	string e = n2hexstr(check_for_crc(exe));
	if (check_for_crc(exe) != CRC_MAPLESTORY) { exit(0); }
	VIRTUALIZER_FISH_RED_END
	*/

	MainFunc();
}
// dll entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&MainProc, NULL, 0, 0);
		break;
	}
	case DLL_PROCESS_DETACH:
	{
		cout << "DLL_PROCESS_DETACH";
		CommonHooks->~Common();
		break;
	}
	}
	return TRUE;
}

#pragma endregion