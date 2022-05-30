#pragma once

#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <iostream>
#include <list>
#include <string>
#include <functional>
#include "ThreadPool.h"


//宏定义与常量
#ifdef UNICODE
#define tstring std::wstring
#define to_tstring std::to_wstring
#else
#define tstring std::string
#define to_tstring std::to_string
#endif // UNICODE

#define EXPORT extern "C" __declspec(dllexport)
#define BUFSIZ 32

const int max_proc = 16;
const int max_node = USN_PAGE_SIZE;


//类型定义
typedef struct _MyMsg {
	unsigned int fromOne;
	unsigned int type;
	/*union MsgCore {
		TCHAR strMsg[BUFSIZ];
		void* vMsg;
		int iMsg;
	}msgCore;*/
	union MsgCore {
		static const size_t size{ 64 };//外部可根据这个大小把msgCore地址强制类型转换再赋值
		TCHAR strMsg[size / sizeof(TCHAR)];
		char* cMsg[size / sizeof(char*)];
		void* vMsg[size / sizeof(void*)];
	}msgCore;
	std::time_t createTime;
}MYMSG, * PMYMSG;
//auto a = sizeof MYMSG;
typedef struct _ProcInfo {
	unsigned int flag;
	unsigned int uid;
	unsigned int sendmsgs;
	unsigned int recvmsgs;
	HANDLE event;
	TCHAR evname[8];
}PROCINFO, * PPROCINFO;
//auto a = sizeof PROCINFO;
typedef struct _MsgInfo {
	unsigned int index;
	unsigned int toOne;
	MYMSG msg;
}MSGINFO, * PMSGINFO;
//auto a = sizeof MSGINFO;

namespace cc {
	namespace abs {
		struct ListInfo {
			CRITICAL_SECTION cs;
			size_t node_nums;
		};
		struct IList {
			static ListInfo info;
			virtual ~IList() {}
			virtual void insert_node(IList* head, IList* node) = 0;
			virtual IList* delete_node(IList* head, IList* node = nullptr) = 0;
		};
		//single loop list
		struct List : public IList {
			List* next;
			virtual void insert_node(List* head, List* node) {
				EnterCriticalSection(&info.cs);
				if (head && node) {
					node->next = head->next;
					head->next = node;
					info.node_nums++;
				}
				LeaveCriticalSection(&info.cs);
			}
			virtual List* delete_node(List* head, List* node) {
				EnterCriticalSection(&info.cs);
				List* retNode = nullptr;
				if (!node) {
					node = head->next;
				}
				List* preNode = head;
				for (retNode = preNode->next; retNode != head; retNode = retNode->next, preNode = preNode->next) {
					if (retNode == node) {
						preNode->next = retNode->next;
						//retNode->next = nullptr;
						info.node_nums--;
						break;
					}
				}
				LeaveCriticalSection(&info.cs);
				return retNode;
			}
		};
		//double loop list
		struct BiList : public IList {
			BiList* prev;
			BiList* next;
			virtual void insert_node(BiList* head, BiList* node) {
				EnterCriticalSection(&info.cs);
				if (head && node) {
					BiList* prev = head->prev;
					node->prev = prev;
					node->next = head;
					head->prev = node;
					prev->next = node;//因为是next遍历，所以先操作prev指针，后操作next指针
					info.node_nums++;
				}
				LeaveCriticalSection(&info.cs);
			}
			virtual BiList* delete_node(BiList* head, BiList* node) {
				EnterCriticalSection(&info.cs);
				BiList* retNode = node ? node : head->next;
				if (retNode != head) {
					BiList* next = retNode->next;
					BiList* prev = retNode->prev;
					next->prev = prev;
					prev->next = retNode->next;
					//retNode->prev = nullptr;
					//retNode->next = nullptr;
					//如果取消上一行注释，那么导致RecvMsg()中p->next=null而退出循环，和break一样了
					info.node_nums--;
				}
				LeaveCriticalSection(&info.cs);
				return retNode;
			}
		};
	}

	struct listinfo {
		CRITICAL_SECTION cs;
		size_t node_nums;
	};

