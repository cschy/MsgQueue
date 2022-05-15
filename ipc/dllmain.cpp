// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "dllmain.h"

bool init(DWORD pid)
{
	std::cout << "pid:" << pid << std::endl;
	PPROCINFO pinfo = NULL;
	for (auto& i : procInfo)
	{
		if (!i.flag)
		{
			i.flag = 1;
			pinfo = &i;
			break;
		}
	}
	if (!pinfo) {
		return false;
	}

	pinfo->uid = pid;
	_stprintf_s(pinfo->evname, to_tstring(pinfo->uid).c_str());
	pinfo->event = CreateEvent(NULL, FALSE, FALSE, pinfo->evname);
	if (pinfo->event)
	{
		OutputDebugStringA("Success: CreateEvent\n");
	}
	else
	{
		OutputDebugStringA((std::to_string(GetLastError()) + "Failure: CreateEvent\n").c_str());
	}

	currentProcNum++;
	return true;
}

void initStructure()
{
	if (!init_data_struct) {
		init_list();
		init_bilist();
		init_data_struct = true;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		initStructure();
		if (!init(cur_pid)) {
			return FALSE;
		}
	}
	break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		currentProcNum--;
		PPROCINFO proc = FINDPROC[cur_pid];
		proc->flag = 0;
		break;
	}
	return TRUE;
}
