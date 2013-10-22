/*
 * lightframe.cpp
 *
 *  Created on: 2012-10-31
 *      Author: jimm
 */

#include <stdarg.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>

#include "common/common_version.h"
#include "frame_logthread.h"
#include "frame_mainthread.h"
#include "frame_netthread.h"
#include "lightframe.h"


#ifndef WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#endif

#define FRAME_LOG_PATH	"./log"
#define FRAME_BLL_BASE	"./bill"
#define SERVER_NAME		"LightFrame Library"

FRAME_NAMESPACE_BEGIN

ServerCommand g_eServerCommand = enmServerCommand_None;		//����������
ServerStatus g_eServerStatus = enmServerStatus_None;		//������״̬

bool g_bTestMode = false;

//��ӡlibframe��汾��
void print_version_frame_lib()
{
	char version[128] = { 0 };
	GET_VERSION(version);

	printf("%s, %s\n", SERVER_NAME, version);
#ifdef DEBUG
	printf("Build for debug\n");
#else
	printf("Build for release\n");
#endif

#ifdef SVNVER
	printf("svn version no:%d\n", SVNVER);
#endif

#ifdef SVNDIR
	printf("source dir in svn:%s\n", SVNDIR);
#endif

	printf("\n");
}

CLightFrame::CLightFrame(const char* szServerName)
{
	memset(m_arrServerName, 0, sizeof(m_arrServerName));
	strcpy(m_arrServerName, szServerName);

	m_nAppThreadCount = 0;
	memset(m_arrAppThread, 0, sizeof(m_arrAppThread));
}

CLightFrame::~CLightFrame()
{
}

static void frame_mkdir(const char *path)
{
#ifndef WIN32
	struct stat buf;
	stat(path,&buf);

	if(S_ISDIR(buf.st_mode) == 0)
	{
		mkdir(path,0777);
	}
#endif
}

void CLightFrame::MakeDir()
{
	//��鲢����logĿ¼
	frame_mkdir(FRAME_LOG_PATH);

	//��鲢����bllĿ¼
	frame_mkdir(FRAME_BLL_BASE);

	//��鲢����bll/serverĿ¼
	char bllpath[enmMaxStringLength]={0};
	sprintf(bllpath,FRAME_BLL_BASE"/%s",m_arrServerName);
	frame_mkdir(bllpath);
}

//Frame��ʼ��
int32_t CLightFrame::Initialize()
{
	int32_t nRet = S_OK;

	//��ʼ���������
	srand((unsigned)time(NULL));

	//��鲢����Ŀ¼
	MakeDir();

	//������־����
	CREATE_FRAMELOGENGINE_INSTANCE();

	//���ɷ����������ļ���
	g_FrameConfigMgt.MakeFrameBaseConfigFilePath(m_arrServerName);
	//��ʼ�����ù�����
	nRet = g_FrameConfigMgt.Initialize();
	if(nRet < 0)
	{
		return nRet;
	}

	//��ʼ������ڴ������
	nRet = g_FrameMemMgt.Initialize();
	if(nRet < 0)
	{
		return nRet;
	}
	
//	//��ʼ����������
//	nRet = g_FrameDalMgt.Initialize();
//	if(nRet < 0)
//	{
//		return nRet;
//	}

	//��ʼ����ʱ��������
	nRet = g_FrameTimerMgt.Initialize();
	if(nRet < 0)
	{
		return nRet;
	}

//	//��ʼ���Ự������
//	nRet = g_FrameSessionMgt.Initialize();
//	if(nRet < 0)
//	{
//		return nRet;
//	}

	//��ʼ����Ϣӳ�������
	nRet = g_FrameMsgEventMgt.Initialize();
	if(nRet < 0)
	{
		return nRet;
	}

	//��ʼ�����ӹ�����
//	nRet = g_FrameSocketMgt.Initialize();
//	if(nRet < 0)
//	{
//		return nRet;
//	}

	//��ʼ��������Ϣ������
//	nRet = g_FrameConnectionMgt.Initialize();
//	if(nRet < 0)
//	{
//		return nRet;
//	}

	//��ʼ���������
	nRet = g_FrameNetQueue.Initialize();
	if(nRet < 0)
	{
		return nRet;
	}

	//��ʼ����־�̶߳���
	nRet = g_FrameLogThread.Initialize();
	if(nRet < 0)
	{
		return nRet;
	}

	//��ʼ�������̶߳���
	nRet = g_FrameNetThread.Initialize();
	if(nRet < 0)
	{
		return nRet;
	}

	//��ʼ�����̶߳���
	int32_t nAppThreadCount = g_FrameConfigMgt.GetFrameBaseConfig().GetAppThreadCount();
	if(nAppThreadCount > enmMaxAppThreadCount)
	{
		nAppThreadCount = enmMaxAppThreadCount;
	}
	for(int32_t i = 0; i < nAppThreadCount; ++i)
	{
		CFrameMainThread *pThread = new CFrameMainThread();
		if(pThread == NULL)
		{
			return E_NULLPOINTER;
		}

		nRet = pThread->Initialize();
		if(nRet < 0)
		{
			return nRet;
		}

		pThread->SetThreadIndex(i);

		m_arrAppThread[m_nAppThreadCount] = pThread;
		++m_nAppThreadCount;
	}

	nRet = g_FrameTimerTask.Start();
	if(nRet < 0)
	{
		return nRet;
	}

	g_FrameLogEngine.WriteBaseLog(enmLogLevel_Notice, "lightframe init finished, %s is ready to start!\n", m_arrServerName);

	return nRet;
}

