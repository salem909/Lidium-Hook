#pragma once
#include <utils/char_conversion/char_conversion.hpp>
#include <Shlwapi.h>
#include <ImageHlp.h>
#include <PsApi.h>

#pragma comment(lib, "DbgHelp.lib")



#define LOG(...) fwprintf(sgLogFile, __VA_ARGS__)

std::string GetCurrentTimeForFileName()
{
	auto time = std::time(nullptr);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&time), "%F_%T"); // ISO 8601 without timezone information.
	auto s = ss.str();
	std::replace(s.begin(), s.end(), ':', '-');
	return s;
}

char* gen_random(char* s, size_t len)
{
	for (size_t i = 0; i < len; ++i)
	{
		int randomChar = rand() % (26 + 26 + 10);
		if (randomChar < 26)
			s[i] = 'a' + randomChar;
		else if (randomChar < 26 + 26)
			s[i] = 'A' + randomChar - 26;
		else
			s[i] = '0' + randomChar - 26 - 26;
	}
	s[len] = 0;
	return s;
}



static void ReadKey(HKEY hKey, const char* ValueName, char* Buffer, DWORD size)
{
	DWORD dwType;
	LSTATUS ret = RegQueryValueExA(hKey, ValueName, NULL, &dwType, (LPBYTE)Buffer, &size);
	if (ret != ERROR_SUCCESS || dwType != REG_SZ)
		Buffer[0] = '\0';
}

std::wstring get_win_product_name()
{
	std::wstring result = L"Unknown Windows Product Name.";
	HKEY hkey;
	DWORD dwType, dwSize;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, (L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		wchar_t p_name_str[1024];

		dwType = REG_SZ;
		dwSize = sizeof(p_name_str);

		if (RegQueryValueExW(hkey, (L"ProductName"), NULL, &dwType, (PBYTE)& p_name_str, &dwSize) == ERROR_SUCCESS)
			result = p_name_str;

		RegCloseKey(hkey);
	}
	return result;
}



HMODULE GetThisDllHandle()
{
	MEMORY_BASIC_INFORMATION info;
	size_t len = VirtualQueryEx(GetCurrentProcess(), (void*)GetThisDllHandle, &info, sizeof(info));
	if (len != sizeof(info)) return NULL;
	return len ? (HMODULE)info.AllocationBase : NULL;
}

