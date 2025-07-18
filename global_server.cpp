#include "global_server.h"
#include "admin_commands.h"
#include "protocolmgr.h"
#include "common_func.h"
#include "config_mgr.h"
#include "proto_base.h"
#include "idlhandller.h"
#include "apiprocessor.h"
#include "global_player.h"
#include "common_utils.h"

GlobalServer::GlobalServer()
{
	m_pRollLog = NULL;
	m_timer = 0;
	m_pConn = NULL;
	memset(m_pDayLog, '\0', sizeof(DayTrace *) * MAXLOGFILE);
}

GlobalServer::~GlobalServer()
{
	for(int i = 0; i < MAXLOGFILE; ++i)
	{
		if(m_pDayLog[i] != NULL)
		{
			delete m_pDayLog[i];
		}
	}

	if(m_pRollLog != NULL)
	{
		delete m_pRollLog;
	}
}

void GlobalServer::init(const char *conf_file) throw(runtime_error)
{
	try
	{
		Server::init(conf_file);

		m_confMgr.Init(conf_file);

		std::string logpath = m_confMgr["globalserver\\RollLog\\Name"];
		uint32_t logsize = s2u(m_confMgr["globalserver\\RollLog\\Size"]);
		uint32_t lognum = s2u(m_confMgr["globalserver\\RollLog\\Num"]);
		uint32_t loglevel = s2u(m_confMgr["globalserver\\RollLog\\Level"]);

		std::string pidpath = m_confMgr["globalserver\\pidLog\\Name"];

		m_pRollLog = new base::Logger(logpath.c_str(), logsize, lognum, loglevel, true, false);

		m_strSvrId = m_confMgr["globalserver\\svrInfo\\SVRID"];
		m_nSvrId = base::s2i(m_strSvrId);

		init_daylog();

		m_pDbMgr = new DBMgr;
		m_pDbMgr->init_db_ar(m_confMgr,m_pRollLog);

		initRankList();

		initZoneSitMgr();

		init_admin();

		m_pUserSvrCache = new UserServerCache();
		m_pUserSvrCache->init(m_pDbMgr, m_pRollLog);

		m_pIdlHandller = new IDLHandller(this);
		m_pIdlHandller->start();

		m_pGlobalPlayer = new GlobalPlayer();
		m_pGlobalPlayer->init(this);

		redcordProcessPid(pidpath.c_str(), getpid());
		m_timer = time(NULL);
	}
	catch(conf_load_error &ex)
	{
		cout<<"GlobalServer::Init failed:"<<ex.what()<<endl;
		throw ex;
	}
	catch(conf_not_find &ex)
	{
		cout<<"GlobalServer::Init conf_not_find:"<<ex.what()<<endl;
		throw ex;
	}
	catch(...)
	{
		cout<<"GlobalServer::Init Unkown error."<<endl;
	}
}

