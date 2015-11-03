#define _NO_CVCONST_H
#define _CRT_STDIO_ISO_WIDE_SPECIFIERS

#include <Windows.h>
#include <DbgHelp.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum BasicType
{
	btNoType = 0,
	btVoid = 1,
	btChar = 2,
	btWChar = 3,
	btInt = 6,
	btUInt = 7,
	btFloat = 8,
	btBCD = 9,
	btBool = 10,
	btLong = 13,
	btULong = 14,
	btCurrency = 25,
	btDate = 26,
	btVariant = 27,
	btComplex = 28,
	btBit = 29,
	btBSTR = 30,
	btHresult = 31,
};

#define WIDEN_(x) L ## x
#define WIDEN(x) WIDEN_(x)

typedef struct _StackTrace {
	wchar_t message[2 * 1024 * 1024];
	int written;
	HANDLE process;
	HANDLE thread;
	PCONTEXT contextRecord;
	STACKFRAME64 currentStackFrame;
	bool isFirstParameter;
	LPVOID scratchSpace;
} StackTrace;

bool printBasicType(StackTrace* stackTrace, PSYMBOL_INFOW pSymInfo, ULONG typeIndex, void* valueLocation, bool whilePrintingPointer)
{
	enum BasicType basicType;
	if (!SymGetTypeInfo(stackTrace->process, pSymInfo->ModBase, typeIndex, TI_GET_BASETYPE, &basicType))
	{
		return false;
	}

	switch (basicType)
	{
	case btChar:
	{
		if (!whilePrintingPointer)
		{
			char value;
			ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"'%c'", value);
		}
		else
		{
			char* value = (char*)valueLocation;

			MEMORY_BASIC_INFORMATION pageInfo = { 0 };
			if (VirtualQueryEx(stackTrace->process, value, &pageInfo, sizeof(pageInfo)) == 0)
			{
				return false;
			}

			PVOID pageEndAddress = (char*)pageInfo.BaseAddress + pageInfo.RegionSize;

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"\"");

			for (int charsWritten = 0; charsWritten < 100; )
			{
				if ((void*)value < pageEndAddress)
				{
					char next;
					ReadProcessMemory(stackTrace->process, value, &next, sizeof(next), NULL);

					if (next != '\0')
					{
						if (charsWritten == 100 - 1)
						{
							stackTrace->written +=
								swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
									L"...");
							break;
						}
						else
						{
							stackTrace->written +=
								swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
									L"%c", next);
							charsWritten++;
							value++;
						}
					}
					else
					{
						break;
					}
				}
				else
				{
					if (VirtualQueryEx(stackTrace->process, pageEndAddress, &pageInfo, sizeof(pageInfo)) == 0)
					{
						stackTrace->written +=
							swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
								L"<Bad memory>");
						break;
					}

					pageEndAddress = (char*)pageInfo.BaseAddress + pageInfo.RegionSize;
				}
			}

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"\"");
		}

		break;
	}

	case btWChar:
	{
		if (!whilePrintingPointer)
		{
			wchar_t value;
			ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"'%lc'", value);
		}
		else
		{
			wchar_t* value = (wchar_t*)valueLocation;

			MEMORY_BASIC_INFORMATION pageInfo = { 0 };
			if (VirtualQueryEx(stackTrace->process, value, &pageInfo, sizeof(pageInfo)) == 0)
			{
				return false;
			}

			PVOID pageEndAddress = (char*)pageInfo.BaseAddress + pageInfo.RegionSize;

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"L\"");

			for (int charsWritten = 0; charsWritten < 100; )
			{
				if ((void*)((char*)value + 1) < pageEndAddress)
				{
					wchar_t next;
					ReadProcessMemory(stackTrace->process, value, &next, sizeof(next), NULL);

					if (next != L'\0')
					{
						if (charsWritten == 100 - 1)
						{
							stackTrace->written +=
								swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
									L"...");
							break;
						}
						else
						{
							stackTrace->written +=
								swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
									L"%lc", next);
							charsWritten++;
							value++;
						}
					}
					else
					{
						break;
					}
				}
				else
				{
					if (VirtualQueryEx(stackTrace->process, pageEndAddress, &pageInfo, sizeof(pageInfo)) == 0)
					{
						stackTrace->written +=
							swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
								L"<Bad memory>");
						break;
					}

					pageEndAddress = (char*)pageInfo.BaseAddress + pageInfo.RegionSize;
				}
			}

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"\"");
		}

		break;
	}

	case btInt:
	{
		ULONG64 length;
		if (!SymGetTypeInfo(stackTrace->process, pSymInfo->ModBase, typeIndex, TI_GET_LENGTH, &length))
		{
			return false;
		}

		switch (length)
		{
		case sizeof(int8_t) :
		{
			int8_t value;
			ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"%" WIDEN(PRId8), value);
			break;
		}

		case sizeof(int16_t) :
		{
			int16_t value;
			ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"%" WIDEN(PRId16), value);
			break;
		}

		case sizeof(int32_t) :
		{
			int32_t value;
			ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"%" WIDEN(PRId32), value);
			break;
		}

		case sizeof(int64_t) :
		{
			int64_t value;
			ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"%" WIDEN(PRId64), value);
			break;
		}

		default:
		{
			return false;
			break;
		}
		}

		break;
	}

	case btUInt:
	{
		ULONG64 length;
		if (!SymGetTypeInfo(stackTrace->process, pSymInfo->ModBase, typeIndex, TI_GET_LENGTH, &length))
		{
			return false;
		}

		switch (length)
		{
		case sizeof(uint8_t) :
		{
			uint8_t value;
			ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"%" WIDEN(PRIu8), value);
			break;
		}

		case sizeof(uint16_t) :
		{
			uint16_t value;
			ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"%" WIDEN(PRIu16), value);
			break;
		}

		case sizeof(uint32_t) :
		{
			uint32_t value;
			ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"%" WIDEN(PRIu32), value);
			break;
		}

		case sizeof(uint64_t) :
		{
			uint64_t value;
			ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"%" WIDEN(PRIu64), value);
			break;
		}

		default:
		{
			return false;
			break;
		}
		}

		break;
	}

	case btFloat:
	{
		float value;
		ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

		stackTrace->written +=
			swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
				L"%f", value);
		break;
	}

	case btLong:
	{
		long value;
		ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

		stackTrace->written +=
			swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
				L"%ld", value);
		break;
	}

	case btULong:
	{
		unsigned long value;
		ReadProcessMemory(stackTrace->process, valueLocation, &value, sizeof(value), NULL);

		stackTrace->written +=
			swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
				L"%lu", value);
		break;
	}

	default:
	{
		return false;
		break;
	}
	}

	return true;
}

