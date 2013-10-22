/*
 * frame_mem_mgt.cpp
 *
 *  Created on: 2012-12-18
 *      Author: jimm
 */

#include "common/common_errordef.h"
#include "frame_mem_mgt.h"
#include <string.h>

FRAME_NAMESPACE_BEGIN

CFrameMemMgt::CFrameMemMgt()
{
	m_nMaxBlockSize = 0;
	m_nMemLeakCount = 0;
}

CFrameMemMgt::~CFrameMemMgt()
{

}

int32_t CFrameMemMgt::Initialize()
{
	int32_t nWantCount = 0;
	//��MAM_BLOCK_SIZE_STEP�ֽڵ�n * MAM_BLOCK_SIZE_STEP�ֽڸ�����0���ڴ��,��Ҫ��Ϊ�������ڴ������
	for(int32_t i = 1; i <= 1024 * 1024; ++i)
	{
		//ע�⣬��ʼ���ڴ�һ��Ҫ��֤���̲߳�����
		int32_t nMallocCount = MallocMemBlock(i * MAM_BLOCK_SIZE_STEP, nWantCount);
		if(nMallocCount < nWantCount)
		{
			return -1;
		}
	}

	return S_OK;
}

int32_t CFrameMemMgt::Resume()
{
	return S_OK;
}

int32_t CFrameMemMgt::Uinitialize()
{
	return S_OK;
}

uint8_t *CFrameMemMgt::AllocBlock(int32_t nWantSize)
{
	if(nWantSize <= 0)
	{
		return NULL;
	}

	int32_t nIndex = GetTableIndexByBytes(nWantSize);
	if(nIndex <= 0)
	{
		return NULL;
	}

	MemBlockInfo *pMemBlockInfo = GetMemBlockInfo(nIndex);
	if(pMemBlockInfo == NULL)
	{
		return NULL;
	}

	MUTEX_GUARD(MemBlockLock, pMemBlockInfo->m_stMemBlockLock);

	if(pMemBlockInfo->m_stMemBlockList.empty())
	{
		MallocMemBlock(pMemBlockInfo->m_nBlockSize, ALLOC_MEM_STEP);
	}

	if(pMemBlockInfo->m_stMemBlockList.empty())
	{
		return NULL;
	}

	uint8_t *pMem = NULL;
	MemBlockHead *pHead = NULL;
	while(!pMemBlockInfo->m_stMemBlockList.empty())
	{
		uint8_t *pMemTemp = pMemBlockInfo->m_stMemBlockList.front();
		pMemBlockInfo->m_stMemBlockList.pop_front();
		//�������ü���
		pHead = (MemBlockHead *)(pMemTemp - sizeof(MemBlockHead));
		//���ڴ��Ƿ�����ʹ��
		if(pHead->m_nReferCount > 0)
		{
			RecordMemLeakInfo(pMemTemp);
			continue;
		}
		pMem = pMemTemp;
		++pHead->m_nReferCount;
		break;
	}

	return pMem;
}

void CFrameMemMgt::RecycleBlock(uint8_t *pMemBlock)
{
	if(pMemBlock == NULL)
	{
		return;
	}

	MemBlockHead *pHead = (MemBlockHead *)(pMemBlock - sizeof(MemBlockHead));
	int32_t nIndex = pHead->m_nIndex;
	if(m_stMemInfoTable.count(nIndex) <= 0)
	{
		RecordMemLeakInfo(pMemBlock);
		return;
	}

	//���ڴ���С
	uint32_t nBlockSize = pHead->m_nBlockSize;
	if(nBlockSize == 0 || nBlockSize > m_nMaxBlockSize)
	{
		RecordMemLeakInfo(pMemBlock);
		return;
	}

//	int32_t nIndex = GetTableIndexByBytes(nBlockSize);
//	if(nIndex <= 0)
//	{
//		RecordMemLeakInfo(pMemBlock);
//		return;
//	}

	MemBlockInfo *pMemBlockInfo = GetMemBlockInfo(nIndex);
	if(pMemBlockInfo == NULL)
	{
		RecordMemLeakInfo(pMemBlock);
		return;
	}

	if(pMemBlockInfo->m_nBlockSize != nBlockSize)
	{
		RecordMemLeakInfo(pMemBlock);
		return;
	}

	//���ü�����һ
	if(pHead->m_nReferCount > 0)
	{
		--pHead->m_nReferCount;

		MUTEX_GUARD(MemBlockLock, pMemBlockInfo->m_stMemBlockLock);

		pMemBlockInfo->m_stMemBlockList.push_front(pMemBlock);
	}
	else
	{
		RecordMemLeakInfo(pMemBlock);
	}
}