void GlobalServer::dataReceived(Connection *pConn, const char *pData, unsigned int nLen)
{
	if(nLen < 10)
	{
		m_pRollLog->error("[%s:%d]GlobalServer::dataReceived:invalid packet, %d", __FILE__, __LINE__, pConn->fd(), nLen);
		connectionLost(pConn);
		return;
	}
	try
	{
		ProtocolMgr msg;
		msg.decode_header(pData, nLen);
		m_pRollLog->normal("GlobalServer::dataReceived: cmd:%d, uid:%d, len:%d", msg.m_header.cmd, msg.m_header.ext,nLen);
		if(msg.m_header.stx == 0x22)
		{
			msg.decode(pData, nLen, ProtocolMgr::client);
			switch(msg.m_header.cmd)
			{
				//服务器间协议处理
				case protosvr::SVR_REGIST:
				{
					procSvrRegister(pConn);
					break;
				}
				case protosvr::SVR_HEARTBEAT:
				{
					break;
				}
				case protosvr::SVR_ADDRANKDATA:
				{
					msgpack::object obj = msg.m_pUnpackBody->get();
					addRankData(obj);
					break;
				}
				case protosvr::SVR_UPDATEZONESIT:
				{
					msgpack::object obj = msg.m_pUnpackBody->get();
					updateZoneSitInfo(obj);
					break;
				}
				case protosvr::SVR_PLAYER_WX_BIND: //玩家微信公众号，绑定解绑信息
				{
					msgpack::object obj = msg.m_pUnpackBody->get();
					wxPublicBindInfo(obj);
					break;
				}
				case protosvr::SVR_EXTENSION_CODE: //玩家上报推广码信息
				{
					msgpack::object obj = msg.m_pUnpackBody->get();
					playerUpLoadExtensionCode(obj);
					break;
				}
				default:
				{
					m_pRollLog->error("[%s:%d] Unsupport cmd:%d", __FILE__, __LINE__, msg.m_header.cmd);
					return;
				}
			}
		}
		else
		{
			//客户端协议处理
			if(RANK_LIST == msg.m_header.cmd)
			{
				procRank(pConn, pData, nLen);
			}
			else if(ZONE_SIT_INFO == msg.m_header.cmd)
			{
				procGetZoneSitInfo(pConn, pData, nLen);
			}
			else if(ENTER_ZONE_SIT == msg.m_header.cmd)
			{
				procEnterZoneSit(pConn, pData, nLen);
			}
			else if(ZONE_STATE == msg.m_header.cmd)
			{
				procZoneState(pConn, pData, nLen);
			}
		}
	}
	catch(std::string &e)
	{
		m_pRollLog->error("[%s:%d] GlobalServer::init %s", __FILE__, __LINE__, e.c_str());
		return;
	}
	catch(exception &e)
	{
		m_pRollLog->error("[%s:%d] GlobalServer::init %s", __FILE__, __LINE__, e.what());
		return;
	}
	catch(...)
	{
		m_pRollLog->error("[%s:%d] GlobalServer::init unkown exception", __FILE__, __LINE__);
		return;
	}
}

void GlobalServer::connectionMade(Connection *pConn)
{

}

void GlobalServer::connectionLost(Connection *pConn)
{

}

void GlobalServer::timeoutHandller(time_t now)
{
	if(m_timer == 0 || now - m_timer < 1)
	{
		return;
	}

	m_timer = now;
	m_rankMgr.fresh(now);
}

void GlobalServer::init_daylog()
{
    m_pDayLog[NEW_PLAYER_RECORD] = new DayTrace();
	m_pDayLog[NEW_PLAYER_RECORD]->setLogDir(m_confMgr["globalserver\\dayLog\\LogDir"].c_str());
	m_pDayLog[NEW_PLAYER_RECORD]->setLogName(m_confMgr["globalserver\\dayLog\\newplayerrecord\\LogName"].c_str());
	m_pDayLog[NEW_PLAYER_RECORD]->setMaxSize(s2l(m_confMgr["globalserver\\dayLog\\MaxSize"]));
	m_pDayLog[NEW_PLAYER_RECORD]->setLevel(s2u(m_confMgr["globalserver\\dayLog\\Level"]));
	m_pDayLog[NEW_PLAYER_RECORD]->setHourName();

	m_pDayLog[REG_CONN_RECORD] = new DayTrace();
	m_pDayLog[REG_CONN_RECORD]->setLogDir(m_confMgr["globalserver\\dayLog\\LogDir"].c_str());
	m_pDayLog[REG_CONN_RECORD]->setLogName(m_confMgr["globalserver\\dayLog\\pipregconnrecord\\LogName"].c_str());
	m_pDayLog[REG_CONN_RECORD]->setMaxSize(s2l(m_confMgr["globalserver\\dayLog\\MaxSize"]));
	m_pDayLog[REG_CONN_RECORD]->setLevel(s2u(m_confMgr["globalserver\\dayLog\\Level"]));
	m_pDayLog[REG_CONN_RECORD]->setHourName();

	m_pDayLog[CONNECT_TIME_RECORD] = new DayTrace();
	m_pDayLog[CONNECT_TIME_RECORD]->setLogDir(m_confMgr["globalserver\\dayLog\\LogDir"].c_str());
	m_pDayLog[CONNECT_TIME_RECORD]->setLogName(m_confMgr["globalserver\\dayLog\\connecttimerecord\\LogName"].c_str());
	m_pDayLog[CONNECT_TIME_RECORD]->setMaxSize(s2l(m_confMgr["globalserver\\dayLog\\MaxSize"]));
	m_pDayLog[CONNECT_TIME_RECORD]->setLevel(s2u(m_confMgr["globalserver\\dayLog\\Level"]));
	m_pDayLog[CONNECT_TIME_RECORD]->setHourName();

	 //排行榜日志
	 struct timeval tv;
	 gettimeofday(&tv,NULL);
	 long lCurTime = tv.tv_sec*1000 + tv.tv_usec/1000;
	 int nTimesCreateNewLog = 300; //5分钟一个新文件

	 m_pDayLog[OP_LOG_RANK] = new DayTrace();
	 m_pDayLog[OP_LOG_RANK]->setLogDir(m_confMgr["globalserver\\operateLog\\LogDir"].c_str());
	 m_pDayLog[OP_LOG_RANK]->setSvrId(m_strSvrId);
	 m_pDayLog[OP_LOG_RANK]->setLogTime(nTimesCreateNewLog);
	 m_pDayLog[OP_LOG_RANK]->setLogName(m_confMgr["globalserver\\operateLog\\RankLog\\LogName"].c_str(), lCurTime);
	 m_pDayLog[OP_LOG_RANK]->setMaxSize(s2l(m_confMgr["globalserver\\operateLog\\MaxSize"]));
	 m_pDayLog[OP_LOG_RANK]->setLevel(s2u(m_confMgr["globalserver\\operateLog\\Level"]));
}