bool printGivenType(StackTrace* stackTrace, PSYMBOL_INFOW pSymInfo, enum SymTagEnum symbolTag, ULONG typeIndex, void* valueLocation, bool whilePrintingPointer)
{
	switch (symbolTag)
	{
	case SymTagBaseType:
	{
		if (!printBasicType(stackTrace, pSymInfo, typeIndex, valueLocation, whilePrintingPointer))
		{
			return false;
		}

		break;
	}

	case SymTagPointerType:
	{
		void* pointedValueLocation;
		ReadProcessMemory(stackTrace->process, valueLocation, &pointedValueLocation, sizeof(pointedValueLocation), NULL);

		stackTrace->written +=
			swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
				L"0x%p -> ", pointedValueLocation);

		if (pointedValueLocation < (void*)0x1000)
		{
			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"?");
			break;
		}

		DWORD pointedTypeIndex;
		if (!SymGetTypeInfo(stackTrace->process, pSymInfo->ModBase, typeIndex, TI_GET_TYPE, &pointedTypeIndex))
		{
			return false;
		}

		ULONG64 pointedTypeLength;
		if (!SymGetTypeInfo(stackTrace->process, pSymInfo->ModBase, pointedTypeIndex, TI_GET_LENGTH, &pointedTypeLength))
		{
			return false;
		}

		MEMORY_BASIC_INFORMATION pageInfo = { 0 };
		if (VirtualQueryEx(stackTrace->process, pointedValueLocation, &pageInfo, sizeof(pageInfo)) == 0)
		{
			return false;
		}

		for (;;)
		{
			PVOID pageEndAddress = (char*)pageInfo.BaseAddress + pageInfo.RegionSize;

			if ((void*)((char*)pointedValueLocation + pointedTypeLength) >= pageEndAddress)
			{
				if (VirtualQueryEx(stackTrace->process, pageEndAddress, &pageInfo, sizeof(pageInfo)) == 0)
				{
					return false;
				}
			}
			else
			{
				break;
			}
		}

		enum SymTagEnum pointedSymbolTag;
		if (!SymGetTypeInfo(stackTrace->process, pSymInfo->ModBase, pointedTypeIndex, TI_GET_SYMTAG, &pointedSymbolTag))
		{
			return false;
		}

		return printGivenType(stackTrace, pSymInfo, pointedSymbolTag, pointedTypeIndex, pointedValueLocation, true);
	}

	default:
	{
		return false;
	}
	}

	return true;
}

