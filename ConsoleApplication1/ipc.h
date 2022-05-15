#pragma once

#include <Windows.h>
#include <iostream>
#include <functional>

//namespace cschy {
//#define BUFSIZ 32
//    typedef struct _MyMsg {
//        unsigned int fromOne;
//        unsigned int type;
//        union MsgCore {
//            TCHAR strMsg[BUFSIZ];
//            void* vMsg;
//            int iMsg;
//        }msgCore;
//        std::time_t createTime;
//    }MYMSG, * PMYMSG;
//
//#define IMPORT extern "C" __declspec(dllimport)
//
//	IMPORT BOOL SendMsg(unsigned int, MYMSG);
//
//	IMPORT BOOL RecvMsg(unsigned int, std::function<void(PMYMSG)>);
//}