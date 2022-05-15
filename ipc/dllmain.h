#pragma once

#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <iostream>
#include <list>
#include <string>
#include <functional>


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
				prev->next = node;//��Ϊ��next�����������Ȳ���prevָ�룬�����nextָ��
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
			//���ȡ����һ��ע�ͣ���ô����RecvMsg()��p->next=null���˳�ѭ������breakһ����
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


//ȫ�ֱ���
DWORD cur_pid = GetCurrentProcessId();


//������
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
	lshead.ls.next = &lshead.ls;//����ѭ��������ͷ�ڵ���ָ

	for (int i = 0; i < ARRAYSIZE(lsmem); i++) {
		msgbuf[i].index = i;
		lsmem[i].data = &msgbuf[i];
		cc::list::insert_node(&lshead.ls, &lsmem[i].ls, &meminfo);
	}
}

void init_bilist() {
	InitializeCriticalSection(&msqinfo.cs);
	//bihead.data = &msqinfo;
	//˫��ѭ��������ͷ�ڵ���ָ
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

	std::cout << "��ǰ��Ϣ���д�С��" << msqinfo.node_nums << std::endl;
	//if (meminfo.node_nums == 0) {//msqɾ���ڵ������ڴ���գ��������msq==max_node�����أ�����ܳ��������pop_node��ͷ�ڵ�(��������)
	//	return FALSE;//�ȴ�����
	//}
	//����һ���ڴ�鲢ȡ����Ϣ�ṹ
	cc::list* lsnode = cc::list::pop_node(&lshead.ls, &meminfo);
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
	cc::bilist::insert_node(&bilshead.bils, &bilsmem[mi->index].bils, &msqinfo);

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

EXPORT BOOL RecvMsg(unsigned int self, std::function<void(PMYMSG)> callback)
{
	PPROCINFO proc = FINDPROC[self];
	if (!proc)
	{
		return FALSE;
	}
	WaitForSingleObject(proc->event, INFINITE);

	cc::bilist* p = &bilshead.bils;
	while (p)
	{
		if (p != &bilshead.bils)//����ͷ�ڵ㣬��Ϊͷ�ڵ�û�д���Ϣ
		{
			cc::databilist* node = CONTAINING_RECORD(p, cc::databilist, bils);
			PMSGINFO mi = PMSGINFO(node->data);
			if (mi->toOne == self)
			{
				callback(&mi->msg);
				cc::bilist::delete_node(p, &msqinfo);
				cc::list::insert_node(&lshead.ls, &lsmem[mi->index].ls, &meminfo);
				proc->recvmsgs++;
				std::cout << "recv:" << proc->recvmsgs << " msqsize:" << msqinfo.node_nums << std::endl;
				//break;
				// break����һ�δ���һ����Ϣ����Ϣ����ѯ�������ѭ��(������)��Ӱ��
				// break֮��ͷ��أ���������ٶȸ����������ٶȣ���ʵ����Ҫ���������������źŵ��������յ��ź�Ҳ��Ҫһ����ʱ��
				// SetEvent�ܿ죬��WaitForSingleObject�ȵ��ź�ʱ���Ѿ��������м�ĺܶ���Ϣ
				// ��ʱ��recvmsgs�������Ǵ����˵���Ϣ��sendmsgs��recvmsgs�Ĳ��췴ӳ�������ٶ��������ٶȵĲ���
				// ���ע��break����������ֹͣ����ʱ�����߻�ȥ����ʣ�����Ϣ����ʱ��recvmsgs�����ܹ��յ�����Ϣ
				// �ں��¼��������ں˹�������ʹ������ȻҪ�����ں��������л������������������ٶȣ�����ʹ��������
				// ��������������cpu���ģ���ʹ��WaitForSingleObject�򲻻ᣬ�������һֱ����Ϣ��ô�������л���ռ��������
				// �����Ҿ����������Ϣ�ܼ��������ʹ������������Ϣϡ������ʹ���ں��¼�����ȽϺ�
			}
		}
		p = p->next;
	}
	return TRUE;
}