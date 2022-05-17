#include <iostream>
#include <Windows.h>
#include <functional>
#include <thread>
#include <iomanip>//std::put_time
#include <sstream>
#include <tchar.h>
#include "ipc.h"
//#pragma comment(lib,"ipc.lib")

//dynamic call
namespace cschy {
#define BUFSIZ 32
    typedef struct _MyMsg {
        unsigned int fromOne;
        unsigned int type;
        union MsgCore {
            TCHAR strMsg[BUFSIZ];
            void* vMsg;
            int iMsg;
        }msgCore;
        std::time_t createTime;
    }MYMSG, * PMYMSG;

    typedef BOOL(*pfnSendMessage)(unsigned int, MYMSG);
    pfnSendMessage SendMsg = (pfnSendMessage)GetProcAddress(LoadLibraryA("ipc.dll"), "SendMsg");

    using pfnCallBack = std::function<void(PMYMSG)>;
    typedef BOOL(*pfnGetMessage)(unsigned int, pfnCallBack);
    pfnGetMessage RecvMsg = (pfnGetMessage)GetProcAddress(LoadLibraryA("ipc.dll"), "RecvMsg");
}

void PrintDebugString(LPCTSTR lpszFmt, ...)
{
    va_list args;
    va_start(args, lpszFmt);
    int len = _vsctprintf(lpszFmt, args) + 2;
    TCHAR* lpszBuf = (TCHAR*)_malloca(len * sizeof(TCHAR));//栈中分配, 不需要释放
    if (lpszBuf == NULL)
    {
        OutputDebugStringA("Failure: _malloca lpszBuf NULL");
        return;
    }
    _vstprintf_s(lpszBuf, len, lpszFmt, args);
    va_end(args);
    lpszBuf[len - 2] = _T('\n');
    lpszBuf[len - 1] = _T('\0');
    OutputDebugString(lpszBuf);
}

int main()
{
    DWORD pid;
    cschy::MYMSG msg{};
    setlocale(LC_ALL, "");

    //SendMessage
    std::thread([&]() {
        while (std::wcin >> pid >> msg.msgCore.strMsg)
        {
            while (true) {
                cschy::SendMsg(pid, msg);
            }
        }
    }).detach();


#define TEST_TIME 0

#if TEST_TIME
    ULONGLONG localtime_total = 0;
    ULONGLONG ss_total = 0;
    ULONGLONG wpf_total = 0;
    int count = 0;
#endif
    //GetMessage
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    cschy::RecvMsg(GetCurrentProcessId(), [&](cschy::PMYMSG msg) {
#if TEST_TIME
        count++;
        ULONGLONG start_localtime = GetTickCount64();
#endif
        tm t;
        localtime_s(&t, &msg->createTime);
#if TEST_TIME
        localtime_total += (GetTickCount64() - start_localtime);
#endif
#if TEST_TIME
        ULONGLONG start_ss = GetTickCount64();
#endif
        std::wstringstream ss;
        ss << std::put_time(/*std::localtime(&msg->createTime)*/&t, L"%Y-%m-%d %H:%M:%S");
#if TEST_TIME
        ss_total += (GetTickCount64() - start_ss);
#endif
#if TEST_TIME
        ULONGLONG start_wpf = GetTickCount64();
#endif
        //wprintf(L"收到msg:%s, time:%s\n", msg->msgCore.strMsg, ss.str().c_str());//同步io影响了消费者速度
        //PrintDebugString(_T("收到msg:%s, time:%s"), msg->msgCore.strMsg, ss.str().c_str());
        TCHAR buf[64];
        ZeroMemory(buf, sizeof(buf));
        _stprintf_s(buf, _T("收到msg:%s, time:%s\n"), msg->msgCore.strMsg, ss.str().c_str());
        WriteConsole(hOut, buf, lstrlen(buf), NULL, NULL);
        //WriteFile(hOut, buf, lstrlen(buf)*2, NULL, NULL);//乱码，认为底层调用的是WriteConsoleA()，因为和WriteConsoleA()输出一样
#if TEST_TIME
        wpf_total += (GetTickCount64() - start_wpf);
#endif
#if TEST_TIME
        if (count == 4096) {
            std::cout << "localtime_total:" << localtime_total << std::endl;//0
            std::cout << "ss_total:" << ss_total << std::endl;//32
            std::cout << "wpf_total:" << wpf_total << std::endl;//2808
        }
#endif
    });

    //std::thread([]() {
    //    while (cschy::RecvMsg(GetCurrentProcessId(), [](cschy::PMYMSG msg) {
    //        tm t;
    //        localtime_s(&t, &msg->createTime);
    //        std::wstringstream ss;
    //        ss << std::put_time(/*std::localtime(&msg->createTime)*/&t, L"%Y-%m-%d %H:%M:%S");
    //        wprintf(L"收到msg:%s, time:%s\n", msg->msgCore.strMsg, ss.str().c_str());
    //        }))
    //    {
    //
    //    }
    //    }).detach();

    
    return 0;
}