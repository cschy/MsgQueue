#pragma once

#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <iostream>
#include <list>
#include <string>
#include <functional>


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
	union MsgCore {
		TCHAR strMsg[BUFSIZ];
		void* vMsg;
		int iMsg;
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
	struct listinfo {
		CRITICAL_SECTION cs;
		size_t node_nums;
	};

	namespace abs {
		struct ilist {
			virtual ~ilist() {}
			virtual void insert_node(ilist* head, ilist* node, listinfo* lsinfo) = 0;
			virtual void delete_node(ilist* node, listinfo* lsinfo) = 0;
			virtual ilist* pop_node(ilist* head, listinfo* lsinfo) = 0;
		};
		struct listhead : public ilist {
			listinfo* lsinfo;
			ilist* ils;
		};
		//single loop list
		struct sllist : public ilist {
			sllist* next;
			virtual void insert_node(sllist* head, sllist* node, listinfo* lsinfo) {
				EnterCriticalSection(&lsinfo->cs);
				if (head && node) {
					node->next = head->next;
					head->next = node;
				}
				LeaveCriticalSection(&lsinfo->cs);
			}
			virtual void delete_node(sllist* node, listinfo* lsinfo) {

			}
			virtual sllist* pop_node(sllist* head, listinfo* lsinfo) {
				EnterCriticalSection(&lsinfo->cs);
				sllist* node = head->next;
				if (head != node) {
					head->next = node->next;
					node->next = nullptr;
				}
				LeaveCriticalSection(&lsinfo->cs);
				return node;
			}
		};
		//double loop list
		struct bllist : public ilist {
			bllist* prev;
			bllist* next;
			virtual void insert_node(bllist* head, bllist* node, listinfo* lsinfo) {
				EnterCriticalSection(&lsinfo->cs);
				if (head && node) {
					bllist* prev = head->prev;
					node->prev = prev;
					node->next = head;
					prev->next = node;
					head->prev = node;
				}
				LeaveCriticalSection(&lsinfo->cs);
			}
			virtual void delete_node(bllist* node, listinfo* lsinfo) {
				EnterCriticalSection(&lsinfo->cs);
				bllist* next = node->next;
				bllist* prev = node->prev;
				prev->next = node->next;
				next->prev = prev;
				node->prev = nullptr;
				node->next = nullptr;
				LeaveCriticalSection(&lsinfo->cs);
			}
			virtual bllist* pop_node(bllist* head, listinfo* lsinfo) {
				return nullptr;
			}
		};
	}

	struct list {
		list* next;
		static void insert_node(list* head, list* node, listinfo* lsinfo) {
			EnterCriticalSection(&lsinfo->cs);
			if (head && node) {
				node->next = head->next;
				head->next = node;
				lsinfo->node_nums++;
			}
			LeaveCriticalSection(&lsinfo->cs);
		}
		static list* pop_node(list* head, listinfo* lsinfo) {
			EnterCriticalSection(&lsinfo->cs);
			list* node = head->next;
			if (head != node) {
				head->next = node->next;
				node->next = nullptr;
				lsinfo->node_nums--;
			}
			LeaveCriticalSection(&lsinfo->cs);
			return node;
		}
	};
	//void list_insert(list* head, list* node, CRITICAL_SECTION* cs = NULL) {
	//	if (cs) {
	//		EnterCriticalSection(cs);
	//	}
	//	if (head && node) {
	//		node->next = head->next;
	//		head->next = node;
	//	}
	//	if (cs) {
	//		LeaveCriticalSection(cs);
	//	}
	//}
	//list* list_delete(list* head, CRITICAL_SECTION* cs = NULL) {
	//	if (cs) {
	//		EnterCriticalSection(cs);
	//	}
	//	list* node = head->next;
	//	if (head && node && (head != node)) {
	//		head->next = node->next;
	//		//node->next = nullptr;
	//	}
	//	if (cs) {
	//		LeaveCriticalSection(cs);
	//	}
	//	return node;
	//}

	struct datalist {
		void* data;
		list ls;
	};


	struct bilist {
		bilist* prev;
		bilist* next;
		static void insert_node(bilist* head, bilist* node, listinfo* lsinfo) {
			EnterCriticalSection(&lsinfo->cs);
			if (head && node) {
				bilist* prev = head->prev;
				node->prev = prev;
				node->next = head;
				head->prev = node;
				prev->next = node;//因为是next遍历，所以先操作prev指针，后操作next指针
				lsinfo->node_nums++;
			}
			LeaveCriticalSection(&lsinfo->cs);
		}
		static void delete_node(bilist* node, listinfo* lsinfo) {
			EnterCriticalSection(&lsinfo->cs);
			bilist* next = node->next;
			bilist* prev = node->prev;
			next->prev = prev;
			prev->next = node->next;
			//node->prev = nullptr;
			//node->next = nullptr;
			//如果取消上一行注释，那么导致RecvMsg()中p->next=null而退出循环，和break一样了
			lsinfo->node_nums--;
			LeaveCriticalSection(&lsinfo->cs);
		}
	};
	//void bilist_insert(bilist* head, bilist* node, CRITICAL_SECTION* cs = NULL) {
	//	if (cs) {
	//		EnterCriticalSection(cs);
	//	}
	//	if (head && node) {
	//		bilist* prev = head->prev;
	//		node->prev = prev;
	//		node->next = head;
	//		if (prev) {
	//			prev->next = node;
	//		}
	//		head->prev = node;
	//	}
	//	if (cs) {
	//		LeaveCriticalSection(cs);
	//	}
	//}
	//void bilist_delete(bilist* node, CRITICAL_SECTION* cs = NULL) {
	//	if (cs) {
	//		EnterCriticalSection(cs);
	//	}
	//	bilist* next = node->next;
	//	bilist* prev = node->prev;
	//	prev->next = node->next;
	//	next->prev = prev;
	//	//node->prev = nullptr;
	//	//node->next = nullptr;
	//	if (cs) {
	//		LeaveCriticalSection(cs);
	//	}
	//}

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

cc::listinfo meminfo = { 0 };
cc::listinfo msqinfo = { 0 };
cc::datalist lshead = { 0 };
cc::databilist bilshead = { 0 };

int currentProcNum = 0;
bool init_data_struct = false;

#pragma data_seg ()
#pragma comment (linker, "/section:.shared,rws")

void init_list() {
	InitializeCriticalSection(&meminfo.cs);
	//head.data = &meminfo;
	lshead.ls.next = &lshead.ls;//单向循环链表，头节点自指

	for (int i = 0; i < ARRAYSIZE(lsmem); i++) {
		msgbuf[i].index = i;
		lsmem[i].data = &msgbuf[i];
		cc::list::insert_node(&lshead.ls, &lsmem[i].ls, &meminfo);
	}
}

void init_bilist() {
	InitializeCriticalSection(&msqinfo.cs);
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

	std::cout << "当前消息队列大小：" << msqinfo.node_nums << std::endl;
	//if (meminfo.node_nums == 0) {//msq删除节点先于内存回收，如果依据msq==max_node来返回，则可能出现下面的pop_node是头节点(不带数据)
	//	return FALSE;//等待消费
	//}
	//分配一个内存块并取得消息结构
	cc::list* lsnode = cc::list::pop_node(&lshead.ls, &meminfo);
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
	cc::bilist::insert_node(&bilshead.bils, &bilsmem[mi->index].bils, &msqinfo);

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

EXPORT BOOL RecvMsg(unsigned int self, std::function<void(PMYMSG)> callback)
{
	PPROCINFO proc = FINDPROC[self];
	if (!proc)
	{
		return FALSE;
	}
	WaitForSingleObject(proc->event, INFINITE);

	ULONGLONG func_total = 0;
	ULONGLONG start = GetTickCount64();
	cc::bilist* p = &bilshead.bils;
	while (p)
	{
		if (p != &bilshead.bils)//跳过头节点，因为头节点没有存消息
		{
			cc::databilist* node = CONTAINING_RECORD(p, cc::databilist, bils);
			PMSGINFO mi = PMSGINFO(node->data);
			if (mi->toOne == self)
			{
				ULONGLONG func_start = GetTickCount64();
				callback(&mi->msg);//导致消费者慢的主要原因，下面的话当我没说
				func_total += (GetTickCount64() - func_start);

				cc::bilist::delete_node(p, &msqinfo);
				cc::list::insert_node(&lshead.ls, &lsmem[mi->index].ls, &meminfo);
				proc->recvmsgs++;
				std::cout << "recv:" << proc->recvmsgs << " msqsize:" << msqinfo.node_nums << std::endl;
				if (msqinfo.node_nums == 0) {
					std::cout << "func-time:" << func_total << std::endl;
					std::cout << "total-time:" << GetTickCount64() - start << std::endl;
					//采样结果(ms)：1440-3105、3669-7719、5656-12843
				}
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