void GlobalServer::recordRankLog(const int &nUid, const int &nRankType, const int &nRanking, const int &nIntegral)
{
	try
	{
		struct timeval tv;
		gettimeofday(&tv,NULL);
		long lCurTime = tv.tv_sec*1000 + tv.tv_usec/1000;
		char szLogHeader[512] = {0};
		int nGameId = 2002;
		if (nRankType == 29)
		{
			nGameId = 1068;
		}
		else if (nRankType == 30)
		{
			nGameId = 1073;
		}
		sprintf(szLogHeader, "%s|1.0|1001|ranking_list_log|%ld|0|0||%d||%d||%d",base::t2s(time(NULL)).c_str(), lCurTime, nGameId, nUid, nUid);

		string strRewards = "{";
		const std::map<int, CsvConfig::RankingList> rankingListMap = CsvConfigMgr::getInstance().getRankingListMap();
		for(std::map<int, CsvConfig::RankingList>::const_iterator it = rankingListMap.begin(); it != rankingListMap.end(); it++)
		{
			if(it->second.get_type() == nRankType)
			{
				string strRanking = it->second.get_ranking();
				map<int, int> mapRankRange;
				map<int, int> mapTmp; //填充参数
				int nDayTmp = 0; //填充
				common_utils::decodeResString(strRanking, mapRankRange, mapTmp, nDayTmp);
				if(nRanking >= mapRankRange.begin()->first && nRanking <= mapRankRange.begin()->second)
				{
					string strReward = it->second.get_reward();
					map<int, int> mapRewardItemid2num;	
					common_utils::decodeResString(strReward, mapRewardItemid2num, mapTmp, nDayTmp);
					int loop_num = 0;
					int num = mapRewardItemid2num.size();
		            for(map<int,int>::iterator iter = mapRewardItemid2num.begin(); iter != mapRewardItemid2num.end(); iter++)
		            {
		            	strRewards += "\"" + base::i2s(iter->first) + "\":\"" + base::i2s(iter->second) + "\"";
		            	loop_num += 1;
		            	if (loop_num < num)
		            	{
		            		strRewards += ","; 
		            	}
		            }
				}
			}
		}
		strRewards += "}";
		int nRoomId = 2002015;
		if (nRankType == 26)
		{
			nRoomId = 2002014;
		}
		else if (nRankType == 29)
		{
			nRoomId = 1068001;
		}
		else if (nRankType == 30)
		{
			nRoomId = 1073001;
		}

		m_pRollLog->debug("GlobalServer::recordRankLog rewards[uid:%d, rank:%d, times:%d, rewards:%s]", nUid, nRankType, nRanking, strRewards.c_str());
		m_pDayLog[OP_LOG_RANK]->trace_reduce("%s|%d|%d|%d|%d|%s", szLogHeader, nRoomId, nRankType, nRanking, nIntegral, strRewards.c_str());

	}
	catch(...)
	{
		m_pRollLog->error("[%s:%d] GlobalServer::recordRankLog failed unkown", __FILE__, __LINE__);
	}
}

void GlobalServer::initRankList()
{
	try
	{
		m_rankMgr.init(m_confMgr, m_pRollLog, this);
	}
	catch(const std::string &str)
	{
		m_pRollLog->error("[%s:%d] GlobalServer::initRankList failed:%s", __FILE__, __LINE__, str.c_str());
		return;
	}
	catch(...)
	{
		m_pRollLog->error("[%s:%d] GlobalServer::initRankList failed unkown", __FILE__, __LINE__);
		return;
	}
}