//��ӿ��ڴ�����ʹ�ü�¼
uint8_t* CFrameMemMgt::AddBlkAddrRcd(uint8_t *pBlock)
{
	MUTEX_GUARD(lock, m_stBlkAddrLock);
	if(pBlock != NULL)
	{
		if(m_stBlkAddrRcd.count(pBlock) > 0)
		{
			//todo : trouble
		}
		else
		{
			m_stBlkAddrRcd.insert(pBlock);
		}
	}

	return pBlock;
}

//ɾ�����ڴ�����ʹ�ü�¼
void CFrameMemMgt::DelBlkAddrRcd(uint8_t *pBlock)
{
	MUTEX_GUARD(lock, m_stBlkAddrLock);
	if(pBlock != NULL)
	{
		if(m_stBlkAddrRcd.count(pBlock) > 0)
		{
			m_stBlkAddrRcd.erase(pBlock);
		}
	}
}

//�Ƿ���ڴ�������ʹ�ü�¼
bool CFrameMemMgt::HasBlkAddrRcd(uint8_t *pBlock)
{
	MUTEX_GUARD(lock, m_stBlkAddrLock);
	return m_stBlkAddrRcd.count(pBlock) > 0;
}

//��Ӷ��ڴ�����ʹ�ü�¼
uint8_t* CFrameMemMgt::AddHeapAddrRcd(uint8_t *pBlock)
{
	MUTEX_GUARD(lock, m_stHeapAddrLock);
	if(pBlock != NULL)
	{
		if(m_stHeapAddrRcd.count(pBlock) > 0)
		{
			//todo : trouble
		}
		else
		{
			m_stHeapAddrRcd.insert(pBlock);
		}
	}

	return pBlock;
}

//ɾ�����ڴ�����ʹ�ü�¼
void CFrameMemMgt::DelHeapAddrRcd(uint8_t *pBlock)
{
	MUTEX_GUARD(lock, m_stHeapAddrLock);
	if(pBlock != NULL)
	{
		if(m_stHeapAddrRcd.count(pBlock) > 0)
		{
			m_stHeapAddrRcd.erase(pBlock);
		}
		delete pBlock;
	}
}

//�Ƿ��ж��ڴ�����ʹ�ü�¼
bool CFrameMemMgt::HasHeapAddrRcd(uint8_t *pBlock)
{
	MUTEX_GUARD(lock, m_stHeapAddrLock);
	return m_stHeapAddrRcd.count(pBlock) > 0;
}

//��¼�ڴ�й¶��Ϣ
void CFrameMemMgt::RecordMemLeakInfo(uint8_t *pMemBlock)
{
	if(pMemBlock != NULL)
	{
		++m_nMemLeakCount;
	}
}

