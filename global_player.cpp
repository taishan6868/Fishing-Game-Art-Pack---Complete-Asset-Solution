#include "global_player.h"
#include "timefuncdef.h"
#include "config_mgr.h"
#include "proto_svr.h"
#include "global_server.h"
#include "common_def.h"
#include <sys/time.h>
#include <algorithm>

GlobalPlayer::GlobalPlayer()
{}

GlobalPlayer::~GlobalPlayer()
{
	
}

void GlobalPlayer::init(GlobalServer *pServer)
{
	try
	{
		m_pRollLog = pServer->m_pRollLog;

		m_pDbMgr = pServer->m_pDbMgr;

		m_pServer = pServer;

		loadNick();

		loadWxInfo();

		loadExtensionCode();
		
	}
	catch(std::runtime_error &e)
	{
		m_pRollLog->error("[%s:%d] %s", __FILE__, __LINE__, e.what());
	}
	catch(...)
	{
		m_pRollLog->error("[%s:%d] unkown exception", __FILE__, __LINE__);
	}
}

void GlobalPlayer::loadNick()
{
	string strTbl = TBL_PLAYER;
	string strSql = "";
	for (int i = 0; i < 10; i++)
	{
		string strDbTbl = strTbl + base::i2s(i);
		strSql = "select uid, account_id, nick, phone from " + strDbTbl + " ;";
		base::MySqlData data = m_pDbMgr->query_sql(strSql.c_str());
		int rowNum = data.num_rows();
		for (int j = 0; j < rowNum; j++)
		{
			int uid = base::s2i(data[j]["uid"]);
			string puid = data[j]["account_id"];
			string name = data[j]["nick"];
			string phone = data[j]["phone"];

			m_setNick.insert(name);
		} 
	}
}

void GlobalPlayer::loadWxInfo()
{
	map<string, map<int, int> > mapOpenId2Time2Uid; //openid对应绑定时间和玩家id，用map升序来做排序
	string strTbl = TBL_PLAYER_WX;
	string strSql = "";
	for (int i = 0; i < 10; i++)
	{
		string strDbTbl = strTbl + base::i2s(i);
		strSql = "select uid, openid, bind_time, stat, bind_cnt, history_openid from " + strDbTbl + " ;";
		base::MySqlData data = m_pDbMgr->query_sql(strSql.c_str());
		int rowNum = data.num_rows();
		for (int j = 0; j < rowNum; j++)
		{
			int uid = base::s2i(data[j]["uid"]);
			string openid = data[j]["openid"];
			int bindTime = base::s2i(data[j]["bind_time"]);
			int state = base::s2i(data[j]["stat"]);
			int bindCnt = base::s2i(data[j]["bind_cnt"]);
			string history_openid = data[j]["history_openid"];

			//绑定状态，就放入内存
			if(openid != "" && state == WXOPENIDSTAT_BIND)
			{
				if(mapOpenId2Time2Uid.find(openid) == mapOpenId2Time2Uid.end())
				{
					std::map<int, int> mapTime2Uid;
					mapTime2Uid[bindTime] = uid;
					mapOpenId2Time2Uid[openid] = mapTime2Uid;
				}
				else
				{
					mapOpenId2Time2Uid[openid][bindTime] = uid;
				}
			}

			if(history_openid != "")
			{
				std::vector<string> vec;
				common_utils::split_string(history_openid, vec, ",");
				for(int i = 0; i < vec.size(); i++)
				{
					m_setOpenid.insert(vec[i]);
				}
			}

			//上线时history_openid无数据，从openid取
			if(openid != "")
			{
				m_setOpenid.insert(openid);
			}
		} 
	}

	//map已经是升序排列，直接顺序读取进list即可
	for(map<string, map<int, int> >::iterator it = mapOpenId2Time2Uid.begin(); it != mapOpenId2Time2Uid.end(); it++)
	{
		if(m_mapOpenid2UidList.find(it->first) == m_mapOpenid2UidList.end())
		{
			std::list<int> uidList;
			m_mapOpenid2UidList[it->first] = uidList;
		}

		for(map<int, int>::iterator iter = it->second.begin(); iter != it->second.end(); iter++)
		{
			m_mapOpenid2UidList[it->first].push_back(iter->second);
		}
	}
}

void GlobalPlayer::loadExtensionCode()
{
	string strTbl = TBL_PLAYER_EXTENSION;
	string strSql = "";
	for (int i = 0; i < 10; i++)
	{
		string strDbTbl = strTbl + base::i2s(i);
		strSql = "select uid, extension_code from " + strDbTbl + " ;";
		base::MySqlData data = m_pDbMgr->query_sql(strSql.c_str());
		int rowNum = data.num_rows();
		for (int j = 0; j < rowNum; j++)
		{
			int uid = base::s2i(data[j]["uid"]);
			long extensionCode = base::s2l(data[j]["extension_code"]);

			m_mapExtensionCode2Uid[extensionCode] = uid;
		} 
	}
}


void GlobalPlayer::updatePlayerNick(const char* data,unsigned int nLength)
{
	ProtocolMgr pack;
	pack.decode(data, nLength, ProtocolMgr::client);
    msgpack::object obj = pack.m_pUnpackBody->get();
    std::ostringstream  os("");
    os<< obj;
    m_pRollLog->debug("GlobalPlayer::updatePlayerNick body:%s", os.str().c_str());
			
	protosvr::SvrPlayerNick svrReq;
	obj.convert(&svrReq);

	base::Guard guard(m_lockNick) ;
	if(svrReq.OLD_NICK != "")
	{
		m_setNick.erase(svrReq.OLD_NICK);
	}
	if(svrReq.NICK != "")
	{
		m_setNick.insert(svrReq.NICK);
	}
	
}

