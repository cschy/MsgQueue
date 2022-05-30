#pragma once

#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <iostream>
#include <list>
#include <string>
#include <functional>
#include "ThreadPool.h"


//�궨���볣��
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


//���Ͷ���
typedef struct _MyMsg {
	unsigned int fromOne;
	unsigned int type;
	/*union MsgCore {
		TCHAR strMsg[BUFSIZ];
		void* vMsg;
		int iMsg;
	}msgCore;*/
	union MsgCore {
		static const size_t size{ 64 };//�ⲿ�ɸ��������С��msgCore��ַǿ������ת���ٸ�ֵ
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
					prev->next = node;//��Ϊ��next�����������Ȳ���prevָ�룬�����nextָ��
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
					//���ȡ����һ��ע�ͣ���ô����RecvMsg()��p->next=null���˳�ѭ������breakһ����
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
				prev->next = node;//��Ϊ��next�����������Ȳ���prevָ�룬�����nextָ��
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
			//���ȡ����һ��ע�ͣ���ô����RecvMsg()��p->next=null���˳�ѭ������breakһ����
			info.node_nums--;
			LeaveCriticalSection(&info.cs);
		}
	};

	struct databilist {
		void* data;
		bilist bils;
	};
}


//ȫ�ֱ���
DWORD cur_pid = GetCurrentProcessId();


//�����
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
	lshead.ls.next = &lshead.ls;//����ѭ������ͷ�ڵ���ָ
	
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
	//˫��ѭ������ͷ�ڵ���ָ
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

	std::cout << "��ǰ��Ϣ���д�С��" << /*msqinfo.node_nums*/ cc::bilist::info.node_nums << std::endl;
	//if (meminfo.node_nums == 0) {//msqɾ���ڵ������ڴ���գ��������msq==max_node�����أ�����ܳ��������pop_node��ͷ�ڵ�(��������)
	//	return FALSE;//�ȴ�����
	//}
	//����һ���ڴ�鲢ȡ����Ϣ�ṹ
	//cc::list* lsnode = cc::list::pop_node(&lshead.ls, &meminfo);
	cc::list* lsnode = cc::list::pop_node(&lshead.ls);
	if (lsnode == &lshead.ls) {
		return FALSE;//�ȴ�����
	}
	cc::datalist* block = CONTAINING_RECORD(lsnode, cc::datalist, ls);
	PMSGINFO mi = PMSGINFO(block->data);
	//�����Ϣ�ṹ
	mi->toOne = toOne;
	memcpy(&mi->msg.msgCore, &msg.msgCore, sizeof(msg.msgCore));
	mi->msg.fromOne = cur_pid;//specialized
	mi->msg.createTime = std::time(nullptr);
	//������Ϣ����
	bilsmem[mi->index].data = mi;
	//cc::bilist::insert_node(&bilshead.bils, &bilsmem[mi->index].bils, &msqinfo);
	cc::bilist::insert_node(&bilshead.bils, &bilsmem[mi->index].bils);

	if (SetEvent(CreateEvent(NULL, FALSE, FALSE, proc->evname)))
	{
		OutputDebugStringA("Success: SetEvent\n");
		self->sendmsgs++;
		std::cout << "send:" << self->sendmsgs << std::endl;
		//���Ҫʵ��ͬ����д����������ȴ����Ѿ��
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
		if (p != &bilshead.bils)//����ͷ�ڵ㣬��Ϊͷ�ڵ�û�д���Ϣ
		{
			cc::databilist* node = CONTAINING_RECORD(p, cc::databilist, bils);
			PMSGINFO mi = PMSGINFO(node->data);
			if (mi->toOne == self)
			{
#if TEST_TIME
				ULONGLONG func_start = GetTickCount64();
#endif
#if TEST_THREADS
				pool.add([&]() {//�����̳߳غ���ʱ�䲻����٣���printf��IOƿ��
					callback(&mi->msg);
				});
#else
				callback(&mi->msg);//����������������Ҫԭ������Ļ�����û˵
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
				// break����һ�δ���һ����Ϣ����Ϣ����ѯ�������ѭ��(������)��Ӱ��
				// break֮��ͷ��أ���������ٶȸ����������ٶȣ���ʵ����Ҫ���������������źŵ��������յ��ź�Ҳ��Ҫһ����ʱ��
				// SetEvent�ܿ죬��WaitForSingleObject�ȵ��ź�ʱ���Ѿ�������м�ĺܶ���Ϣ
				// ��ʱ��recvmsgs������Ǵ����˵���Ϣ��sendmsgs��recvmsgs�Ĳ��췴ӳ�������ٶ��������ٶȵĲ���
				// ���ע��break����������ֹͣ����ʱ�����߻�ȥ����ʣ�����Ϣ����ʱ��recvmsgs�����ܹ��յ�����Ϣ
				// �ں��¼��������ں˹�����ʹ������ȻҪ�����ں��������л������������������ٶȣ�����ʹ��������
				// ��������������cpu���ģ���ʹ��WaitForSingleObject�򲻻ᣬ�������һֱ����Ϣ��ô�������л���ռ��������
				// �����Ҿ����������Ϣ�ܼ��������ʹ������������Ϣϡ������ʹ���ں��¼�����ȽϺ�
			}
		}
		p = p->next;
	}
	return TRUE;
}