int32_t CLightFrame::Resume()
{
	return Reload();
}

int32_t CLightFrame::Uninitialize()
{
	return S_OK;
}

int32_t CLightFrame::Start()
{
	//������־�߳�
	g_FrameLogThread.Start();

	//�������߳�
	for(int32_t i = 0; i < m_nAppThreadCount; ++i)
	{
		if(m_arrAppThread[i] == NULL)
		{
			continue;
		}

		m_arrAppThread[i]->Start();
	}

	//���������߳�
	//g_FrameNetThread.Start();

	g_FrameLogEngine.WriteBaseLog(enmLogLevel_Notice, "lightframe start finished, %s is start success!\n", m_arrServerName);

	return S_OK;
}

void CLightFrame::Run()
{
	ServerCommand eServerCommand = enmServerCommand_None;

	while (true)
	{
		eServerCommand = g_eServerCommand;

		if (enmServerCommand_Reload == eServerCommand)
		{
			//WriteLog(LOGPREFIX, enmLogLevel_Notice, "Activity server received reload signal!\n");
			g_eServerCommand = enmServerCommand_None;
			Reload();
		}
		else if (enmServerCommand_Terminate == eServerCommand)
		{
			//WriteLog(LOGPREFIX, enmLogLevel_Notice, "Activity server received terminate signal!\n");

			//֪ͨLibFrame��������
			Terminal();
			break;
		}

		if (g_bTestMode)
		{
			OnTestRequest();
		}

		//TODO:
		Delay(100000);
	}

	g_FrameLogEngine.WriteBaseLog(enmLogLevel_Notice, "%s terminated!\n", m_arrServerName);
}

int32_t CLightFrame::Reload()
{
	//��ʼ���������
	srand((unsigned)time(NULL));

	int32_t nRet = g_FrameConfigMgt.Resume();
	if(nRet < 0)
	{
		g_FrameLogEngine.WriteBaseLog(enmLogLevel_Notice, "lightframe reload is failed!{nRet=0x%08x}\n", nRet);
	}
	else
	{
		g_FrameLogEngine.WriteBaseLog(enmLogLevel_Notice, "lightframe reload finished!\n");
	}

	return nRet;
}

int32_t CLightFrame::Terminal()
{
	//��ֹ�����߳�
	//g_FrameNetThread.Terminate();

	//��ֹ���߳�
	for(int32_t i = 0; i < m_nAppThreadCount; ++i)
	{
		if(m_arrAppThread[i] == NULL)
		{
			continue;
		}

		m_arrAppThread[i]->Terminate();
	}

	//��ֹ��־�߳�
	g_FrameLogThread.Terminate();

	return S_OK;
}

//ע����������
void CLightFrame::AddConfigCenter(int32_t nType, const char *szName, IConfigCenter* pConfigCenter)
{
	g_FrameConfigMgt.AddUsrConfigCenter(nType, szName, pConfigCenter);
}


//ע����Ϣ������
void CLightFrame::RegistMsgHandler(uint32_t nMsgID, IMessageEventCS* pHandler, CS_MSG_PROC proc/* = NULL*/)
{
	g_FrameMsgEventMgt.AddMsgEvent(nMsgID, pHandler, proc);
}