void GlobalServer::initZoneSitMgr()
{
	try
	{
		m_zoneSitMgr.init(m_confMgr, m_pRollLog, this);
	}
	catch(const std::string &str)
	{
		m_pRollLog->error("[%s:%d] GlobalServer::initZoneSitMgr failed:%s", __FILE__, __LINE__, str.c_str());
		return;
	}
	catch(...)
	{
		m_pRollLog->error("[%s:%d] GlobalServer::initZoneSitMgr failed unkown", __FILE__, __LINE__);
		return;
	}
}


void GlobalServer::procSvrRegister(Connection *pConn)
{
	try
	{
		ProtocolMgr packSent;
		packSent.m_header.stx = 0x22;
		packSent.m_header.cmd = protosvr::SVR_REGIST;

		protosvr::SvrRegistBodyRsp svrRegRsp;
		svrRegRsp.RES = 0;

		const std::string &msgList = m_confMgr["globalserver\\svrInfo\\msgList"];
		std::vector< std::string >  vecOut;
		common_utils::split_string(msgList, vecOut, ",");
		for(size_t i = 0; i < vecOut.size(); i++)
		{
			svrRegRsp.MSG_LIST.push_back(base::s2i(vecOut[i]));
		}
		packSent.m_pEncoder->pack(svrRegRsp);

		char buf[2048]={'\0'};
		int nLen = 2048;
		packSent.encode(buf,nLen);
		pConn->sendMessage(buf,nLen);
		m_pConn = pConn;
	}
	catch(...)
	{
		m_pRollLog->error("GlobalServer::procSvrRegister [%s:%d] unkown exception", __FILE__, __LINE__);
	}
}

void GlobalServer::procGetZoneSitInfo(Connection *pConn, const char *pData, unsigned int nLen)
{
	try
	{
		proto29zone::CProto29000ZoneSitInfo req;

		req.decode_c2s(pData, nLen);
		
		m_pRollLog->debug("GlobalServer::procGetZoneSitInfo enter, uid:%d, seqno:%d, zone:%d, timestamp:%d",req.ext1,req.seqno,req.m_c2s.zone,req.m_c2s.timestamp);

		proto29zone::CProto29000ZoneSitInfo s2c;
		s2c.ext1 = req.ext1;
		s2c.seqno = req.seqno;

		//校验策划表和客户端数据
		//...

		s2c.m_s2c.code = m_zoneSitMgr.getZoneSitInfo(req.m_c2s.zone, req.m_c2s.timestamp, s2c.m_s2c.sitinfo);

		s2c.m_s2c.zone = req.m_c2s.zone;
		
		char buf[10240] = {0};

		int len = sizeof(buf);

		s2c.encode_s2c(buf, len);

		pConn->sendMessage(buf, len);
	}
	catch(...)
	{
		m_pRollLog->error("GlobalServer::procGetZoneSitInfo [%s:%d] unkown exception", __FILE__, __LINE__);
	}
}