UINT GetComputerManufacturer(LPSTR lpBuffer, UINT uSize)
{
	HKEY   hkData;
	HANDLE hHeap;
	LPSTR  lpString = NULL;
	LPBYTE lpData = NULL;
	DWORD  dwType = 0, dwSize = 0;
	UINT   uIndex, uStart, uEnd, uString, uLength, uState = 0;
	LONG   lErr;

	if ((lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\mssmbios\\Data"),
		0, KEY_QUERY_VALUE, &hkData)) != ERROR_SUCCESS) {
		SetLastError(lErr);
		return 0;
	}
	if ((lErr = RegQueryValueEx(hkData, TEXT("SMBiosData"), NULL, &dwType, NULL, &dwSize)) == ERROR_SUCCESS) {
		if (dwSize == 0 || dwType != REG_BINARY) lErr = ERROR_BADKEY;
		else {
			hHeap = GetProcessHeap();
			lpData = (LPBYTE)HeapAlloc(hHeap, 0, dwSize);
			if (!lpData) lErr = ERROR_NOT_ENOUGH_MEMORY;
			else lErr = RegQueryValueEx(hkData, TEXT("SMBiosData"),
				NULL, NULL, lpData, &dwSize);
		}
	}
	RegCloseKey(hkData);

	if (lErr == ERROR_SUCCESS) {
		uIndex = 8 + *(WORD*)(lpData + 6);
		uEnd = 8 + *(WORD*)(lpData + 4);
		while (lpData[uIndex] != 0x7F && uIndex < uEnd) {
			uIndex += lpData[(uStart = uIndex) + 1];
			uString = 1;
			do {
				if (lpData[uStart] == 0x01 && uState == 0) {
					if (lpData[uStart + 4] == uString ||
						lpData[uStart + 5] == uString ||
						lpData[uStart + 6] == uString) {
						lpString = (LPSTR)(lpData + uIndex);
						if (!StrCmpIA(lpString, "System manufacturer")) {
							lpString = NULL;
							uState++;
						}
					}

				}
				else if (lpData[uStart] == 0x02 && uState == 1) {
					if (lpData[uStart + 4] == uString ||
						lpData[uStart + 5] == uString ||
						lpData[uStart + 6] == uString)
						lpString = (LPSTR)(lpData + uIndex);

				}
				else if (lpData[uStart] == 0x03 && uString == 1) {
					switch (lpData[uStart + 5])
					{
					default:   lpString = "(Other)";               break;
					case 0x02: lpString = "(Unknown)";             break;
					case 0x03: lpString = "(Desktop)";             break;
					case 0x04: lpString = "(Low Profile Desktop)"; break;
					case 0x06: lpString = "(Mini Tower)";          break;
					case 0x07: lpString = "(Tower)";               break;
					case 0x08: lpString = "(Portable)";            break;
					case 0x09: lpString = "(Laptop)";              break;
					case 0x0A: lpString = "(Notebook)";            break;
					case 0x0E: lpString = "(Sub Notebook)";        break;
					}

				}
				if (lpString != NULL) {
					uLength = strlen(lpString) + 1;
					if (uSize > uLength + 1)
						lpBuffer += wsprintfA(lpBuffer, "%s ", lpString);
					uSize -= uLength;
					lpString = NULL;
				}
				uString++;
				while (lpData[uIndex++]);
			} while (lpData[uIndex] && uIndex < uEnd);
			uIndex++;
		}
	}

	if (lpData)
		HeapFree(hHeap, 0, lpData);
	SetLastError(lErr);
	return uSize;
}