//ע��ϵͳ�¼�
void CLightFrame::RegistSysEvent(uint16_t nEventID, ISystemEvent* pHandle)
{
	g_FrameMsgEventMgt.AddSysEvent(nEventID, pHandle);
}

void CLightFrame::DumpMessage(const char* szContent, MessageHeadCS *pMsgHead, IMsgBody* pMsgBody,
		const uint16_t nOptionLen, const char *pOptionData)
{
	uint32_t offset = 0;
	char szLog[enmMaxLogInfoLength];
	szLog[0] = 0;
	sprintf(szLog + offset, szContent);
	offset = (uint32_t)strlen(szLog);

	sprintf(szLog + offset, " MessageHeadCS=");
	offset = (uint32_t)strlen(szLog);
	pMsgHead->Dump(szLog, enmMaxLogInfoLength, offset);

	if(pMsgBody)
	{
		sprintf(szLog + offset, " MessageBody=");
		offset = (uint32_t)strlen(szLog);
		pMsgBody->Dump(szLog, enmMaxLogInfoLength, offset);
	}

	g_FrameLogEngine.WriteMainLog(enmLogLevel_Notice, "%s\n", szLog);
}

//������Ϣ
int32_t CLightFrame::PostMessage(MessageHeadCS* pMessageHead, IMsgBody *pMsgBody, CSocket *pSocket)
{
	int32_t nPacketLen = MessageHeadCS::CompactSize() + pMsgBody->GetSize();

	uint8_t *pMem = MALLOC(nPacketLen);
	if(pMem == NULL)
	{
		WriteLog(enmLogLevel_Error, __func__, __LINE__, "nPacketLen is too big!{nPacketLen=%d, "
				"MsgBodySize=%d}\n", nPacketLen, pMsgBody->GetSize());
		return E_NULLPOINTER;
	}

	NetPacket *pNetPacket = new(pMem)NetPacket();

	ConnInfo stConnInfo;
	stConnInfo.pSocket = pSocket;
	stConnInfo.nTunnelIndex = pSocket->GetSocketFD();
	stConnInfo.nErrorCode = 0;

	memcpy(&pNetPacket->m_pNetPacket[0], &stConnInfo, sizeof(ConnInfo));

	//ȥ��conninfo���ڴ��׵�ַ�����ݴ�С
	uint8_t *pBufCS = &pNetPacket->m_pNetPacket[sizeof(ConnInfo)];
	uint32_t nBufSizeCS = nPacketLen - sizeof(ConnInfo);

	//����ͷ�ĳ��ȸ�ֵ
	pMessageHead->nTotalSize = MessageHeadCS::HeadSize() + pMsgBody->GetSize();

	uint32_t offset = 0;
	int32_t nRet = pMessageHead->MessageEncode(pBufCS, nBufSizeCS - offset, offset);
	if (0 > nRet)
	{
		FREE(pMem);
		return nRet;
	}

	nRet = pMsgBody->MessageEncode(pBufCS, nBufSizeCS - offset, offset);
	if (0 > nRet)
	{
		FREE(pMem);
		return nRet;
	}

	pNetPacket->m_nNetPacketLen = nPacketLen;

	if(!g_FrameNetQueue.IsSendCSQueueFull())
	{
		//DumpMessage("Send Message ", pMessageHead, pMsgBody, 0, NULL);
		return g_FrameNetQueue.PushSendCSQueue(pNetPacket);
	}
	else
	{
		WriteLog(enmLogLevel_Error, __func__, __LINE__, "send queue is full!\n");
	}
	FREE(pMem);

	return S_OK;
}

