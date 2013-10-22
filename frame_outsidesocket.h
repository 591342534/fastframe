/*
 * frame_outsidesocket.h
 *
 *  Created on: 2012-11-20
 *      Author: jimm
 */

#ifndef FRAME_OUTSIDESOCKET_H_
#define FRAME_OUTSIDESOCKET_H_

#include "common/common_codeengine.h"
#include "frame_namespace.h"
#include "frame_socket.h"
//#include "frame_socket_mgt.h"
#include "frame_protocolhead.h"

FRAME_NAMESPACE_BEGIN

//���Կͻ��˵����ݰ�������װ�������С
#define OUTSIDE_PACKET_SIZE			(0xffff + sizeof(ConnUin))

class COutsideSocket : public CSocket
{
public:
	COutsideSocket();

	virtual ~COutsideSocket();

	virtual void Reset();

	virtual void Clear();

	virtual int32_t OnRead(int32_t nErrorCode);

	virtual int32_t OnWrite(int32_t nErrorCode);

	virtual int32_t OnError(int32_t nErrorCode);

	virtual int32_t OnConnected();

	virtual int32_t OnDisconnect(int32_t nErrorCode);
protected:
	//�ӽ��ջ�������ȡһ����Ϣ��
	virtual int32_t MakeMessage();

	//���ͻ�����ʣ�������
	//int32_t SendRestData();

	//�������ӹر��¼�
	int32_t MakeSystemEvent(uint16_t nEventID);

protected:
	//RoleID			m_nRoleID;
	uint16_t		m_nCurPacketSize;			//��ǰ���ĳ���
	uint16_t		m_nPacketOffset;			//�Ѿ��ж���������
	NetPacket		*m_pPacket;					//��װ��Ӧ�ò�������
};

FRAME_NAMESPACE_END

#endif /* FRAME_OUTSIDESOCKET_H_ */