	struct list {
		list* next;
		static listinfo info;
		static void insert_node(list* head, list* node) {
			EnterCriticalSection(&info.cs);
			if (head && node) {
				node->next = head->next;
				head->next = node;
				info.node_nums++;
			}
			LeaveCriticalSection(&info.cs);
		}
		static list* pop_node(list* head) {
			EnterCriticalSection(&info.cs);
			list* node = head->next;
			if (head != node) {
				head->next = node->next;
				node->next = nullptr;
				info.node_nums--;
			}
			LeaveCriticalSection(&info.cs);
			return node;
		}
	};

	struct datalist {
		void* data;
		list ls;
	};


	struct bilist {
		bilist* prev;
		bilist* next;
		static listinfo info;
		static void insert_node(bilist* head, bilist* node) {
			EnterCriticalSection(&info.cs);
			if (head && node) {
				bilist* prev = head->prev;
				node->prev = prev;
				node->next = head;
				head->prev = node;
				prev->next = node;//因为是next遍历，所以先操作prev指针，后操作next指针
				info.node_nums++;
			}
			LeaveCriticalSection(&info.cs);
		}
		static void delete_node(bilist* node) {
			EnterCriticalSection(&info.cs);
			bilist* next = node->next;
			bilist* prev = node->prev;
			next->prev = prev;
			prev->next = node->next;
			//node->prev = nullptr;
			//node->next = nullptr;
			//如果取消上一行注释，那么导致RecvMsg()中p->next=null而退出循环，和break一样了
			info.node_nums--;
			LeaveCriticalSection(&info.cs);
		}
	};

	struct databilist {
		void* data;
		bilist bils;
	};
}


//全局变量
DWORD cur_pid = GetCurrentProcessId();


//共享段
#pragma data_seg (".shared")

MSGINFO msgbuf[max_node] = { 0 };
cc::datalist lsmem[max_node] = { 0 };
cc::databilist bilsmem[max_node] = { 0 };

PROCINFO procInfo[max_proc] = { 0 };

//cc::listinfo meminfo = { 0 };
//cc::listinfo msqinfo = { 0 };
cc::datalist lshead = { 0 };
cc::databilist bilshead = { 0 };

cc::listinfo cc::list::info = { 0 };//meminfo
cc::listinfo cc::bilist::info = { 0 };//msqinfo

int currentProcNum = 0;
bool init_data_struct = false;

#pragma data_seg ()
#pragma comment (linker, "/section:.shared,rws")

void init_list() {
	InitializeCriticalSection(&cc::list::info.cs);
	//InitializeCriticalSection(&meminfo.cs);
	//head.data = &meminfo;
	lshead.ls.next = &lshead.ls;//单向循环链表，头节点自指
	
	for (int i = 0; i < ARRAYSIZE(lsmem); i++) {
		msgbuf[i].index = i;
		lsmem[i].data = &msgbuf[i];
		//cc::list::insert_node(&lshead.ls, &lsmem[i].ls, &meminfo);
		cc::list::insert_node(&lshead.ls, &lsmem[i].ls);
	}
}

void init_bilist() {
	InitializeCriticalSection(&cc::bilist::info.cs);
	//InitializeCriticalSection(&msqinfo.cs);
	//bihead.data = &msqinfo;
	//双向循环链表，头节点自指
	bilshead.bils.next = &bilshead.bils;
	bilshead.bils.prev = &bilshead.bils;
}

struct _FindProc {
	PPROCINFO operator[](unsigned int i) {
		for (auto& pi : procInfo)
		{
			if (pi.uid == i)
				return &pi;
		}
		return nullptr;
	}
}FINDPROC;