#include <stdio.h>
//ͳ��Ŀǰ�����ڴ�������
void CFrameMemMgt::PrintMemBlockInfo()
{
	int32_t nOffset = 0;
	static char strMemBlockInfo[1024 * 1024];

	memset(strMemBlockInfo, 0, sizeof(strMemBlockInfo));

	sprintf(strMemBlockInfo + nOffset, "frame mem block info : {");
	nOffset = strlen(strMemBlockInfo);

	//���ڴ�����Ϣ
	for(MemInfoTable::iterator it = m_stMemInfoTable.begin(); it != m_stMemInfoTable.end(); it++)
	{
		MemBlockInfo *pMemBlockInfo = it->second;
		if(pMemBlockInfo == NULL)
		{
			continue;
		}

		if(pMemBlockInfo->m_nBlockCount <= ALLOC_MEM_STEP)
		{
			continue;
		}

		MUTEX_GUARD(MemBlockLock, pMemBlockInfo->m_stMemBlockLock);

		sprintf(strMemBlockInfo + nOffset, "{BlockSize(%d) : BlockCount(%d) : LeftCount(%d)}",
				pMemBlockInfo->m_nBlockSize, pMemBlockInfo->m_nBlockCount,
				(int32_t)pMemBlockInfo->m_stMemBlockList.size());
		nOffset = strlen(strMemBlockInfo);
	}

	sprintf(strMemBlockInfo + nOffset, ", m_nMemLeakCount = %d}", m_nMemLeakCount);
	nOffset = strlen(strMemBlockInfo);

	//g_FrameLogEngine.WriteMainLog(enmLogLevel_Notice, "%s\n", strMemBlockInfo);
	printf("%s\n", strMemBlockInfo);

	//�ڴ������¼
	nOffset = 0;
	memset(strMemBlockInfo, 0, sizeof(strMemBlockInfo));

	sprintf(strMemBlockInfo + nOffset, "frame mem malloc record : {");
	nOffset = strlen(strMemBlockInfo);

	m_stAllocMemRecordLock.enter();

	for(MemRecordMap::iterator it = m_stAllocMemRecordMap.begin(); it != m_stAllocMemRecordMap.end(); it++)
	{
		LineNoMap *pLineNoMap = it->second;
		for(LineNoMap::iterator st = pLineNoMap->begin(); st != pLineNoMap->end(); st++)
		{
			BlockSizeMap *pBlockSizeMap = st->second;
			for(BlockSizeMap::iterator pt = pBlockSizeMap->begin(); pt != pBlockSizeMap->end(); pt++)
			{
				MemOperationRecord *pRecord = pt->second;
				if(pRecord == NULL)
				{
					continue;
				}

				sprintf(strMemBlockInfo + nOffset, "{FileName=%s, LineNo=%d, BlockSize=%u, MallocCount=%u}",
						it->first.c_str(), pRecord->m_nLineNo, pRecord->m_nBlockSize, pRecord->m_nOperationCount);
				nOffset = strlen(strMemBlockInfo);
			}
		}
	}

	m_stAllocMemRecordLock.leave();

	sprintf(strMemBlockInfo + nOffset, "}");
	nOffset = strlen(strMemBlockInfo);

	//g_FrameLogEngine.WriteMainLog(enmLogLevel_Notice, "%s\n", strMemBlockInfo);
	printf("%s\n", strMemBlockInfo);

	//�ڴ��ͷż�¼
	nOffset = 0;
	memset(strMemBlockInfo, 0, sizeof(strMemBlockInfo));

	sprintf(strMemBlockInfo + nOffset, "frame mem free record : {");
	nOffset = strlen(strMemBlockInfo);

	m_stRecycleMemRecordLock.enter();

	for(MemRecordMap::iterator it = m_stRecycleMemRecordMap.begin(); it != m_stRecycleMemRecordMap.end(); it++)
	{
		LineNoMap *pLineNoMap = it->second;
		for(LineNoMap::iterator st = pLineNoMap->begin(); st != pLineNoMap->end(); st++)
		{
			BlockSizeMap *pBlockSizeMap = st->second;
			for(BlockSizeMap::iterator pt = pBlockSizeMap->begin(); pt != pBlockSizeMap->end(); pt++)
			{
				MemOperationRecord *pRecord = pt->second;
				if(pRecord == NULL)
				{
					continue;
				}

				sprintf(strMemBlockInfo + nOffset, "{FileName=%s, LineNo=%d, BlockSize=%u, FreeCount=%u}",
						it->first.c_str(), pRecord->m_nLineNo, pRecord->m_nBlockSize, pRecord->m_nOperationCount);
				nOffset = strlen(strMemBlockInfo);
			}
		}
	}

	m_stRecycleMemRecordLock.leave();

	sprintf(strMemBlockInfo + nOffset, "}");
	nOffset = strlen(strMemBlockInfo);

	//g_FrameLogEngine.WriteMainLog(enmLogLevel_Notice, "%s\n", strMemBlockInfo);
	printf("%s\n", strMemBlockInfo);
}

int32_t CFrameMemMgt::GetBlockSize(int32_t nWantSize)
{
	int32_t nBlockSize = 0;
	if(nWantSize % MAM_BLOCK_SIZE_STEP == 0)
	{
		nBlockSize = nWantSize;
	}
	else
	{
		nBlockSize = ((nWantSize / MAM_BLOCK_SIZE_STEP) + 1) * MAM_BLOCK_SIZE_STEP;
	}

	return nBlockSize;
}