bool printType(StackTrace* stackTrace, PSYMBOL_INFOW pSymInfo, void* valueLocation)
{
	enum SymTagEnum symbolTag;
	if (!SymGetTypeInfo(stackTrace->process, pSymInfo->ModBase, pSymInfo->TypeIndex, TI_GET_SYMTAG, &symbolTag))
	{
		return false;
	}

	return printGivenType(stackTrace, pSymInfo, symbolTag, pSymInfo->TypeIndex, valueLocation, false);
}

BOOL CALLBACK enumParams(
	_In_ PSYMBOL_INFOW pSymInfo,
	_In_ ULONG SymbolSize,
	_In_opt_ PVOID UserContext)
{
	if ((pSymInfo->Flags & SYMFLAG_PARAMETER) == 0)
	{
		return TRUE;
	}

	StackTrace* stackTrace = (StackTrace*)UserContext;

	if (stackTrace->isFirstParameter)
	{
		stackTrace->isFirstParameter = false;
	}
	else
	{
		stackTrace->written +=
			swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
				L", ");
	}

	stackTrace->written +=
		swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
			L"%ls = ", pSymInfo->Name);

	void* valueLocation = NULL;
	if (pSymInfo->Flags & SYMFLAG_REGISTER)
	{
		if (stackTrace->scratchSpace == NULL)
		{
			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"<Could not allocate memory to write register value>");
			return TRUE;
		}

		valueLocation = stackTrace->scratchSpace;

		switch (pSymInfo->Register)
		{
#ifndef _WIN64
		case 17:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Eax, sizeof(stackTrace->contextRecord->Eax), NULL);
			break;
		case 18:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Ecx, sizeof(stackTrace->contextRecord->Ecx), NULL);
			break;
		case 19:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Edx, sizeof(stackTrace->contextRecord->Edx), NULL);
			break;
		case 20:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Ebx, sizeof(stackTrace->contextRecord->Ebx), NULL);
			break;
#else
		case 328:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Rax, sizeof(stackTrace->contextRecord->Rax), NULL);
			break;

		case 329:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Rbx, sizeof(stackTrace->contextRecord->Rbx), NULL);
			break;

		case 330:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Rcx, sizeof(stackTrace->contextRecord->Rcx), NULL);
			break;

		case 331:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Rdx, sizeof(stackTrace->contextRecord->Rdx), NULL);
			break;

		case 336:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->R8, sizeof(stackTrace->contextRecord->R8), NULL);
			break;

		case 337:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->R9, sizeof(stackTrace->contextRecord->R9), NULL);
			break;

		case 154:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Xmm0, sizeof(stackTrace->contextRecord->Xmm0), NULL);
			break;

		case 155:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Xmm1, sizeof(stackTrace->contextRecord->Xmm1), NULL);
			break;

		case 156:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Xmm2, sizeof(stackTrace->contextRecord->Xmm2), NULL);
			break;

		case 157:
			WriteProcessMemory(stackTrace->process, valueLocation, &stackTrace->contextRecord->Xmm3, sizeof(stackTrace->contextRecord->Xmm3), NULL);
			break;
