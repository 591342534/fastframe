/*
 * frame_clientsocket.cpp
 *
 *  Created on: 2012-12-3
 *      Author: jimm
 */

#include "frame_netqueue.h"
#include "frame_outsidesocket.h"
#include "common/common_codeengine.h"
#include "frame_eventid.h"
#include "frame_configmgt.h"
//#include "frame_connection_mgt.h"
#include "frame_mem_mgt.h"
#include "frame_msgevent_mgt.h"

FRAME_NAMESPACE_BEGIN

COutsideSocket::COutsideSocket()
{
	Reset();
}

COutsideSocket::~COutsideSocket()
{

}

void COutsideSocket::Reset()
{
	m_nSocketAttr = enmSocketAttr_Outside;

	Clear();
}

void COutsideSocket::Clear()
{
	CSocket::Clear();

	//m_nRoleID = enmInvalidRoleID;
	if(m_pPacket != NULL)
	{
		FREE((uint8_t *)m_pPacket);
	}
	m_pPacket = NULL;
	m_nCurPacketSize = 0;
	m_nPacketOffset = 0;
}

int32_t COutsideSocket::OnRead(int32_t nErrorCode)
{
	uint8_t arrBuf[enmMaxMessageSize];

	int32_t nCloseCode = 0;
	int32_t nRecvBytes = 0;
	int32_t nRet = Recv(arrBuf, enmMaxMessageSize, nRecvBytes);
	if(nRet != S_OK)
	{
		nCloseCode = nRet;
	}

	m_stRecvBuffer.Write(arrBuf, nRecvBytes);
	//��ȡ��Ϣ��
	MakeMessage();

	if(nCloseCode != 0)
	{
		CloseSocket(nCloseCode);
		return E_SOCKETERROR;
	}

	return S_OK;
}

int32_t COutsideSocket::OnWrite(int32_t nErrorCode)
{
	if(nErrorCode != 0)
	{
		CloseSocket(SYS_EVENT_CONN_ERROR);
		return E_SOCKETERROR;
	}

	int32_t nRet = SendRestData();
	if(nRet < 0)
	{
		CloseSocket(SYS_EVENT_CONN_ERROR);
		return E_SOCKETERROR;
	}

	return S_OK;
}

int32_t COutsideSocket::OnError(int32_t nErrorCode)
{
	CloseSocket(SYS_EVENT_CONN_ERROR);
	return S_OK;
}

int32_t COutsideSocket::OnConnected()
{
	//�����ӳɹ��ǲ��������ݵ�
	MakeMessage();

	MakeSystemEvent(SYS_EVENT_INITIATIVE_CONNECT_SUCCESS);

	return S_OK;
}

int32_t COutsideSocket::OnDisconnect(int32_t nCloseCode)
{
	m_pEpoll->DeleteEvent(this);

	//û��ͨ����֤�ľͲ���Ӧ�ò㷢�ͶϿ������¼�
	//if(g_FrameConnectionMgt.GetConnection(m_nSocketFD) != NULL)
	{
		MakeMessage();

		//�ͻ��������ر�
		if(nCloseCode == (int32_t)E_SOCKETDISCONNECTED)
		{
			MakeSystemEvent(SYS_EVENT_CONN_CLIENT_CLOSED);
		}
		else
		{
			MakeSystemEvent(SYS_EVENT_CONN_ERROR);
		}
	}

//	//���peer�����socket�����ӳ��
//	g_FrameSocketMgt.DelSocket(m_nPeerType, m_nSocketFD);
//	//���մ�socket����
//	g_FrameSocketMgt.DestroySocketObject(this);

	return S_OK;
}