//ͳ��������Ϣ
void CFrameMemMgt::RecordAllocInfo(const char*pFileName, int32_t nLineNo, uint32_t nBlockSize)
{
	//if(pFileName == NULL)
	{
		return;
	}

	MUTEX_GUARD(AllocMemRecordLock, m_stAllocMemRecordLock);

	//�Ѿ����ļ���¼
	if(m_stAllocMemRecordMap.count(string(pFileName)) > 0)
	{
		LineNoMap *pLineNoMap = m_stAllocMemRecordMap[string(pFileName)];
		BlockSizeMap *pBlockSizeMap = NULL;
		MemOperationRecord *pRecord = NULL;
		//��ȡ�кź�blocksize��ӳ��
		LineNoMap::iterator st = pLineNoMap->find(nLineNo);
		//û���ҵ��Ļ��ʹ���һ��ӳ��
		if(st == pLineNoMap->end())
		{
			pBlockSizeMap = new(nothrow) BlockSizeMap();
			if(pBlockSizeMap == NULL)
			{
				return;
			}

			pLineNoMap->insert(make_pair(nLineNo, pBlockSizeMap));
		}
		//�ҵ��˻���Ҫ�鿴��û��blocksize����Ӧ���ݼ�¼�����ӳ��
		else
		{
			pBlockSizeMap = st->second;
			//�ҵ���blocksize��Ӧ��MemOperationRecord
			if(pBlockSizeMap->count(nBlockSize) > 0)
			{
				pRecord = (*pBlockSizeMap)[nBlockSize];
			}
		}

		//û���ҵ���Ӧ��record����
		if(pRecord == NULL)
		{
			pRecord = new(nothrow) MemOperationRecord();
			if(pRecord == NULL)
			{
				return;
			}

			pRecord->m_nLineNo = nLineNo;
			pRecord->m_nBlockSize = nBlockSize;

			pBlockSizeMap->insert(make_pair(nBlockSize, pRecord));
		}

		++pRecord->m_nOperationCount;
	}
	//û���ļ������кŵ�ӳ��ʹ���ӳ��
	else
	{
		LineNoMap *pLineNoMap = new(nothrow) LineNoMap;
		if(pLineNoMap == NULL)
		{
			return;
		}

		BlockSizeMap *pBlockSizeMap = new(nothrow) BlockSizeMap();
		if(pBlockSizeMap == NULL)
		{
			delete pLineNoMap;
			return;
		}

		MemOperationRecord *pRecord = new(nothrow) MemOperationRecord();
		if(pRecord == NULL)
		{
			delete pBlockSizeMap;
			delete pLineNoMap;
			return;
		}

		pRecord->m_nLineNo = nLineNo;
		pRecord->m_nBlockSize = nBlockSize;
		pRecord->m_nOperationCount = 1;

		pBlockSizeMap->insert(make_pair(nBlockSize, pRecord));
		pLineNoMap->insert(make_pair(nLineNo, pBlockSizeMap));
		m_stAllocMemRecordMap[string(pFileName)] = pLineNoMap;
	}
}

