/*
 * lightframe_impl.h
 *
 *  Created on: 2012-10-30
 *      Author: jimm
 */

#ifndef LIGHTFRAME_IMPL_H_
#define LIGHTFRAME_IMPL_H_

#include "frame_typedef.h"
#include "frame_msg_impl.h"
#include "common/common_memory.h"
#include "frame_protocolhead.h"
#include "common/common_tcpsocket.h"

FRAME_NAMESPACE_BEGIN

//Server���ýӿ�
class IServerConfig_Impl
{
public:
	virtual ~IServerConfig_Impl() {};

	virtual const char* GetServerName()		= 0;
	//��ȡServer����
	virtual int32_t GetZoneID()				= 0;

	//TODO(ServerType�̻���Frame�������ļ�, ����汾����)
	virtual EntityType GetServerType()			= 0;
	virtual ServerID GetServerID()			= 0;
	virtual LogLevel GetLogLevel()			= 0;
	virtual int32_t GetLogFileSize()		= 0;
};

//�������Ľӿ�
class IConfigCenter
{
public:
	virtual ~IConfigCenter() {};
	virtual int32_t Initialize(const char* szFileName = NULL, const int32_t type=0) = 0;
	virtual int32_t Reload(const char* szFileName = NULL, const int32_t type=0) = 0;
	virtual int32_t Uninitialize() = 0;
};

//�������Ľӿ�
class IDataCenter
{
public:
	virtual ~IDataCenter() {};

	virtual const char* GetName() = 0;

	//��Frame�ṩ�Լ������ڴ��С�Ĳ�ѯ�ӿ�
	virtual size_t GetSize() = 0;

	//��ʼ���ӿڵ��õ�ʱ�� �����ڴ��ѷ���
	virtual int32_t Initialize() = 0;
	virtual int32_t Resume() = 0;
	virtual int32_t Uninitialize() = 0;
};


//���м̳�ISessionData�����඼Ҫ�������м�������ĺ�
#define SESSION_INTERFACE() 	\
	public:	\
	int32_t GetSize(){return sizeof(*this);}

#define SESSION_DATA_BEGIN(class_name)		\
	class class_name : public ISessionData	\
	{	\
		SESSION_INTERFACE()

#define SESSION_DATA_MEMBER(type, member)	\
	type		member;

#define SESSION_DATA_END()	};

class CFrameTimer;

//��ʱ���¼��ӿ�
class ITimerEvent
{
public:
	virtual ~ITimerEvent() {};
	//��ʱ���¼�
	virtual int32_t OnTimerEvent(CFrameTimer *pTimer) = 0;
};

typedef int32_t (ITimerEvent::*TimerProc)(CFrameTimer *pTimer);

class CSocket;

//Message�¼��ӿ�
class IMessageEventCS
{
public:
	virtual ~IMessageEventCS() {};

	//��Ϣ�¼�
	virtual int32_t OnMessageEvent(CSocket *pConnection, MessageHeadCS * pMsgHead, IMsgBody* pMsgBody,
			const uint16_t nOptionLen = 0, const void *pOptionData = NULL) = 0;

};

typedef int32_t (IMessageEventCS::*CS_MSG_PROC)(CSocket *pConnection, MessageHeadCS *pMsgHead, IMsgBody *pMsgBody,
			const uint16_t nOptionLen, const void *pOptionData);


class IIOHandler
{
public:
	virtual ~IIOHandler(){};

	virtual int32_t ReadEvent() = 0;

	virtual int32_t WriteEvent() = 0;

	virtual int32_t ErrorEvent() = 0;
};

class ISystemEvent
{
public:
	virtual ~ISystemEvent() {};

	virtual int32_t OnSystemEvent(uint16_t nEventID, void *pParam) = 0;
};

typedef int32_t (*SYS_EVENT_PROC)(uint16_t nEventID, void *pParam);


class ITimerData
{
public:
	virtual ~ITimerData(){};

	virtual int32_t GetSize() = 0;
};
//���м̳�ITimerData�����඼Ҫ�������м�������ĺ�
#define TIMER_INTERFACE() 	\
	public:	\
	int32_t GetSize(){return sizeof(*this);}

#define TIMER_DATA_BEGIN(class_name)	\
	class class_name : public ITimerData	\
	{	\
		TIMER_INTERFACE()

#define TIMER_DATA_END()	};


class CSocket;

//Frame�߼����ܽӿ�
class IFrame_Impl
{
public:
	virtual ~IFrame_Impl() {};

	virtual void RegistMsgHandler(uint32_t nMsgID, IMessageEventCS* pHandle, CS_MSG_PROC proc = NULL) = 0;

	//ע��ϵͳ�¼�
	virtual void RegistSysEvent(uint16_t nEventID, ISystemEvent* pHandle) = 0;
	//������Ϣ
	virtual int32_t PostMessage(MessageHeadCS* pMessageHead, IMsgBody *pMsgBody, CSocket *pSocket) = 0;

	//������Ϣ
	virtual int32_t PostMessage(MessageHeadCS* pMessageHead, const uint8_t *pMsgBodyBuf, const uint32_t nMsgBodyLen, CSocket *pSocket) = 0;

	//д��־(DEBUGģʽֱ��д�ļ�)
	virtual int32_t WriteLog(LogLevel loglevel, const char *szFunc, const int32_t lineno,
			const char *szFormat, ...) = 0;

	//���ö�ʱ��
	virtual int32_t CreateTimer(int32_t nTimerID, ITimerEvent* pHandler, ITimerData *pTimerData,
			int64_t nCycleTime, bool bLoop, TimerIndex& timerIndex) = 0;

	//ɾ����ʱ��
	virtual int32_t RemoveTimer(const TimerIndex timerIndex) = 0;

	virtual void SetDaemon() = 0;
};

FRAME_NAMESPACE_END

#endif /* LIGHTFRAME_IMPL_H_ */