#endif

		default:
			stackTrace->written +=
				swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L"<Unknown register %lu>", pSymInfo->Register);
			return TRUE;
		}
	}
	else
	{
		valueLocation = (void*)(stackTrace->currentStackFrame.AddrFrame.Offset + pSymInfo->Address);
	}

	if (!printType(stackTrace, pSymInfo, valueLocation))
	{
		stackTrace->written +=
			swprintf_s(stackTrace->message + stackTrace->written, sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
				L"?");
	}

	return TRUE;
}

void printStack(StackTrace* stackTrace)
{
	if (!SymInitializeW(stackTrace->process, L"srv*http://arnaviont61/symbols", TRUE))
	{
		return;
	}

	SYMBOL_INFOW* symbol = calloc(sizeof(*symbol) + 256 * sizeof(wchar_t), 1);
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFOW);

#ifndef _WIN64
	DWORD machineType = IMAGE_FILE_MACHINE_I386;
#else
	DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#endif

	for (int i = 0; ; i++)
	{
		if (!StackWalk64(machineType, stackTrace->process, stackTrace->thread, &stackTrace->currentStackFrame, stackTrace->contextRecord, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
		{
			break;
		}

		if (SymFromAddrW(stackTrace->process, stackTrace->currentStackFrame.AddrPC.Offset, NULL, symbol))
		{
			stackTrace->written +=
				swprintf_s(&stackTrace->message[stackTrace->written], sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written - 1,
					L">%02i: 0x%08llX %ls (", i, symbol->Address, symbol->Name);

			IMAGEHLP_STACK_FRAME stackFrame = { 0 };
			stackFrame.InstructionOffset = symbol->Address;
			if (SymSetContext(stackTrace->process, &stackFrame, NULL))
			{
				stackTrace->isFirstParameter = true;
				SymEnumSymbolsW(stackTrace->process, 0, NULL, enumParams, stackTrace);
			}

			stackTrace->written +=
				swprintf_s(&stackTrace->message[stackTrace->written], sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L")\n");
		}
		else
		{
			stackTrace->written +=
				swprintf_s(&stackTrace->message[stackTrace->written], sizeof(stackTrace->message) / sizeof(stackTrace->message[0]) - stackTrace->written,
					L">%02i: 0x%08llX ?\n", i, stackTrace->currentStackFrame.AddrPC.Offset);
		}
	}

	free(symbol);

	SymCleanup(stackTrace->process);
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		return -1;
	}

	char* processIdString = argv[1];
	char* threadIdString = argv[2];
	char* exceptionPointersString = argv[3];

	DWORD processId;
	DWORD threadId;
	LPEXCEPTION_POINTERS exception;
	EXCEPTION_POINTERS remoteException = { 0 };
	CONTEXT remoteContextRecord = { 0 };

	sscanf_s(processIdString, "%lu", &processId);
	sscanf_s(threadIdString, "%lu", &threadId);
	sscanf_s(exceptionPointersString, "%p", &exception);

	StackTrace* stackTrace = calloc(sizeof(*stackTrace), 1);
	stackTrace->process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	stackTrace->thread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
	SuspendThread(stackTrace->thread);

	ReadProcessMemory(stackTrace->process, exception, &remoteException, sizeof(remoteException), NULL);
	ReadProcessMemory(stackTrace->process, remoteException.ContextRecord, &remoteContextRecord, sizeof(remoteContextRecord), NULL);

#ifndef _WIN64
	stackTrace->currentStackFrame.AddrPC.Offset = remoteContextRecord.Eip;
	stackTrace->currentStackFrame.AddrPC.Mode = AddrModeFlat;
	stackTrace->currentStackFrame.AddrFrame.Offset = remoteContextRecord.Ebp;
	stackTrace->currentStackFrame.AddrFrame.Mode = AddrModeFlat;
	stackTrace->currentStackFrame.AddrStack.Offset = remoteContextRecord.Esp;
	stackTrace->currentStackFrame.AddrStack.Mode = AddrModeFlat;
#else
	stackTrace->currentStackFrame.AddrPC.Offset = remoteContextRecord.Rip;
	stackTrace->currentStackFrame.AddrPC.Mode = AddrModeFlat;
	stackTrace->currentStackFrame.AddrFrame.Offset = remoteContextRecord.Rbp;
	stackTrace->currentStackFrame.AddrFrame.Mode = AddrModeFlat;
	stackTrace->currentStackFrame.AddrStack.Offset = remoteContextRecord.Rsp;
	stackTrace->currentStackFrame.AddrStack.Mode = AddrModeFlat;
#endif

	stackTrace->contextRecord = &remoteContextRecord;

	// The biggest registers are the XMM registers (128 bits), so reserve enough space for them.
	stackTrace->scratchSpace = VirtualAllocEx(stackTrace->process, NULL, 128 / 8, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	wchar_t tempPath[MAX_PATH + 1];
	GetTempPathW(sizeof(tempPath) / sizeof(tempPath[0]), tempPath);

	printStack(stackTrace);
	BOOL wroteStackTrace = stackTrace->written > 0;

	wchar_t stackTraceFilename[MAX_PATH + 1];
	if (wroteStackTrace)
	{
		GetTempFileNameW(tempPath, L"dmp", 0, stackTraceFilename);

		HANDLE stackTraceFile = CreateFileW(stackTraceFilename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD numBytesWritten;
		if (stackTraceFile != INVALID_HANDLE_VALUE)
		{
			wroteStackTrace = WriteFile(stackTraceFile, stackTrace->message, (stackTrace->written - 1) * sizeof(wchar_t), &numBytesWritten, NULL);
			CloseHandle(stackTraceFile);
		}
	}

	wchar_t message[1024] = { 0 };

	int written =
		wroteStackTrace ?
			swprintf_s(
				message, sizeof(message) / sizeof(message[0]),
				L"Stack trace written to %ls\n\n",
				stackTraceFilename) :
			swprintf_s(
				message, sizeof(message) / sizeof(message[0]),
				L"Writing the stack trace failed because of error 0x%08X\n\n",
				GetLastError());

	wchar_t dumpFilename[MAX_PATH + 1];
	bool wroteMiniDump = false;

	if (GetTempFileNameW(tempPath, L"dmp", 0, dumpFilename) != 0)
	{
		HANDLE dumpFile = CreateFileW(dumpFilename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (dumpFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInformation = { 0 };
			minidumpExceptionInformation.ThreadId = threadId;
			minidumpExceptionInformation.ExceptionPointers = exception;
			minidumpExceptionInformation.ClientPointers = TRUE;

			wroteMiniDump =
				MiniDumpWriteDump(
					stackTrace->process, processId,
					dumpFile,
					MiniDumpWithDataSegs |
					MiniDumpWithHandleData |
					MiniDumpScanMemory |
					MiniDumpWithUnloadedModules |
					MiniDumpWithIndirectlyReferencedMemory |
					MiniDumpWithPrivateReadWriteMemory |
					MiniDumpWithFullMemoryInfo |
					MiniDumpWithThreadInfo |
					MiniDumpIgnoreInaccessibleMemory,
					&minidumpExceptionInformation, NULL, NULL);

			CloseHandle(dumpFile);
		}
	}

	written +=
		wroteMiniDump ?
			swprintf_s(message + written, sizeof(message) / sizeof(message[0]) - written, L"Minidump written to %ls", dumpFilename) :
			swprintf_s(message + written, sizeof(message) / sizeof(message[0]) - written, L"Minidump could not be taken because of error 0x%08X", GetLastError());

	MessageBoxW(NULL, message, L"Crash", MB_OK);

	ResumeThread(stackTrace->thread);
	CloseHandle(stackTrace->thread);
	CloseHandle(stackTrace->process);

	return 0;
}