//������Ϣ
int32_t CLightFrame::PostMessage(MessageHeadCS* pMessageHead, const uint8_t *pMsgBodyBuf,const uint32_t nMsgBodyLen, CSocket *pSocket)
{
	int32_t nPacketLen = MessageHeadCS::CompactSize() + nMsgBodyLen;

	uint8_t *pMem = MALLOC(nPacketLen);
	if(pMem == NULL)
	{
		WriteLog(enmLogLevel_Error, __func__, __LINE__, "nPacketLen is too big!{nPacketLen=%d, "
				"MsgBodySize=%d}\n", nPacketLen, nMsgBodyLen);
		return E_NULLPOINTER;
	}

	NetPacket *pNetPacket = new(pMem)NetPacket();

	ConnInfo stConnInfo;
	stConnInfo.pSocket = pSocket;
	stConnInfo.nTunnelIndex = pSocket->GetSocketFD();
	stConnInfo.nErrorCode = 0;

	memcpy(&pNetPacket->m_pNetPacket[0], &stConnInfo, sizeof(ConnInfo));

	//ȥ��conninfo���ڴ��׵�ַ�����ݴ�С
	uint8_t *pBufCS = &pNetPacket->m_pNetPacket[sizeof(ConnInfo)];
	uint32_t nBufSizeCS = nPacketLen - sizeof(ConnInfo);

	//����ͷ�ĳ��ȸ�ֵ
	pMessageHead->nTotalSize = MessageHeadCS::HeadSize() + nMsgBodyLen;

	uint32_t offset = 0;
	int32_t nRet = pMessageHead->MessageEncode(pBufCS, nBufSizeCS - offset, offset);
	if (0 > nRet)
	{
		FREE(pMem);
		return nRet;
	}

	memcpy(pBufCS + offset, pMsgBodyBuf, nMsgBodyLen);
	offset += nMsgBodyLen;

	pNetPacket->m_nNetPacketLen = nPacketLen;

	if(!g_FrameNetQueue.IsSendCSQueueFull())
	{
		//DumpMessage("Send Message", pMessageHead, NULL, 0, NULL);
		return g_FrameNetQueue.PushSendCSQueue(pNetPacket);
	}
	else
	{
		WriteLog(enmLogLevel_Error, __func__, __LINE__, "send queue is full!\n");
	}
	FREE(pMem);

	return S_OK;
}

//д��־
int32_t CLightFrame::WriteLog(LogLevel loglevel, const char *szFunc, const int32_t nLineNO, const char *szFormat, ...)
{
	//�жϵȼ��Ƿ��㹻
	if (loglevel < g_FrameConfigMgt.GetFrameBaseConfig().GetLogLevel())
	{
		return E_LIBF_LOGLEVEL;
	}

	char szLog[enmMaxLogInfoLength];

	//������־��Ϣ
	va_list ap;
	va_start(ap, szFormat);
	vsprintf(szLog, szFormat, ap);
	va_end(ap);

	return g_FrameLogEngine.WriteMainLog(loglevel, szFunc, nLineNO, szLog);
};

int32_t CLightFrame::WriteUserLog(char *szName, LogLevel logLevel, const char *szFileName, const int32_t nLineNO,const char* szFormat, ...)
{
	//�жϵȼ��Ƿ��㹻
	if (logLevel < g_FrameConfigMgt.GetFrameBaseConfig().GetLogLevel())
	{
		return E_LIBF_LOGLEVEL;
	}

	char szLog[enmMaxLogInfoLength];

	//������־��Ϣ
	va_list ap;
	va_start(ap, szFormat);
	vsprintf(szLog, szFormat, ap);
	va_end(ap);

	return g_FrameLogEngine.WriteUserLog(logLevel, szName, szFileName, nLineNO, szLog);
}

//���ö�ʱ��
int32_t CLightFrame::CreateTimer(int32_t nTimerID, ITimerEvent *pHandle, ITimerData *pTimerData, int64_t nCycleTime, bool bLoop, TimerIndex& timerIndex)
{
	return g_FrameTimerMgt.CreateTimer(nTimerID, pHandle, pTimerData, nCycleTime, bLoop, timerIndex);
}

//���ö�ʱ��
int32_t CLightFrame::CreateTimer(TimerProc Proc, ITimerEvent *pTimer, ITimerData *pTimerData, int64_t nCycleTime, bool bLoop, TimerIndex& timerIndex)
{
	return g_FrameTimerMgt.CreateTimer(Proc, pTimer, pTimerData, nCycleTime, bLoop, timerIndex);
}

//ȡ����ʱ��
int32_t CLightFrame::RemoveTimer(const TimerIndex timerIndex)
{
	return g_FrameTimerMgt.RemoveTimer(timerIndex);
}

//��ʼ��Ϊ�ػ�����
void CLightFrame::SetDaemon(void)
{
	pid_t pid = 0;

	if ((pid = fork()) != 0)
	{
		exit(0);
	}

	setsid();

	signal(SIGINT,  SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	struct sigaction sig;
	sig.sa_handler = SIG_IGN;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGPIPE, &sig, NULL);

	if ((pid = fork()) != 0)
	{
		exit(0);
	}

	umask(0);
}

void CLightFrame::OnTestRequest()
{

}

FRAME_NAMESPACE_END

