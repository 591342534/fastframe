/*
 * frame_mem_mgt.h
 *
 *  Created on: 2012-12-18
 *      Author: jimm
 */

#ifndef FRAME_MEM_MGT_H_
#define FRAME_MEM_MGT_H_

#include "common/common_object.h"
#include "common/common_singleton.h"
#include "common/common_mutex.h"
#include "frame_namespace.h"
#include "frame_logengine.h"

#include <map>
#include <list>
#include <string>
#include <set>

using namespace std;

FRAME_NAMESPACE_BEGIN

//ÿ�������ڴ�ĸ���
#define ALLOC_MEM_STEP				1000
//�ڴ�����������
#define MAM_BLOCK_SIZE_STEP			64

class MemBlockHead
{
public:
	MemBlockHead()
	{
		m_nIndex = -1;
		m_nBlockSize = 0;
		m_nReferCount = 0;
	}

	int32_t			m_nIndex;				//���ڴ���е�����λ��
	uint32_t		m_nBlockSize;			//�ڴ��ʵ�ʴ�С
	uint16_t		m_nReferCount;			//���ڶ��ٵط�����
};

class MemBlockInfo
{
public:
	MemBlockInfo()
	{
		m_nBlockSize = 0;
		m_nBlockCount = 0;
	}

	CriticalSection	m_stMemBlockLock;
	uint32_t		m_nBlockSize;			//���ڴ���С
	int32_t			m_nBlockCount;			//�ڴ��ĸ���
	list<uint8_t *>	m_stMemBlockList;		//�ڴ��ָ������
};

typedef map<int32_t, MemBlockInfo *> MemInfoTable;

class MemOperationRecord
{
public:
	MemOperationRecord()
	{
		m_nLineNo = 0;
		m_nBlockSize = 0;
		m_nOperationCount = 0;
	}
	int32_t			m_nLineNo;
	uint32_t		m_nBlockSize;
	uint32_t		m_nOperationCount;
};

typedef map<uint32_t, MemOperationRecord *>			BlockSizeMap;
typedef map<int32_t, BlockSizeMap *>				LineNoMap;
typedef map<string, LineNoMap *>  					MemRecordMap;

typedef set<uint8_t *>		MemAddressRecord;

class CFrameMemMgt : public CObject
{
public:

	CFrameMemMgt();
	virtual ~CFrameMemMgt();

	//ע�⣬��ʼ���ڴ�һ��Ҫ��֤���̲߳�����
	int32_t Initialize();
	int32_t Resume();
	int32_t Uinitialize();

	//�����ڴ��
	uint8_t *AllocBlock(int32_t nWantSize);
	//�����ڴ��
	void RecycleBlock(uint8_t *pMemBlock);
	//��ӿ��ڴ�����ʹ�ü�¼
	uint8_t* AddBlkAddrRcd(uint8_t *pBlock);
	//ɾ�����ڴ�����ʹ�ü�¼
	void DelBlkAddrRcd(uint8_t *pBlock);
	//�Ƿ���ڴ�������ʹ�ü�¼
	bool HasBlkAddrRcd(uint8_t *pBlock);
	//��Ӷ��ڴ�����ʹ�ü�¼
	uint8_t* AddHeapAddrRcd(uint8_t *pBlock);
	//ɾ�����ڴ�����ʹ�ü�¼
	void DelHeapAddrRcd(uint8_t *pBlock);
	//�Ƿ��ж��ڴ�����ʹ�ü�¼
	bool HasHeapAddrRcd(uint8_t *pBlock);
	//��¼�ڴ�й¶��Ϣ
	void RecordMemLeakInfo(uint8_t *pMemBlock);
	//ͳ��Ŀǰ�����ڴ�������
	void PrintMemBlockInfo();

