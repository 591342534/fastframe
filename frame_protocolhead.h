/*
 * frame_protocolhead.h
 *
 *  Created on: 2011-11-23
 *      Author: jimm
 */

#ifndef FRAME_PROTOCOLHEAD_H_
#define FRAME_PROTOCOLHEAD_H_

#include "common/common_typedef.h"
#include "common/common_codeengine.h"
#include "common/common_string.h"
#include "frame_namespace.h"
#include "frame_typedef.h"
#include "frame_msg_impl.h"
#include <string.h>
#include <stdio.h>

FRAME_NAMESPACE_BEGIN

typedef uint8_t						MessageType;		//��Ϣ����
enum
{
	enmMessageType_None				= 0x00,				//��Чֵ
	enmMessageType_Request			= 0x01,				//����
	enmMessageType_Response			= 0x02,				//��Ӧ
	enmMessageType_Notify			= 0x03,				//֪ͨ
	enmMessageType_Other			= 0x04,				//����
};

typedef uint8_t						TransType;			//��������
enum
{
	enmTransType_None				= 0xFF,				//��Чֵ
	enmTransType_P2P				= 0,				//��Ե�
	enmTransType_P2G				= 1,				//�鲥
	enmTransType_Broadcast			= 2,   				//�㲥
	enmTransType_ByKey				= 3,				//���ݹؼ���ת��
	enmTransType_Regist				= 4,				//Serverע��
	enmTransType_Ramdom				= 5,				//��ָ����Ŀ��servertype�����ѡ��һ������
	enmTransType_Disconnect         = 6,				//server���ӶϿ�
};

enum
{
	enmMaxOptionalCount				= 128,				//����ѡ�ֶδ�С
	enmMaxDestinationCout			= 126,				//�鲥ʱ���Ŀ�ĵ�����
};

enum
{
	enmMinCSHeadMessageLength		= 28,			//CS��Ϣͷ����С����
};

/*********************************************************************
* ͨ��Э��������ݽṹ����
*********************************************************************/


//CS��Ϣͷ��
class MessageHeadCS
{
public:
	uint32_t		nTotalSize;		//�����������ĳ���,�����packSecHead��������packSecHead�ĳ���
	uint8_t			nHaveSecHead;	//�Ƿ��еڶ���ͷ
	uint16_t		nMessageID;		//��Ϣid

	enum
	{
		enmInvalidFlag		= 0,
		enmHaveSecondHead	= 210,
	};

	MessageHeadCS()
	{
		nTotalSize = 0;
		nHaveSecHead = enmHaveSecondHead;
		nMessageID = 0;
	}

	//����û�п�϶�Ľṹ�峤��
	static int32_t CompactSize()
	{
		return (sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t));
	}

	static int32_t HeadSize()
	{
		return CompactSize() - sizeof(uint32_t);
	}

	int32_t MessageEncode(uint8_t* buf, const uint32_t size, uint32_t& offset)
	{
		int32_t ret = S_OK;
		*((uint32_t *)(buf + offset)) = nTotalSize;
		offset += sizeof(nTotalSize);

		*(buf + offset) = nHaveSecHead;
		offset += sizeof(nHaveSecHead);

		*((uint16_t *)(buf + offset)) = nMessageID;
		offset += sizeof(nMessageID);

		return ret;
	}

	int32_t MessageDecode(const uint8_t *buf, const uint32_t size, uint32_t& offset)
	{
		int32_t ret = S_OK;
		nTotalSize = *((uint32_t *)(buf + offset));
		offset += sizeof(nTotalSize);

		nHaveSecHead = *(buf + offset);
		offset += sizeof(nHaveSecHead);

		nMessageID = *((uint16_t *)(buf + offset));
		offset += sizeof(nMessageID);

		return ret;
	}

	void Dump(char* buf, const uint32_t size, uint32_t& offset)
	{
	    sprintf(buf + offset, "{nTotalSize=%u, nHaveSecHead=%d, nMessageID=%d}",
	    		nTotalSize, nHaveSecHead, nMessageID);
	    offset = (uint32_t)strlen(buf);
	}
};



class ConnUin
{
public:
	void					*pSocket;
	uint16_t				nTunnelIndex;
	uint8_t					nErrorCode;				//������

	ConnUin()
	{
		Reset();
	}

	void Reset()
	{
		pSocket			= NULL;
		nTunnelIndex	= 0;
		nErrorCode		= 0;
	}

	void Dump(char* buf, const uint32_t size, uint32_t& offset)
	{
	    sprintf(buf + offset, "{nTunnelIndex=%d, nErrorCode=%d}",
	    		nTunnelIndex, nErrorCode);
	    offset = (uint32_t)strlen(buf);
	}
};

typedef ConnUin		ConnInfo;


class NetPacket
{
public:
	NetPacket()
	{
		m_nNetPacketLen = 0;
	}

	virtual ~NetPacket()
	{
	}

	int32_t 	m_nNetPacketLen;	//����������4�ֽڵĳ���
	uint8_t		m_pNetPacket[0];
};

FRAME_NAMESPACE_END

#endif /* FRAME_PROTOCOLHEAD_H_ */
