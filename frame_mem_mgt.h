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

using namespace std;

FRAME_NAMESPACE_BEGIN

//每次申请内存的个数
#define ALLOC_MEM_STEP				1000
//内存块的增长步长
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

	int32_t			m_nIndex;				//在内存表中的索引位置
	uint32_t		m_nBlockSize;			//内存块实际大小
	uint16_t		m_nReferCount;			//正在多少地方引用
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
	uint32_t		m_nBlockSize;			//此内存块大小
	int32_t			m_nBlockCount;			//内存块的个数
	list<uint8_t *>	m_stMemBlockList;		//内存块指针链表
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

	//注意，初始化内存一定要保证单线程操作！
	int32_t Initialize();
	int32_t Resume();
	int32_t Uinitialize();

	//分配内存块
	uint8_t *AllocBlock(int32_t nWantSize);
	//回收内存块
	void RecycleBlock(uint8_t *pMemBlock);
	//添加块内存正在使用记录
	uint8_t* AddBlkAddrRcd(uint8_t *pBlock);
	//删除块内存正在使用记录
	void DelBlkAddrRcd(uint8_t *pBlock);
	//是否块内存有正在使用记录
	bool HasBlkAddrRcd(uint8_t *pBlock);
	//添加堆内存正在使用记录
	uint8_t* AddHeapAddrRcd(uint8_t *pBlock);
	//删除堆内存正在使用记录
	void DelHeapAddrRcd(uint8_t *pBlock);
	//是否有堆内存正在使用记录
	bool HasHeapAddrRcd(uint8_t *pBlock);
	//记录内存泄露信息
	void RecordMemLeakInfo(uint8_t *pMemBlock);
	//统计目前各个内存块的数量
	void PrintMemBlockInfo();

	int32_t GetBlockSize(int32_t nWantSize);
	//统计申请信息
	void RecordAllocInfo(const char*pFileName, int32_t nLineNo, uint32_t nBlockSize);
	//统计释放信息
	void RecordRecycleInfo(const char*pFileName, int32_t nLineNo, uint32_t nBlockSize);
	//获取最大内存块大小
	uint32_t GetMaxBlockSize();

protected:
	int32_t MallocMemBlock(int32_t nBytes, int32_t nWantCount);

	MemBlockInfo *CreateMemBlockInfo(int32_t nBytes);

	MemBlockInfo *GetMemBlockInfo(int32_t nIndex);

	int32_t GetTableIndexByBytes(int32_t nDataSize);

protected:
	//最大的内存块大小
	uint32_t			m_nMaxBlockSize;

	MemInfoTable		m_stMemInfoTable;
	//内存块泄露的个数
	uint32_t			m_nMemLeakCount;

	//内存块的使用记录
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

//获取最大内存大小
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

//增加引用计数
int32_t IncReferCount(uint8_t *pMem);
//减少引用计数
int32_t DecReferCount(uint8_t *pMem);
//获取引用计数
int32_t GetReferCount(uint8_t *pMem);

uint8_t* frame_malloc(uint32_t size, char *pFileName, int32_t nLineNo);

void frame_free(void *addr, char *pFileName, int32_t nLineNo);

#define MALLOC(size) 			frame_malloc(size, __FILE__, __LINE__)

#define FREE(addr)				frame_free(addr, __FILE__, __LINE__)

#define NEW(cls)				new(MALLOC(sizeof(cls))) cls()

#define DELETE(obj)				FREE(obj)


FRAME_NAMESPACE_END

#endif /* FRAME_MEM_MGT_H_ */