	int32_t GetBlockSize(int32_t nWantSize);
	//ͳ��������Ϣ
	void RecordAllocInfo(const char*pFileName, int32_t nLineNo, uint32_t nBlockSize);
	//ͳ���ͷ���Ϣ
	void RecordRecycleInfo(const char*pFileName, int32_t nLineNo, uint32_t nBlockSize);
	//��ȡ����ڴ���С
	uint32_t GetMaxBlockSize();

protected:
	int32_t MallocMemBlock(int32_t nBytes, int32_t nWantCount);

	MemBlockInfo *CreateMemBlockInfo(int32_t nBytes);

	MemBlockInfo *GetMemBlockInfo(int32_t nIndex);

	int32_t GetTableIndexByBytes(int32_t nDataSize);

protected:
	//�����ڴ���С
	uint32_t			m_nMaxBlockSize;

	MemInfoTable		m_stMemInfoTable;
	//�ڴ��й¶�ĸ���
	uint32_t			m_nMemLeakCount;

	//�ڴ���ʹ�ü�¼
	CriticalSection		m_stBlkAddrLock;
	MemAddressRecord	m_stBlkAddrRcd;
	CriticalSection		m_stHeapAddrLock;
	MemAddressRecord	m_stHeapAddrRcd;

	CriticalSection		m_stAllocMemRecordLock;
	MemRecordMap		m_stAllocMemRecordMap;

	CriticalSection		m_stRecycleMemRecordLock;
	MemRecordMap		m_stRecycleMemRecordMap;
};

#define	CREATE_FRAMEMEMMGT_INSTANCE				CSingleton<CFrameMemMgt>::CreateInstance
#define	g_FrameMemMgt							CSingleton<CFrameMemMgt>::GetInstance()
#define	DESTROY_FRAMEMEMMGT_INSTANCE			CSingleton<CFrameMemMgt>::DestroyInstance

//��ȡ����ڴ��С
#define MaxBlockSize		g_FrameMemMgt.GetMaxBlockSize()

/*#define MALLOC(size)	\
	((size > MaxBlockSize) ? (new(nothrow) uint8_t[size]) : \
		(g_FrameMemMgt.RecordAllocInfo(__FILE__, __LINE__, g_FrameMemMgt.GetBlockSize(size)),	\
		g_FrameMemMgt.AddBlkAddrRcd(g_FrameMemMgt.AllocBlock(g_FrameMemMgt.GetBlockSize(size)))))
*/

/*#define FREE(addr)		\
	do{	\
		uint8_t *ptr = reinterpret_cast<uint8_t *>(addr);	\
		if((ptr) != NULL)		\
		{	\
			if(!g_FrameMemMgt.HasBlkAddrRcd(ptr))	\
			{	\
				delete ptr;	\
			}	\
			else	\
			{	\
				MemBlockHead *pHead = (MemBlockHead *)(ptr - sizeof(MemBlockHead));		\
				g_FrameMemMgt.RecordRecycleInfo(__FILE__, __LINE__, pHead->m_nBlockSize);		\
				g_FrameMemMgt.DelBlkAddrRcd(ptr);	\
				g_FrameMemMgt.RecycleBlock(ptr);		\
			}	\
		}	\
	}while(false)
*/

//�������ü���
int32_t IncReferCount(uint8_t *pMem);
//�������ü���
int32_t DecReferCount(uint8_t *pMem);
//��ȡ���ü���
int32_t GetReferCount(uint8_t *pMem);

uint8_t* frame_malloc(uint32_t size, char *pFileName, int32_t nLineNo);

void frame_free(void *addr, char *pFileName, int32_t nLineNo);

#define MALLOC(size) 			frame_malloc(size, __FILE__, __LINE__)

#define FREE(addr)				frame_free(addr, __FILE__, __LINE__)

#define NEW(cls)				new(MALLOC(sizeof(cls))) cls()

#define DELETE(obj)				FREE(obj)


FRAME_NAMESPACE_END

#endif /* FRAME_MEM_MGT_H_ */