//ͳ���ͷ���Ϣ
void CFrameMemMgt::RecordRecycleInfo(const char*pFileName, int32_t nLineNo, uint32_t nBlockSize)
{
	//if(pFileName == NULL)
	{
		return;
	}

	MUTEX_GUARD(RecycleMemRecordLock, m_stRecycleMemRecordLock);

	//�Ѿ����ļ���¼
	if(m_stRecycleMemRecordMap.count(string(pFileName)) > 0)
	{
		LineNoMap *pLineNoMap = m_stRecycleMemRecordMap[string(pFileName)];
		BlockSizeMap *pBlockSizeMap = NULL;
		MemOperationRecord *pRecord = NULL;
		//��ȡ�кź�blocksize��ӳ��
		LineNoMap::iterator st = pLineNoMap->find(nLineNo);
		//û���ҵ��Ļ��ʹ���һ��ӳ��
		if(st == pLineNoMap->end())
		{
			pBlockSizeMap = new(nothrow) BlockSizeMap();
			if(pBlockSizeMap == NULL)
			{
				return;
			}

			pLineNoMap->insert(make_pair(nLineNo, pBlockSizeMap));
		}
		//�ҵ��˻���Ҫ�鿴��û��blocksize����Ӧ���ݼ�¼�����ӳ��
		else
		{
			pBlockSizeMap = st->second;
			//�ҵ���blocksize��Ӧ��MemOperationRecord
			if(pBlockSizeMap->count(nBlockSize) > 0)
			{
				pRecord = (*pBlockSizeMap)[nBlockSize];
			}
		}

		//û���ҵ���Ӧ��record����
		if(pRecord == NULL)
		{
			pRecord = new(nothrow) MemOperationRecord();
			if(pRecord == NULL)
			{
				return;
			}

			pRecord->m_nLineNo = nLineNo;
			pRecord->m_nBlockSize = nBlockSize;

			pBlockSizeMap->insert(make_pair(nBlockSize, pRecord));
		}

		++pRecord->m_nOperationCount;
	}
	//û���ļ������кŵ�ӳ��ʹ���ӳ��
	else
	{
		LineNoMap *pLineNoMap = new(nothrow) LineNoMap;
		if(pLineNoMap == NULL)
		{
			return;
		}

		BlockSizeMap *pBlockSizeMap = new(nothrow) BlockSizeMap();
		if(pBlockSizeMap == NULL)
		{
			delete pLineNoMap;
			return;
		}

		MemOperationRecord *pRecord = new(nothrow) MemOperationRecord();
		if(pRecord == NULL)
		{
			delete pBlockSizeMap;
			delete pLineNoMap;
			return;
		}

		pRecord->m_nLineNo = nLineNo;
		pRecord->m_nBlockSize = nBlockSize;
		pRecord->m_nOperationCount = 1;

		pBlockSizeMap->insert(make_pair(nBlockSize, pRecord));
		pLineNoMap->insert(make_pair(nLineNo, pBlockSizeMap));
		m_stRecycleMemRecordMap[string(pFileName)] = pLineNoMap;
	}
}

//��ȡ����ڴ���С
uint32_t CFrameMemMgt::GetMaxBlockSize()
{
	return m_nMaxBlockSize;
}

int32_t CFrameMemMgt::MallocMemBlock(int32_t nBytes, int32_t nWantCount)
{
	int32_t nIndex = GetTableIndexByBytes(nBytes);
	if(nIndex <= 0)
	{
		return 0;
	}

	MemBlockInfo *pMemBlockInfo = GetMemBlockInfo(nIndex);
	if(pMemBlockInfo == NULL)
	{
		pMemBlockInfo = CreateMemBlockInfo(nBytes);
		if(pMemBlockInfo == NULL)
		{
			return 0;
		}
	}

//	MUTEX_GUARD(MemBlockLock, pMemBlockInfo->m_stMemBlockLock);

	int32_t nMallocCount = 0;
	for(int32_t i = 0; i < nWantCount; ++i)
	{
		//ǰ��8�ֽڴ���ڴ����Ϣ
		uint8_t *pMem = new(nothrow) uint8_t[pMemBlockInfo->m_nBlockSize + sizeof(MemBlockHead)];
		if(pMem == NULL)
		{
			break;
		}

		MemBlockHead *pHead = new(pMem)MemBlockHead();
		pHead->m_nIndex = nIndex;
		pHead->m_nBlockSize = pMemBlockInfo->m_nBlockSize;
		pHead->m_nReferCount = 0;
		pMemBlockInfo->m_stMemBlockList.push_back(&pMem[sizeof(MemBlockHead)]);
		++pMemBlockInfo->m_nBlockCount;

		++nMallocCount;
	}

	return nMallocCount;
}

int32_t CFrameMemMgt::GetTableIndexByBytes(int32_t nBytes)
{
	int32_t nMemTableIndex = 0;
	int32_t nIndex = nBytes / MAM_BLOCK_SIZE_STEP;
	if(nIndex < 0)
	{
		nMemTableIndex = -1;
	}
	else
	{
		if(nBytes % MAM_BLOCK_SIZE_STEP > 0)
		{
			nMemTableIndex = nIndex + 1;
		}
		else
		{
			nMemTableIndex = nIndex;
		}
	}

	return nMemTableIndex;
}