bool GlobalPlayer::isNickExist(string strNick)
{
	base::Guard guard(m_lockNick) ;
	if(m_setNick.find(strNick) != m_setNick.end())
	{
		return true;
	}
	return false;
}

//获取最早绑定这个openid的玩家id
int GlobalPlayer::getFirstUidBindOpenid(string strOpenid)
{
	if(m_mapOpenid2UidList.find(strOpenid) != m_mapOpenid2UidList.end())
	{
		return m_mapOpenid2UidList[strOpenid].front();
	}
	return 0;
}

//获取绑定这个openid的所有玩家id，返回字符串，用|分割
string GlobalPlayer::getUidBindOpenid(string strOpenid)
{
	string strUids = "";
	if(m_mapOpenid2UidList.find(strOpenid) != m_mapOpenid2UidList.end())
	{
		for(list<int>::iterator it = m_mapOpenid2UidList[strOpenid].begin(); it != m_mapOpenid2UidList[strOpenid].end(); it++)
		{
			if(strUids != "")
			{
				strUids += "|";
			}
			strUids += *it;
		}
	}
	return strUids;
}

//绑定玩家id到微信openid
void GlobalPlayer::bindUid2Openid(int nUid, string strOpendId)
{
	try
	{
		if(m_mapOpenid2UidList.find(strOpendId) != m_mapOpenid2UidList.end())
		{
			std::list<int>& uidList = m_mapOpenid2UidList[strOpendId];
			std::list<int>::iterator it = find(uidList.begin(), uidList.end(), nUid);
			if(it != uidList.end())
			{
				m_pRollLog->debug("GlobalPlayer::bindUid2Openid uid:%d had add...", nUid);
			}
			else
			{
				uidList.push_back(nUid);
			}
		}
		else
		{
			std::list<int> uidList;
			uidList.push_back(nUid);
			m_mapOpenid2UidList[strOpendId] = uidList;
			m_pRollLog->debug("GlobalPlayer::bindUid2Openid uid:%d new openid...", nUid);
		}

		m_setOpenid.insert(strOpendId);
		int uidListLen = m_mapOpenid2UidList[strOpendId].size();
		m_pRollLog->debug("GlobalPlayer::bindUid2Openid uid:%d,uidListLen:%d,openid:%s", nUid, uidListLen, strOpendId.c_str());
	}
	catch(...)
	{
		m_pRollLog->error("GlobalPlayer::bindUid2Openid unknown exception,uid:%d", nUid);
	}
}

int GlobalPlayer::getIdCntBindOpenid(const std::string strOpenid)
{
	m_pRollLog->debug("GlobalPlayer::getIdCntBindOpenid strOpenid:%s", strOpenid.c_str());
	if(m_mapOpenid2UidList.find(strOpenid) != m_mapOpenid2UidList.end())
	{
		return m_mapOpenid2UidList[strOpenid].size();
	}
	return 0;
}

void GlobalPlayer::unBindUid2Openid(int nUid, string strOpendId)
{
	try
	{
		if(m_mapOpenid2UidList.find(strOpendId) == m_mapOpenid2UidList.end())
		{
			m_pRollLog->debug("GlobalPlayer::unBindUid2Openid no openid:%s", strOpendId.c_str());
			return;
		}
		std::list<int>& uidList = m_mapOpenid2UidList[strOpendId];
		int uidListLen = uidList.size();
		m_pRollLog->debug("GlobalPlayer::unBindUid2Openid begin openid:%s,uidListLen:%d", strOpendId.c_str(), uidListLen);

		//STL容器删除元素的陷阱
		//https://www.cnblogs.com/Yogurshine/p/4030728.html
		std::list<int>::iterator iter = uidList.begin();
		for( ; iter != uidList.end();)
		{
			if(*iter == nUid)
			{
				uidList.erase(iter++);
			}
			else
			{
				++iter;
			}
		}

		uidListLen = uidList.size();
		m_pRollLog->debug("GlobalPlayer::unBindUid2Openid end openid:%s,uidListLen:%d", strOpendId.c_str(), uidListLen);
	}
	catch(...)
	{
		m_pRollLog->error("GlobalPlayer::unBindUid2Openid unknow exception");
	}
}

//判断openid是否曾被绑定过
bool GlobalPlayer::isOpenidBound(string strOpendId)
{
	if(m_setOpenid.find(strOpendId) != m_setOpenid.end())
	{
		return true;
	}
	return false;
}


//上报推广码玩家id
void GlobalPlayer::uploadExtensionCode(int nUid, long lnExtensionCode)
{
	try
	{
		m_mapExtensionCode2Uid[lnExtensionCode] = nUid;
		m_pRollLog->debug("GlobalPlayer::uploadExtensionCode uid:%d,Len:%d,lnExtensionCode:%ld", nUid, m_mapExtensionCode2Uid.size(), lnExtensionCode);
	}
	catch(...)
	{
		m_pRollLog->error("GlobalPlayer::uploadExtensionCode unknown exception,uid:%d", nUid);
	}
}

int GlobalPlayer::getUidByExtensionCode(const long lnExCode)
{
	m_pRollLog->debug("GlobalPlayer::getUidByExtensionCode lnExCode:%ld", lnExCode);
	if(m_mapExtensionCode2Uid.find(lnExCode) != m_mapExtensionCode2Uid.end())
	{
		return m_mapExtensionCode2Uid[lnExCode];
	}
	return 0;
}