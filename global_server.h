#ifndef __GLOBAL_SERVER_H__
#define __GLOBAL_SERVER_H__

#include "nskernel/server.h"
#include "base/logger.h"
#include "base/utils.h"
#include "base/configer.h"
#include "nskernel/connection.h"
#include "protocolmgr.h"
//#include "apiprocessor.h"
#include "daytrace.h"
//#include "common_struct.h"
#include <bson/bson.h>
#include <bson/util/json.h>
//#include "idlproxyapi.h"
//#include "svrapi.h"
#include <map>
#include <vector>
#include "proto_base.h"
#include "rank_mgr.h"
#include "proto_20000_rank.h"
#include "zonesitmgr.h"
#include "proto_29001_enter_zone_sit.h"
#include "userservercache.h"
#include "errorcode.h"

class IDLHandller;
class GlobalPlayer;

class GlobalServer : public Server
{
public:
	GlobalServer();
	virtual ~GlobalServer();

public:
	virtual void init(const char *conf_file) throw(runtime_error);
	virtual void dataReceived(Connection *pConn, const char *pData, unsigned int nLen);
	virtual void connectionMade(Connection *pConn);
	virtual void connectionLost(Connection *pConn);
	virtual void timeoutHandller(time_t now);

private:
	void init_daylog();
	
	void initRankList();

	void procSvrRegister(Connection *pConn);

	void init_admin();

	void getRankList(int type, std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2);

	void initZoneSitMgr();

	void procRank(Connection *pConn, const char *pData, unsigned int nLen);

	void procEnterZoneSit(Connection *pConn, const char *pData, unsigned int nLen);

	void procGetZoneSitInfo(Connection *pConn, const char *pData, unsigned int nLen);

	void procZoneState(Connection *pConn, const char *pData, unsigned int nLen);

	void sendClientCmdResp(Connection *pConn, CProtoBase *pReq);

public:
	template<class T>
	void sendSvrMsg(int svrType, int cmd, const T &req)
	{
		try
		{
			if(m_pConn == NULL)
			{
				return;
			}

			ProtocolMgr packet;
			packet.m_header.stx = 0x22;
			packet.m_header.ext = -1;
			packet.m_header.ext2 = svrType;
			packet.m_header.cmd = cmd;
			packet.m_pEncoder->pack(req);

			char buf[10240] = {0};
			int len = sizeof(buf);
			packet.encode(buf, len);
			m_pConn->sendMessage(buf, len);
		}
		catch(...)
		{
			m_pRollLog->error("GlobalServer::sendSvrMsg [%s:%d] unkown error", __FILE__, __LINE__);
		}
	}

	void recordRankLog(const int &nUid, const int &nRankType, const int &nRanklist, const int &nIntegral);

public:
	void addTestData();

	void addRankData(const msgpack::object &obj);

	void reload();

	void updateZoneSitInfo(const msgpack::object &obj);
	void wxPublicBindInfo(const msgpack::object &obj);
	void playerUpLoadExtensionCode(const msgpack::object &obj);
	
private:
	DayTrace		*m_pDayLog[MAXLOGFILE];
	RankMgr			m_rankMgr;
	ZoneSitMgr		m_zoneSitMgr;
	int             m_nSvrId;
	string          m_strSvrId;
	time_t m_timer;

public:
	DBMgr* m_pDbMgr;
	//玩家id 服务id 映射，默认服务id等信息
	UserServerCache* m_pUserSvrCache;

	base::Logger	*m_pRollLog;
	base::FileConfig		m_confMgr;
	Connection *m_pConn;

	IDLHandller* m_pIdlHandller;
	GlobalPlayer* m_pGlobalPlayer;
};

#endif