BOOL ShowThreadStack(HANDLE hThread, CONTEXT* c, FILE* sgLogFile)
{
	STACKFRAME stFrame = { 0 };
	DWORD dwSymOptions, dwFrameNum = 0, dwMachine, dwOffsetFromSym = 0;
	IMAGEHLP_LINE Line = { 0 };
	IMAGEHLP_MODULEW Module = { 0 };
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
	BYTE pbSym[sizeof(_IMAGEHLP_SYMBOLW) + 1024];
	BYTE pbSym2[sizeof(IMAGEHLP_SYMBOL) + 1024];
	_IMAGEHLP_SYMBOLW* pSym = (_IMAGEHLP_SYMBOLW*)& pbSym;
	wchar_t szUndecName[1024], szUndecFullName[1024];
	wchar_t msglog[4096];

	IMAGEHLP_SYMBOL* pSym2 = (IMAGEHLP_SYMBOL*)& pbSym2;


	/*
	if ( ! GetThreadContext( hThread, &c ) )
	{
		SetError (NULL, NULL, 0);
		LOG (("Cannot get thread context%d\n", GetLastError()));
		return FALSE;
	}
	*/
	wsprintfW(msglog, L"Handler DLL base address: %08X\n", (unsigned int)GetThisDllHandle()); LOG(msglog);
	if (!SymInitialize(hProcess, NULL, TRUE))
	{
		wsprintfW(msglog, L"Cannot initialize symbol engine (%08X), trying again without invading process\n", GetLastError()); LOG(msglog);
		if (!SymInitialize(hProcess, NULL, FALSE))
		{
			wsprintfW(msglog, L"Cannot initialize symbol engine (%08X)\n", GetLastError()); LOG(msglog);
			return FALSE;
		}
	}

	dwSymOptions = SymGetOptions();
	dwSymOptions |= SYMOPT_LOAD_LINES;
	dwSymOptions &= ~SYMOPT_UNDNAME;
	SymSetOptions(dwSymOptions);

	stFrame.AddrPC.Mode = AddrModeFlat;
	dwMachine = IMAGE_FILE_MACHINE_I386;
	stFrame.AddrPC.Offset = c->Eip;
	stFrame.AddrStack.Offset = c->Esp;
	stFrame.AddrStack.Mode = AddrModeFlat;
	stFrame.AddrFrame.Offset = c->Ebp;
	stFrame.AddrFrame.Mode = AddrModeFlat;

	Module.SizeOfStruct = sizeof(Module);
	Line.SizeOfStruct = sizeof(Module);

	wsprintfW(msglog, L"\n--# FV EIP----- RetAddr- FramePtr StackPtr Symbol\n"); LOG(msglog);
	do
	{
		SetLastError(0);
		if (!StackWalk(dwMachine, hProcess, hThread, &stFrame, c, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL))
		{
			wsprintfW(msglog, L"Last error after stack walk finished: %08X", GetLastError()); LOG(msglog);
			break;
		}

		wsprintfW(msglog, L"\n%3d %c%c %08lx %08lx %08lx %08lx ",
			dwFrameNum, stFrame.Far ? 'F' : '.', stFrame.Virtual ? 'V' : '.',
			stFrame.AddrPC.Offset, stFrame.AddrReturn.Offset,
			stFrame.AddrFrame.Offset, stFrame.AddrStack.Offset); LOG(msglog);

		if (stFrame.AddrPC.Offset == 0)
		{
			wsprintfW(msglog, L"(-nosymbols-)\n"); LOG(msglog);
		}
		else
		{ // we seem to have a valid PC
			if (!SymGetSymFromAddr(hProcess, stFrame.AddrPC.Offset, &dwOffsetFromSym, pSym2))
			{
				if (GetLastError() != 487)
				{
					wsprintfW(msglog, L"Unable to get symbol from addr (%08X)\n", GetLastError()); LOG(msglog);
				}
			}
			else
			{
				UnDecorateSymbolNameW(pSym->Name, szUndecName, 1024, UNDNAME_NAME_ONLY);
				UnDecorateSymbolNameW(pSym->Name, szUndecFullName, 1024, UNDNAME_COMPLETE);
				wsprintfW(msglog, L"%s", szUndecName); LOG(msglog);
				if (dwOffsetFromSym) wsprintfW(msglog, L" %+ld bytes", (long)dwOffsetFromSym); LOG(msglog);
				wsprintfW(msglog, L"\n    Sig:  %s\n    Decl: %s\n", pSym->Name, szUndecFullName); LOG(msglog);
			}

			if (!SymGetLineFromAddr(hProcess, stFrame.AddrPC.Offset, &dwOffsetFromSym, &Line))
			{
				if (GetLastError() != 487)
				{
					wsprintfW(msglog, L"Unable to get line from addr (%08X)\n", GetLastError()); LOG(msglog);
				}
			}
			else
			{
				wsprintfW(msglog, L"    Line: %s(%lu) %+ld bytes\n", Line.FileName, Line.LineNumber, dwOffsetFromSym); LOG(msglog);
			}

			if (!SymGetModuleInfoW(hProcess, stFrame.AddrPC.Offset, &Module))
			{
				if (GetLastError() != 487)
				{
					wsprintfW(msglog, L"Unable to get module info (%08X)\n", GetLastError()); LOG(msglog);
				}
			}
			else
			{
				wchar_t ty[256];

				switch (Module.SymType)
				{
				case SymNone:
					wcscpy_s(ty, L"-nosymbols-");
					break;
				case SymCoff:
					wcscpy_s(ty, L"COFF");
					break;
				case SymCv:
					wcscpy_s(ty, L"CV");
					break;
				case SymPdb:
					wcscpy_s(ty, L"PDB");
					break;
				case SymExport:
					wcscpy_s(ty, L"-exported-");
					break;
				case SymDeferred:
					wcscpy_s(ty, L"-deferred-");
					break;
				case SymSym:
					wcscpy_s(ty, L"SYM");
					break;
				default:
					_snwprintf_s(ty, sizeof ty, L"symtype=%ld", (long)Module.SymType);
					break;

				}
				wsprintfW(msglog, L"    Mod:  %s[%s], base: %08lxh\n", Module.ModuleName, Module.ImageName, Module.BaseOfImage); LOG(msglog);
				wsprintfW(msglog, L"    Sym:  type: %s, file: %s\n", ty, Module.LoadedImageName); LOG(msglog);
			}
		}
		dwFrameNum++;
	} while (stFrame.AddrReturn.Offset);
	SymCleanup(hProcess);
	CloseHandle(hProcess);

	return TRUE;
}