void GlobalServer::procEnterZoneSit(Connection *pConn, const char *pData, unsigned int nLen)
{
	try
	{
		proto29zone::CProto29001EnterZoneSit req;

		req.decode_c2s(pData, nLen);

		proto29zone::CProto29001EnterZoneSit s2c;
		s2c.ext1 = req.ext1;
	    s2c.seqno = req.seqno;
	    int nUid = req.ext1;

		m_pRollLog->normal("GlobalServer::procEnterZoneSit (zone:%d,uid:%d, sit:%d)", req.m_c2s.zone,req.ext1,req.m_c2s.sitid);
		
		//校验策划表和客户端数据
		int nGameSvrId = 0;
		CsvConfig::ByChooseSeat byChooseSeatConf;
		if(CsvConfigMgr::getInstance().findByChooseSeatByKey(req.m_c2s.zone, byChooseSeatConf))
		{
			nGameSvrId = byChooseSeatConf.get_game_server_id();
		}
		else
		{
			m_pRollLog->error("GlobalServer::procEnterZoneSit (zone:%d,uid:%d, error! not exist!)", req.m_c2s.zone,req.ext1);
			//返回座位错误
			s2c.m_s2c.code = ERROR_CODE::EC_ZONE_SIT_ERROR;
			sendClientCmdResp(pConn, &s2c);
			return;
		}

		//校验座位id
		if(req.m_c2s.sitid > byChooseSeatConf.get_seat_num() || req.m_c2s.sitid < 0) //sitid=0快速入座
		{
			m_pRollLog->error("GlobalServer::procEnterZoneSit (sitid:%d,uid:%d, error! out of get_seat_num:%d!)", req.m_c2s.sitid,req.ext1, byChooseSeatConf.get_seat_num());
			//返回座位错误
			s2c.m_s2c.code = ERROR_CODE::EC_ZONE_SIT_ERROR;
			sendClientCmdResp(pConn, &s2c);
			return;
		}

		//选座进入限制
		if(byChooseSeatConf.get_vip_limit() > 0)
		{
			int nVip = 0;
			int nNeedVip = byChooseSeatConf.get_vip_limit();
			QueryParam paramContb;
		    paramContb.tableName = TBL_PLAYER_CONTRIBUTION + base::i2s(nUid%10);
		    paramContb.keyFieldName = "uid";
		    paramContb.key = nUid;

		    DataVector vecRecordContb;
		    m_pDbMgr->get(paramContb, vecRecordContb);
		    if(!vecRecordContb.empty())
		    {
		        msgpack::unpacked unpack;   
		        msgpack::unpack(&unpack, vecRecordContb[0].data(), vecRecordContb[0].size());  
		        msgpack::object  obj = unpack.get();
		        stringstream ss;
		        ss << obj;
				m_pRollLog->debug("GlobalServer::procEnterZoneSit user vip(ss:%s)", ss.str().c_str());

		        Json::Reader reader;
		        Json::Value value;

		        if (reader.parse(ss.str(), value))
		        {
		            nVip = value["vip"].asInt();
		        }
		    }

			if(nVip < nNeedVip)
			{
				m_pRollLog->error("GlobalServer::procEnterZoneSit error! user vip not enough!(puid:%d, nVip:%d, needvip:%d)", nUid, nVip, nNeedVip);
	            s2c.m_s2c.code = ERROR_CODE::EC_CHOOSE_SEAT_VIP_LIMIT;
				sendClientCmdResp(pConn, &s2c);
	            return;
			}
		}
		//机器人选房进入限制
		if (m_zoneSitMgr.isRobotZone(req.m_c2s.zone, req.m_c2s.sitid))
		{	
			m_pRollLog->error("GlobalServer::procEnterZoneSit error! room is robot[uid:%d, zone:%d, sit:%d]", nUid, req.m_c2s.zone, req.m_c2s.sitid);
			s2c.m_s2c.code = ERROR_CODE::EC_ZONESIT_FULL;
			sendClientCmdResp(pConn, &s2c);
			return;
		}
		
		//先组网关协议，通知网关把用户导到gamesvrid
		protosvr::SvrRegUid2GameSvrId svrRegUid2GameSvrId;
		svrRegUid2GameSvrId.UID = req.ext1;
		svrRegUid2GameSvrId.GAMESVRID = nGameSvrId;
		svrRegUid2GameSvrId.STATE = USER_STAT_ONLINE;
		ProtocolMgr packSvrRegUid2GameSvrId;
		packSvrRegUid2GameSvrId.m_header.stx = 0x22;
	    packSvrRegUid2GameSvrId.m_header.ext = req.ext1;
		packSvrRegUid2GameSvrId.m_header.cmd = protosvr::SVR_REGUID2GAMESVRID;
	    packSvrRegUid2GameSvrId.m_pEncoder->pack(svrRegUid2GameSvrId);
	    char buf[512]={'\0'};
		int nLen = 512;
		packSvrRegUid2GameSvrId.encode(buf,nLen,ProtocolMgr::server);
		pConn->sendMessage(buf,nLen);
		
		//把进入座位信息组包通过网关转发给游戏服
		protosvr::SvrUserEnterZoneSit svrUserEnterZoneSit;
		svrUserEnterZoneSit.ZONE = req.m_c2s.zone;
		svrUserEnterZoneSit.SIT = req.m_c2s.sitid;
		svrUserEnterZoneSit.ROOMTYPE = byChooseSeatConf.get_room_id();
		svrUserEnterZoneSit.SEQNO = req.seqno;
		ProtocolMgr packEnter;
		packEnter.m_header.stx = 0x22;
	    packEnter.m_header.ext = req.ext1;
		packEnter.m_header.ext2 = FISH_SERVER;
		packEnter.m_header.cmd = protosvr::SVR_USERENTERZONESIT;
	    packEnter.m_pEncoder->pack(svrUserEnterZoneSit);
	    char bufEnter[1024]={'\0'};
		int nLenEnter = 1024;
		packEnter.encode(bufEnter,nLenEnter);
		pConn->sendMessage(bufEnter,nLenEnter);

	}
	catch(...)
	{
		m_pRollLog->error("GlobalServer::procEnterZoneSit [%s:%d] unkown exception", __FILE__, __LINE__);
	}
}