MemBlockInfo *CFrameMemMgt::CreateMemBlockInfo(int32_t nBytes)
{
	int32_t nIndex = GetTableIndexByBytes(nBytes);
	if(nIndex <= 0)
	{
		return NULL;
	}

	MemBlockInfo *pMemBlockInfo = new(nothrow) MemBlockInfo();
	if(pMemBlockInfo == NULL)
	{
		return NULL;
	}

	m_stMemInfoTable[nIndex] = pMemBlockInfo;
	pMemBlockInfo->m_nBlockSize = nBytes;

	//��������ڴ���С
	if((uint32_t)nBytes > m_nMaxBlockSize)
	{
		m_nMaxBlockSize = (uint32_t)nBytes;
	}

	return pMemBlockInfo;
}

MemBlockInfo *CFrameMemMgt::GetMemBlockInfo(int32_t nIndex)
{
	if(m_stMemInfoTable.count(nIndex) <= 0)
	{
		return NULL;
	}

	return m_stMemInfoTable[nIndex];
}

//�������ü���
int32_t IncReferCount(uint8_t *pMem)
{
	if(pMem == NULL)
	{
		return -1;
	}
	if(!g_FrameMemMgt.HasBlkAddrRcd(pMem))
	{
		return -1;
	}

	MemBlockHead *pHead = (MemBlockHead *)(pMem - sizeof(MemBlockHead));
	return ++pHead->m_nReferCount;
}

//�������ü���
int32_t DecReferCount(uint8_t *pMem)
{
	if(pMem == NULL)
	{
		return -1;
	}
	if(!g_FrameMemMgt.HasBlkAddrRcd(pMem))
	{
		return -1;
	}

	MemBlockHead *pHead = (MemBlockHead *)(pMem - sizeof(MemBlockHead));
	return --pHead->m_nReferCount;
}

//��ȡ���ü���
int32_t GetReferCount(uint8_t *pMem)
{
	if(pMem == NULL)
	{
		return -1;
	}
	if(!g_FrameMemMgt.HasBlkAddrRcd(pMem))
	{
		return -1;
	}

	MemBlockHead *pHead = (MemBlockHead *)(pMem - sizeof(MemBlockHead));
	return pHead->m_nReferCount;
}

#include <iostream>
using namespace std;

uint8_t* frame_malloc(uint32_t size, char *pFileName, int32_t nLineNo)
{
	//cout << "malloc:" << pFileName << " line: " << nLineNo << endl << flush;
	uint8_t *pMem = NULL;
	if(size > MaxBlockSize)
	{
		pMem = new(nothrow) uint8_t[size];
		g_FrameMemMgt.AddHeapAddrRcd(pMem);
	}
	else
	{
		int32_t nBlockSize = g_FrameMemMgt.GetBlockSize(size);
		pMem = g_FrameMemMgt.AllocBlock(nBlockSize);
		g_FrameMemMgt.AddBlkAddrRcd(pMem);
		g_FrameMemMgt.RecordAllocInfo(pFileName, nLineNo, nBlockSize);
	}
	//cout << "malloc : " << hex << (unsigned)pMem << dec << " --- size=" << size << endl << flush;
	return pMem;
}

void frame_free(void *addr, char *pFileName, int32_t nLineNo)
{
//	cout << "@@@@free:" << pFileName << " line: " << nLineNo << endl << flush;
	uint8_t *ptr = (uint8_t *)(addr);
	if((ptr) != NULL)
	{
		//cout << "free : " << hex << (unsigned)addr << endl << flush;
		if(g_FrameMemMgt.HasHeapAddrRcd(ptr))
		{
			g_FrameMemMgt.DelHeapAddrRcd(ptr);
		}
		else if(g_FrameMemMgt.HasBlkAddrRcd(ptr))
		{
			MemBlockHead *pHead = (MemBlockHead *)(ptr - sizeof(MemBlockHead));
			g_FrameMemMgt.RecordRecycleInfo(pFileName, nLineNo, pHead->m_nBlockSize);
			g_FrameMemMgt.DelBlkAddrRcd(ptr);
			g_FrameMemMgt.RecycleBlock(ptr);
		}
		else
		{
			cout << "######: " << pFileName << nLineNo << endl << flush;
		}
	}
}


FRAME_NAMESPACE_END

