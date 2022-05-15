#include <iostream>
#include <Windows.h>
#include <functional>
#include <thread>
#include <iomanip>//std::put_time
#include <sstream>
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

int main()
{
    DWORD pid;
    cschy::MYMSG msg{};
    setlocale(0, "chs");

    //GetMessage
    std::thread([]() {
        while (cschy::RecvMsg(GetCurrentProcessId(), [](cschy::PMYMSG msg) {
            tm t;
            localtime_s(&t, &msg->createTime);
            std::wstringstream ss;
            ss << std::put_time(/*std::localtime(&msg->createTime)*/&t, L"%Y-%m-%d %H:%M:%S");
            wprintf(L"收到msg:%s, time:%s\n", msg->msgCore.strMsg, ss.str().c_str());
            }))
        {

        }
        }).detach();

        //SendMessage
        while (std::wcin >> pid >> msg.msgCore.strMsg)
        {
            while (true) {
                cschy::SendMsg(pid, msg);
            }
        }
        return 0;
}