LONG WINAPI OurCrashHandler(EXCEPTION_POINTERS* pExcept)
{
	const DWORD exceptionCode = pExcept->ExceptionRecord->ExceptionCode;
	//const PVOID exceptionAddress = pExcept->ExceptionRecord->ExceptionAddress;
	wchar_t* FaultTx;
	FILE* sgLogFile = nullptr;

	PROCESS_MEMORY_COUNTERS pmc;
	MEMORYSTATUS	MS;
	GlobalMemoryStatus(&MS);
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));


	SYSTEMTIME LocalTime;
	GetLocalTime(&LocalTime);

	SYSTEM_INFO info;
	GetSystemInfo(&info);

	//if (pExcept->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
	//return EXCEPTION_CONTINUE_SEARCH;

	if (exceptionCode & APPLICATION_ERROR_MASK) //Ingore custom SEH error codes, if any. (?)
	{
		return ExceptionContinueExecution;
	}
	if (exceptionCode >= 0x40010001 && exceptionCode <= 0x4001000F) //Ingore all DGB Exception Codes.
	{
		return ExceptionContinueExecution;
	}
	if (exceptionCode == 0xE06D7363 || exceptionCode == 0x406D1388) //C++ Threads.
	{
		return ExceptionContinueExecution;
	}
	if (exceptionCode >= 0x80004000 && exceptionCode <= 0x80004FFF)
	{
		return ExceptionContinueExecution;
	}
	if (exceptionCode == EXCEPTION_PRIV_INSTRUCTION)
	{
		return ExceptionContinueExecution;
	}
	if ((DWORD)pExcept->ExceptionRecord->ExceptionAddress >= 0x70000000)
	{
		//LOG(L"crashed on win32 kernals...maybe");
		return ExceptionContinueExecution;
	}

	else
	{

		switch (exceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION: FaultTx = L"ACCESS VIOLATION";		break;
		case EXCEPTION_DATATYPE_MISALIGNMENT: FaultTx = L"DATATYPE MISALIGNMENT"; break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: FaultTx = L"FLT DIVIDE BY ZERO";	break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: FaultTx = L"ARRAY BOUNDS EXCEEDED";	break;
		case EXCEPTION_FLT_DENORMAL_OPERAND: FaultTx = L"FLT DENORMAL OPERAND";	break;
		case EXCEPTION_FLT_INEXACT_RESULT: FaultTx = L"FLT INEXACT RESULT";	break;
		case EXCEPTION_FLT_INVALID_OPERATION: FaultTx = L"FLT INVALID OPERATION";	break;
		case EXCEPTION_FLT_OVERFLOW: FaultTx = L"FLT OVERFLOW";			break;
		case EXCEPTION_FLT_STACK_CHECK: FaultTx = L"FLT STACK CHECK";		break;
		case EXCEPTION_FLT_UNDERFLOW: FaultTx = L"FLT UNDERFLOW";			break;
		case EXCEPTION_ILLEGAL_INSTRUCTION: FaultTx = L"ILLEGAL INSTRUCTION";	break;
		case EXCEPTION_IN_PAGE_ERROR: FaultTx = L"IN PAGE ERROR";			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO: FaultTx = L"INT DEVIDE BY ZERO";	break;
		case EXCEPTION_INT_OVERFLOW: FaultTx = L"INT OVERFLOW";			break;
		case EXCEPTION_INVALID_DISPOSITION: FaultTx = L"INVALID DISPOSITION";	break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:FaultTx = L"NONCONTINUABLE EXCEPTION"; break;
		case EXCEPTION_PRIV_INSTRUCTION: FaultTx = L"PRIVILEGED INSTRUCTION"; break;
		case EXCEPTION_SINGLE_STEP: FaultTx = L"SINGLE STEP";			break;
		case EXCEPTION_STACK_OVERFLOW: FaultTx = L"STACK OVERFLOW";		break;
		case DBG_CONTROL_C:  FaultTx = L"CONTROL C";		break;
			//case DBG_PRINTEXCEPTION_C:
			//case 0x4001000AL: //DBG_PRINTEXCEPTION_WIDE_C
			//case 0x406D1388: //Thread naming
			//case 0xE06D7363: //C++ Exceptions
		default: FaultTx = L"unk";
		}

		DWORD EIP = pExcept->ContextRecord->Eip;
		DWORD ESP = pExcept->ContextRecord->Esp;
		DWORD EBP = pExcept->ContextRecord->Ebp;
		DWORD EBX = pExcept->ContextRecord->Ebx;
		DWORD EAX = pExcept->ContextRecord->Eax;
		DWORD ECX = pExcept->ContextRecord->Ecx;
		DWORD EDX = pExcept->ContextRecord->Edx;
		DWORD ESI = pExcept->ContextRecord->Esi;
		DWORD EDI = pExcept->ContextRecord->Edi;
		DWORD CurrentMem = pmc.WorkingSetSize / 1048576;
		DWORD TotalMem = MS.dwTotalPhys / 1048576;
		CHAR GetCompManu[128];

		if (GetComputerManufacturer(GetCompManu, 128) < 0)
		{
			sprintf(GetCompManu, "System manufacturer : Unknown");
		}
		wchar_t* GetCompManuW = adnf::utils::char_conversion::ansi_to_unicode(GetCompManu, 1252);


		wchar_t message[4096];
		wsprintfW(message, L"MapleStory- An error occurred during execution. The problem description for the occurring issue has been provided here.\nMapleStory.exe has crashed on: \nAddress : %X\nCode : %X\nWin32 : %X\nReason :%-24s\nCurrent Mem : %d\nTotal Mem: %d\nOperating System :%ws \n%s\nWhen: %d/%d/%d @ %02d:%02d:%02d.%d\nEAX=%08X  EBX=%08X  ECX=%08X\nEDX=%08X  EBP=%08X  ESI=%08X\nEDI=%08X  ESP=%08X  EIP=%08X\nSorry for the inconvenience. Please show this message to an administator.\nso that it may be referenced during bug resolution.\n\n\nA log file will also be written in the game's directory after this message closes, Please include the .log files to the Administrators as well. \nPress OK to return to the exit the application.\nYou will be logged off from the game.\n",
			pExcept->ExceptionRecord->ExceptionAddress,
			pExcept->ExceptionRecord->ExceptionCode,
			GetLastError(),
			FaultTx,
			CurrentMem, TotalMem,
			get_win_product_name().c_str(),
			GetCompManuW,
			LocalTime.wDay, LocalTime.wMonth, LocalTime.wYear,
			LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond, LocalTime.wMilliseconds,
			EAX, EBX, ECX, EDX, EBP, ESI, EDI, ESP, EIP);

		MessageBoxW(0, message, L"MS has crashed.", MB_OK);
		std::string fileName = (std::string(".\\LOGS\\CrashMS_") + GetCurrentTimeForFileName() + ".log");
		errno_t errorCode = fopen_s(&sgLogFile, fileName.c_str(), "a+");
		LOG(message);
		ShowThreadStack(GetCurrentThread(), pExcept->ContextRecord, sgLogFile);
		fclose(sgLogFile);

		return EXCEPTION_CONTINUE_SEARCH;
	}


}