void GlobalServer::procZoneState(Connection *pConn, const char *pData, unsigned int nLen)
{
	try
	{
		proto29zone::CProto29002ZoneState req;

		req.decode_c2s(pData, nLen);
		m_pRollLog->debug("GlobalServer::procZoneState enter, uid:%d, seqno:%d, roomtype:%d",req.ext1,req.seqno,req.m_c2s.roomtype);

		proto29zone::CProto29002ZoneState s2c;
		s2c.ext1 = req.ext1;
		s2c.seqno = req.seqno;

		m_zoneSitMgr.getZoneState(req.m_c2s.roomtype, s2c.m_s2c.zonelist);

		m_pRollLog->debug("GlobalServer::procZoneState roomtype:%d,zonelist.size:%d",req.m_c2s.roomtype, s2c.m_s2c.zonelist.size());
		for(int i = 0; i < s2c.m_s2c.zonelist.size(); i++)
		{
			m_pRollLog->debug("GlobalServer::procZoneState  zone:%d, state:%d",s2c.m_s2c.zonelist[i].zone, s2c.m_s2c.zonelist[i].state);
		}
		char buf[10240] = {0};

		int len = sizeof(buf);

		s2c.encode_s2c(buf, len);

		pConn->sendMessage(buf, len);

		//重新
		
	}
	catch(...)
	{
		m_pRollLog->error("GlobalServer::procZoneState [%s:%d] unkown exception", __FILE__, __LINE__);
	}
}


void GlobalServer::procRank(Connection *pConn, const char *pData, unsigned int nLen)
{
	try
	{
		proto20rank::CProto20000Rank req;

		req.decode_c2s(pData, nLen);

		std::vector<proto20rank::proto_rank_data> v;

		m_rankMgr.setRankIndex(req.m_c2s.rank_type, req.m_c2s.rand_min, req.m_c2s.rand_max);
		m_rankMgr.setUid(req.m_c2s.rank_type, req.ext1);
		m_pRollLog->debug("GlobalServer::procRank client body buffer[uid:%d, rank type:%d, min:%d, max:%d]", req.ext1, req.m_c2s.rank_type, req.m_c2s.rand_min, req.m_c2s.rand_max);
		getRankList(req.m_c2s.rank_type, req.m_s2c.list1, req.m_s2c.list2);
		req.m_s2c.top_rank = m_rankMgr.getTopRank(req.m_c2s.rank_type);
		req.m_s2c.rank_num = m_rankMgr.getRankSize(req.m_c2s.rank_type);

		m_pRollLog->debug("GlobalServer::procRank body buffer[uid:%d, rank type:%d, min:%d, max:%d, top:%d]", req.ext1, req.m_c2s.rank_type, req.m_c2s.rand_min, req.m_c2s.rand_max, req.m_s2c.top_rank);

		char buf[10240] = {0};

		int len = sizeof(buf);

		req.encode_s2c(buf, len);

		pConn->sendMessage(buf, len);
	}
	catch(...)
	{
		m_pRollLog->error("GlobalServer::procRank [%s:%d] unkown exception", __FILE__, __LINE__);
	}
}


void GlobalServer::getRankList(int type, std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	m_rankMgr.getRankList(type, v1, v2);
}

void GlobalServer::addTestData()
{
	for(int i = 0; i < 30; ++i)
	{
		RankData rd;

		rd.uid = rand() % 100;
		rd.level = rand() % 100;
		rd.vip = rand() % 100;

		char buf[32] = {0};
		snprintf(buf, sizeof(buf), "test%d", rand() % 100);
		rd.nick = buf;
		rd.facelook = "999";
		rd.sign = buf;
		rd.tt = time(NULL);

		rd.items[20070003] = rand() % 100;
		
		m_rankMgr.add(rd);
	}
	m_rankMgr.fresh(time(NULL));
}