//�������ӹر��¼�
int32_t COutsideSocket::MakeSystemEvent(uint16_t nEventID)
{
	int32_t nPacketLen = sizeof(NetPacket) + sizeof(ConnInfo);
	uint8_t *pMem = MALLOC(nPacketLen);//g_FrameMemMgt.AllocBlock(nPacketLen);
	if(pMem == NULL)
	{
		return E_NULLPOINTER;
	}

	NetPacket *pNetPacket = new(pMem)NetPacket();
	pNetPacket->m_nNetPacketLen = nPacketLen;

	ConnInfo *pConnInfo = new(&pNetPacket->m_pNetPacket[0])ConnInfo;
	pConnInfo->pSocket = this;
	pConnInfo->nTunnelIndex = (uint16_t)m_nSocketFD;
	pConnInfo->nErrorCode = nEventID;

	g_FrameNetQueue.PushRecvCSQueue(pNetPacket);

	return S_OK;
}

int32_t COutsideSocket::MakeMessage()
{
	//��ʼ���
	while(m_stRecvBuffer.Size() > 0)
	{
		MessageHeadCS stMsgHead;
		if(m_nCurPacketSize <= 0)
		{
			//��ȡ����С�ֶ���ռ���ֽ���
			int32_t nPacketLengthSize = sizeof(stMsgHead.nTotalSize);
			if(m_stRecvBuffer.Size() < nPacketLengthSize)
			{
				break;
			}

			//ȡ����ͷǰ��İ���С�ֶΣ�Ȼ��������
			uint8_t arrPacketSize[nPacketLengthSize];
			m_stRecvBuffer.PeekRead(arrPacketSize, nPacketLengthSize);

			uint32_t nOffset = 0;
			uint32_t nPacketSize = *((uint32_t *)arrPacketSize);
			m_nCurPacketSize = nPacketSize + nPacketLengthSize;
			//CCodeEngine::Decode(arrPacketSize, nPacketLengthSize, nOffset, m_nCurPacketSize);
		}

		int32_t nWantDataSize = (int32_t)(m_nCurPacketSize - m_nPacketOffset);
		int32_t nRealGetDataSize = (m_stRecvBuffer.Size() >= nWantDataSize ? nWantDataSize : m_stRecvBuffer.Size());
		if(nRealGetDataSize <= 0)
		{
			break;
		}

		if(m_pPacket == NULL)
		{
			int32_t nPacketLen = m_nCurPacketSize + sizeof(ConnInfo);
			uint8_t *pMem = MALLOC(sizeof(NetPacket) + nPacketLen);
			if(pMem == NULL)
			{
				break;
			}

			m_pPacket = new(pMem)NetPacket();
			m_pPacket->m_nNetPacketLen = nPacketLen;
		}

		//ǰ����conninfo�ṹ
		uint8_t *pPacketHead = &m_pPacket->m_pNetPacket[sizeof(ConnInfo)];

		if(m_stRecvBuffer.Read(&pPacketHead[m_nPacketOffset], nRealGetDataSize) <= 0)
		{
			break;
		}

		m_nPacketOffset += nRealGetDataSize;
		if(m_nPacketOffset >= m_nCurPacketSize)
		{
			uint32_t nOffset = 0;
			int32_t nRet = stMsgHead.MessageDecode(pPacketHead, m_nPacketOffset, nOffset);
			if(nRet < 0)
			{
				FREE((uint8_t *)m_pPacket);
				m_pPacket = NULL;
				m_nPacketOffset = 0;
				m_nCurPacketSize = 0;
				return nRet;
			}
			//m_nRoleID = stMsgHead.nRoleID;

			ConnInfo *pConnInfo = new(&m_pPacket->m_pNetPacket[0]) ConnInfo;
			pConnInfo->pSocket = this;
			pConnInfo->nTunnelIndex = (uint16_t)m_nSocketFD;
			pConnInfo->nErrorCode = 0;

			g_FrameNetQueue.PushRecvCSQueue(m_pPacket);
			m_pPacket = NULL;
			m_nPacketOffset = 0;
			m_nCurPacketSize = 0;
		}
	}

	return S_OK;
}

FRAME_NAMESPACE_END