EXPORT BOOL SendMsg(unsigned int toOne, MYMSG msg)
{
	PPROCINFO proc = FINDPROC[toOne];
	PPROCINFO self = FINDPROC[cur_pid];
	if (!proc || !self)
	{
		return FALSE;
	}

	std::cout << "当前消息队列大小：" << /*msqinfo.node_nums*/ cc::bilist::info.node_nums << std::endl;
	//if (meminfo.node_nums == 0) {//msq删除节点先于内存回收，如果依据msq==max_node来返回，则可能出现下面的pop_node是头节点(不带数据)
	//	return FALSE;//等待消费
	//}
	//分配一个内存块并取得消息结构
	//cc::list* lsnode = cc::list::pop_node(&lshead.ls, &meminfo);
	cc::list* lsnode = cc::list::pop_node(&lshead.ls);
	if (lsnode == &lshead.ls) {
		return FALSE;//等待消费
	}
	cc::datalist* block = CONTAINING_RECORD(lsnode, cc::datalist, ls);
	PMSGINFO mi = PMSGINFO(block->data);
	//填充消息结构
	mi->toOne = toOne;
	memcpy(&mi->msg.msgCore, &msg.msgCore, sizeof(msg.msgCore));
	mi->msg.fromOne = cur_pid;//specialized
	mi->msg.createTime = std::time(nullptr);
	//挂载消息队列
	bilsmem[mi->index].data = mi;
	//cc::bilist::insert_node(&bilshead.bils, &bilsmem[mi->index].bils, &msqinfo);
	cc::bilist::insert_node(&bilshead.bils, &bilsmem[mi->index].bils);

	if (SetEvent(CreateEvent(NULL, FALSE, FALSE, proc->evname)))
	{
		OutputDebugStringA("Success: SetEvent\n");
		self->sendmsgs++;
		std::cout << "send:" << self->sendmsgs << std::endl;
		//如果要实现同步读写，则在这里等待消费句柄
		return TRUE;
	}
	else
	{
		OutputDebugStringA((std::to_string(GetLastError()) + "Failure: SetEvent\n").c_str());
	}
	return FALSE;
}

#define TEST_TIME 0
#define TEST_THREADS 0
EXPORT BOOL RecvMsg(unsigned int self, std::function<void(PMYMSG)> callback)
{
	PPROCINFO proc = FINDPROC[self];
	if (!proc)
	{
		return FALSE;
	}
	WaitForSingleObject(proc->event, INFINITE);
#if TEST_THREADS
	ThreadPool pool;
#endif
#if TEST_TIME
	ULONGLONG func_total = 0;
	ULONGLONG start = GetTickCount64();
#endif
	cc::bilist* p = &bilshead.bils;
	while (p)
	{
		if (p != &bilshead.bils)//跳过头节点，因为头节点没有存消息
		{
			cc::databilist* node = CONTAINING_RECORD(p, cc::databilist, bils);
			PMSGINFO mi = PMSGINFO(node->data);
			if (mi->toOne == self)
			{
#if TEST_TIME
				ULONGLONG func_start = GetTickCount64();
#endif
#if TEST_THREADS
				pool.add([&]() {//加入线程池后总时间不会减少，是printf的IO瓶颈
					callback(&mi->msg);
				});
#else
				callback(&mi->msg);//导致消费者慢的主要原因，下面的话当我没说
#endif
#if TEST_TIME
				func_total += (GetTickCount64() - func_start);
#endif
				//cc::bilist::delete_node(p, &msqinfo);
				cc::bilist::delete_node(p);
				//cc::list::insert_node(&lshead.ls, &lsmem[mi->index].ls, &meminfo);
				cc::list::insert_node(&lshead.ls, &lsmem[mi->index].ls);
				proc->recvmsgs++;
				std::cout << "recv:" << proc->recvmsgs << " msqsize:" << /*msqinfo.node_nums*/ cc::bilist::info.node_nums << std::endl;
#if TEST_TIME
				if (proc->recvmsgs == 4096) {
					std::cout << "func-time:" << func_total << std::endl;//2840-31
					std::cout << "total-time:" << GetTickCount64() - start << std::endl;//5516-5531
				}
#endif
				//break;
				// break代表一次处理一个消息，消息的轮询收受外层循环(调用者)的影响
				// break之后就返回，如果消费速度跟不上生产速度，事实上主要由于生产者设置信号到消费者收到信号也需要一定的时间
				// SetEvent很快，当WaitForSingleObject等到信号时，已经错过了中间的很多消息
				// 这时的recvmsgs代表的是处理了的消息，sendmsgs与recvmsgs的差异反映了生产速度与消费速度的差异
				// 如果注释break，则当生产者停止生产时消费者会去处理剩余的消息，这时候recvmsgs就是总共收到的消息
				// 内核事件对象由内核管理，我使用它当然要经过内核上下文切换，如果想提高消费者速度，可以使用自旋锁
				// 但是这样会增加cpu消耗，而使用WaitForSingleObject则不会，但是如果一直有消息那么上下文切换会占大量开销
				// 所以我觉得如果是消息密集型则最好使用自旋锁，消息稀疏型则使用内核事件对象比较好
			}
		}
		p = p->next;
	}
	return TRUE;
}