void GlobalServer::addRankData(const msgpack::object &obj)
{
	protosvr::SvrRankDataReq req;
	obj.convert(&req);
	RankData rd;
	rd.uid = req.UID;
	rd.level = req.LEVEL;
	rd.vip = req.VIP;
	rd.nick = req.NICK;
	rd.facelook = req.FACELOOK;
	rd.sign = req.SIGN;
	rd.tt = time(NULL);

	rd.items = req.STRITEM;
	/*
	string strPrint = "";
	for (map<int, int>::iterator iter=rd.items.begin(); iter != rd.items.end();++iter)
	{
		strPrint += base::i2s(iter->first) + ":" + base::i2s(iter->second) + "|";
	}
	
	m_pRollLog->debug("GlobalServer::addRankData parameter[uid:%d, parameter:%s]", rd.uid, strPrint.c_str());
	*/
	std::ostringstream  os("");
    os<< obj;
	m_pRollLog->normal("GlobalServer::addRankData uid:%d, parameter:%s", rd.uid, os.str().c_str());
	m_rankMgr.add(rd);
}

void GlobalServer::init_admin()
{
	ServerAdmin &as = getServerAdmin();
	AdminCmdInfo info;
	info.func_para = this;
	info.desc = "usage:test_admin para1 para2";
	info.func = admin::admin_test;
	as.addCommand("test_admin", info);

	AdminCmdInfo infoReload;
	infoReload.func_para = this;
	infoReload.desc = "usage:reload";
	infoReload.func = admin::reload_conf;
	as.addCommand("reload", infoReload);
}

void GlobalServer::reload()
{
	m_confMgr.Load();
	m_strSvrId = m_confMgr["globalserver\\svrInfo\\SVRID"];
	m_nSvrId = base::s2i(m_strSvrId);
	CsvConfigMgr::getInstance().reload();
	m_rankMgr.reload();
}

void GlobalServer::sendClientCmdResp(Connection *pConn, CProtoBase *pReq)
{
	if(pConn != NULL)
	{
		char buf[4096] = {0};
		int len = sizeof(buf);
		pReq->encode_s2c(buf, len);
		pConn->sendMessage(buf, len);
	}
}

void GlobalServer::updateZoneSitInfo(const msgpack::object &obj)
{
	protosvr::SvrUpdateZoneSit req;
	obj.convert(&req);
	//校验数据正确性
	//..
	
	m_zoneSitMgr.updateZoneSitInfo(req.ZONE, req.SIT, req.NUM);
}


void GlobalServer::wxPublicBindInfo(const msgpack::object &obj)
{
	try
	{
		protosvr::SvrPlayerWxBind wxBind;
		obj.convert(&wxBind);
		m_pRollLog->debug("GlobalServer::wxPublicBindInfo uid:%d,openid:%s,flag:%d", wxBind.UID, wxBind.OPENID.c_str(), wxBind.FLAG);

		if(wxBind.FLAG == WXOPENIDSTAT_BIND)
		{
			m_pGlobalPlayer->bindUid2Openid(wxBind.UID, wxBind.OPENID);
		}
		else
		{
			m_pGlobalPlayer->unBindUid2Openid(wxBind.UID, wxBind.OPENID);
		}
	}
	catch(...)
	{
		m_pRollLog->debug("GlobalServer::wxPublicBindInfo unknow exception...");
	}
}

void GlobalServer::playerUpLoadExtensionCode(const msgpack::object &obj)
{
	try
	{
		protosvr::SvrExtensionCode exCode;
		obj.convert(&exCode);
		m_pRollLog->debug("GlobalServer::playerUpLoadExtensionCode uid:%d,code:%ld", exCode.UID, exCode.EXTENSION_CODE);

		if(exCode.UID <= 0 || exCode.EXTENSION_CODE <= 0)
		{
			m_pRollLog->error("GlobalServer::playerUpLoadExtensionCode uid:%d,code:%ld, param error!", exCode.UID, exCode.EXTENSION_CODE);
			return;
		}

		m_pGlobalPlayer->uploadExtensionCode(exCode.UID, exCode.EXTENSION_CODE);

	}
	catch(...)
	{
		m_pRollLog->debug("GlobalServer::playerUpLoadExtensionCode unknow exception...");
	}
}