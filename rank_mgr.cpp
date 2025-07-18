#include "rank_mgr.h"
#include "timefuncdef.h"
#include "config_mgr.h"
#include "proto_svr.h"
#include "global_server.h"
#include "common_def.h"
#include <sys/time.h>

#define DEFAULT_RANK_SIZE 20


#define JB_ID 10010001		// 金币ID
#define MJ_ID 20070003		// 魔晶ID
#define BJDT_ID 20010004	// 铂金弹头
#define HJDT_ID 20010003	// 黄金弹头
#define BYDT_ID 20010002	// 白银弹头
#define QTDT_ID 20010001	 // 青铜弹头
#define HSJF_ID 10010009   // 花色积分
#define BOTTLE_ID 20090001 // 酒瓶积分

#define STONE_ID 20070001 	// 强化石
#define ESSENCE_ID 20070002 // 原石精华

#define LUCKY_VALUE_ID 10010007    // 幸运值
#define DTJF_ID 10010010    		// 弹头积分
#define DAY_RECHARGE_ID 10010013    // 活动日充值积分

#define PET_INTEGRAL_ID 40020001    //宠物积分

#define MATCH_INTEGRAL_ID 10010008  //比赛积分
#define GS_INTEGRAL_ID 10010009 //猜大小积分ID
#define EGG_INTEGRAL_ID 10010020 //扭蛋活动积分
#define DRAGON_JACKPOT_INTEGRAL_ID 10010021 //金龙奖池积分


#define REAL_RANK 0x01
#define SHOW_RANK 0x10


#define SECONDS_OF_HOUR 3600

#define SECONDS_OF_DAY (24 * SECONDS_OF_HOUR)

#define SECONDS_OF_WEEK (7 * SECONDS_OF_DAY)

#define RANK_FRESH_INTERVAL SECONDS_OF_HOUR

static int DT[] = {BJDT_ID, HJDT_ID, BYDT_ID, QTDT_ID};

static int CL[] = {STONE_ID, ESSENCE_ID};

#define DT_NUM (sizeof(DT) / sizeof(DT[0]))
#define CL_NUM (sizeof(CL) / sizeof(CL[0]))


static bool _Rank_JB_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_DJ_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_DT_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_MJ_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_JF_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_LuckyValue_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_DTJF_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_CL_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_BI_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_Day_Recharge_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_Pet_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_MATCH_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_GS_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_EGG_Compare_Func(const RankData &r1, const RankData &r2);
static bool _Rank_DragonJackpot_Compare_Func(const RankData &r1, const RankData &r2);



struct RankInfo
{
	int type;
	CmpFunc cmp;
};

struct RankInfo ranks[] = 
{
	{RANK_TYPE_JB, _Rank_JB_Compare_Func},
	{RANK_TYPE_DJ, _Rank_DJ_Compare_Func},
	{RANK_TYPE_DT, _Rank_DT_Compare_Func},
	{RANK_TYPE_MJ, _Rank_MJ_Compare_Func},
	{RANK_TYPE_JFD, _Rank_JF_Compare_Func},
	{RANK_TYPE_JFW, _Rank_JF_Compare_Func},
	{RANK_TYPE_ZQJF, _Rank_LuckyValue_Compare_Func},
	{RANK_TYPE_GQJF, _Rank_LuckyValue_Compare_Func},
	{RANK_TYPE_MATERIAL, _Rank_CL_Compare_Func},
	{RANK_TYPE_DTJF, _Rank_DTJF_Compare_Func},
	{RANK_TYPE_DAY_BOTTLE, _Rank_BI_Compare_Func},
	{RANK_TYPE_WEEK_BOTTLE, _Rank_BI_Compare_Func},
	{RANK_TYPE_DAY_RECHARGE, _Rank_Day_Recharge_Compare_Func},
	{RANK_TYPE_PET_DOG_DAY, _Rank_Pet_Compare_Func},
	{RANK_TYPE_PET_DOG_WEEK, _Rank_Pet_Compare_Func},
	{RANK_TYPE_PET_MONKEY_DAY, _Rank_Pet_Compare_Func},
	{RANK_TYPE_PET_MONKEY_WEEK, _Rank_Pet_Compare_Func},
	{RANK_TYPE_PET_DRAGON_DAY, _Rank_Pet_Compare_Func},
	{RANK_TYPE_PET_DRAGON_WEEK, _Rank_Pet_Compare_Func},
	{RANK_MATCH_AP_DAY, _Rank_MATCH_Compare_Func},
	{RANK_MATCH_BP_DAY, _Rank_MATCH_Compare_Func},
	{RANK_MATCH_BP_WEEK, _Rank_MATCH_Compare_Func},
	{RANK_GS_WEEK, _Rank_GS_Compare_Func},
	{RANK_EGG_ACTIVITY, _Rank_EGG_Compare_Func},
	{RANK_DRAGON_JACKPOT, _Rank_DragonJackpot_Compare_Func},
};

#define RANK_NUM (sizeof(ranks) / sizeof(ranks[0]))

Rank::Rank(int type, CmpFunc func, GlobalServer *pServer)
{
	m_type = type;
	m_insert = true;
	m_cmp = func;
	m_next_fresh = -1;
	m_pServer = pServer;
	m_nRankMin = 0;
	m_nRankMax = 0;
	m_nTopRank = 0;
	m_pRollLog = pServer->m_pRollLog;
}

Rank::~Rank()
{
	
}

int Rank::getType()
{
	return m_type;
}

void Rank::setType(int type)
{
    m_type = type;
}

void Rank::setInsertFlag(bool flag)
{
	m_insert = flag;
}

bool Rank::needInsert()
{
	return m_insert;
}

int Rank::getRankSize()
{
    CsvConfig::RankType cfg;
    if(!CsvConfigMgr::getInstance().findRankTypeByKey(m_type, cfg))
    {
        return DEFAULT_RANK_SIZE;
    }
    return cfg.get_size();
}


void Rank::setRankIndex(const int &nMin, const int &nMax)
{
	m_nRankMin = nMin;
	m_nRankMax = nMax;
}
	
int Rank::getTopRank()
{
	return m_nTopRank;
}

void Rank::setUid(const int &nUid)
{
	m_uid = nUid;
}

int Rank::getCurRankSize()
{
	return m_real.size();
}

void Rank::init()
{
	time_t now = time(NULL);
	tm tt;
	localtime_r(&now, &tt);
	tt.tm_min = 0;
	tt.tm_sec = 0;
	m_next_fresh = mktime(&tt) + RANK_FRESH_INTERVAL;
}

void Rank::serialize(int type, std::string &str)
{
	std::vector<RankData> *pList = NULL;
	if(type == REAL_RANK)
	{
		pList = &m_real;
	}
	else if(type == SHOW_RANK)
	{
		pList = &m_show;
	}
	else
	{
		return;
	}


	msgpack::sbuffer sb;
	msgpack::packer<msgpack::sbuffer> pker(&sb);

	pker.pack(pList->size());
	for(std::vector<RankData>::iterator i = pList->begin(); i != pList->end(); ++i)
	{
		pker.pack(*i);
	}
	str.assign(sb.data(), sb.size());

}

void Rank::serialize(std::string &str)
{
	msgpack::sbuffer buf;
	msgpack::packer<msgpack::sbuffer> pack_(&buf);

	pack_.pack_map(3);
	pack_.pack(std::string("type"));
	pack_.pack(m_type);

	std::string real;
	serialize(REAL_RANK, real);

	pack_.pack(std::string("real"));
	pack_.pack(real);

	std::string show;
	serialize(SHOW_RANK, show);

	pack_.pack(std::string("show"));
	pack_.pack(show);

	str.assign(buf.data(), buf.size());

}

void Rank::unserialize(int type, const std::string &data)
{
	std::vector<RankData> *pList = NULL;
	if(REAL_RANK == type)
	{
		pList = &m_real;
	}
	else if(SHOW_RANK == type)
	{
		pList = &m_show;
	}
	else
	{
		return;
	}
	msgpack::unpacker unpack;
	unpack.reserve_buffer(data.length());
	memcpy(unpack.buffer(), data.c_str(), data.length());
	unpack.buffer_consumed(data.length());
	msgpack::unpacked result_;
	unpack.next(&result_);
	int sz;
	result_.get().convert(&sz);
	for(int i = 0; i < sz; ++i)
	{
		if((i >= getRankSize()) && (SHOW_RANK != type))
		{
			break;
		}
		unpack.next(&result_);
		RankData rd;
		result_.get().convert(&rd);
		pList->push_back(rd);
		if (SHOW_RANK != type)
		{
			m_userRank[rd.uid] = i+1;
		}
	}
}

void Rank::unserialize(const std::string &real_data, const std::string &show_data)
{
	if(!real_data.empty())
	{
		unserialize(REAL_RANK, real_data);
	}
	if(!show_data.empty())
	{
		unserialize(SHOW_RANK, show_data);
	}
	m_insert = false;
}

int Rank::filter()
{
	int ret = 0;
	int nNow = TimeFuncDef::getTimeOfDayBegin(time(NULL));
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end();)
	{
		int nReg = TimeFuncDef::getTimeOfDayBegin(i->tt);

		if(nNow - nReg > SECONDS_OF_WEEK)
		{
			i = m_real.erase(i);
			ret = 1;
		}
		else
		{
			++i;
		}
	}
	return ret;
}

int Rank::add(const RankData &rd)
{
	bool del = true;
	for(std::map<int, long>::const_iterator i = rd.items.begin(); i != rd.items.end(); ++i)
	{
		if(i->second > 0)
		{
			del = false;
		}
	}
	if(del)
	{
		for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
		{
			if(i->uid == rd.uid)
			{
				m_real.erase(i);
				return 1;
			}
		}
		return 0;
	}
	
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		if(i->uid == rd.uid)
		{
			*i = rd;
			return 1;
		}
	}
	if(m_real.size() < getRankSize())
	{
		m_real.push_back(rd);
		return 1;
	}
	else
	{
		RankData &last = m_real.back();
		if(m_cmp(rd, last))
		{
			last = rd;
			return 1;
		}
	}
	return 0;
}

void Rank::sort()
{
	std::sort(m_real.begin(), m_real.end(), m_cmp);
	
	m_userRank.clear();
	int times = 0;
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); i++)
	{
		times += 1;
		m_userRank[i->uid] = times;
	}
}

int Rank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
		return -1;
	}
	//printRank(false); //刷新前打印一下榜单
	m_next_fresh += RANK_FRESH_INTERVAL;
	int nNow = TimeFuncDef::getTimeOfDayBegin(time(NULL));
	m_show.clear();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); )
	{
		int nReg = TimeFuncDef::getTimeOfDayBegin(i->tt);
		if(nNow - nReg > SECONDS_OF_WEEK)
		{
			i = m_real.erase(i);
		}
		else
		{
			m_show.push_back(*i);
			++i;
		}
	}
	return 0;
}

int Rank::getNextFreshTime()
{
	return m_next_fresh;
}

void Rank::printRank(bool bOnlyReal)
{
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); i++)
	{
		std::ostringstream os("");
		for(std::map<int, long>::iterator iter = i->items.begin();iter != i->items.end();++iter)
		{
		 	os<<iter->first<<":"<<iter->second<<",";
		}
		m_pRollLog->normal("Rank::printRank:real: type:%d, uid:%d, item:%s", m_type, i->uid, os.str().c_str());
	}

	if(bOnlyReal)
	{
		return;
	}
	m_pRollLog->normal("Rank::printRank: ==================================================================");
	for(std::vector<RankData>::iterator i = m_show.begin(); i != m_show.end(); i++)
	{
		std::ostringstream os("");
		for(std::map<int, long>::iterator iter = i->items.begin();iter != i->items.end();++iter)
		{
		 	os<<iter->first<<":"<<iter->second<<",";
		}
		m_pRollLog->normal("Rank::printRank:show: type:%d, uid:%d, item:%s", m_type, i->uid, os.str().c_str());
	}
}

void Rank::initFromDB()
{
	m_pRollLog->debug("Rank::initFromDB enter");
	//读表初始化
	m_real.clear();
	m_show.clear();
	
	int nRankSize = getRankSize();
	if (m_type == RANK_MATCH_BP_WEEK)
	{
		for(int i = 1; i <= nRankSize; i++) //根据表单大小查库，如果没有，则real_rank先insert一条空数据先
		{
			readFromDb(REAL_RANK, i);
		}
		nRankSize = 7;                      //大奖赛的周榜，显示(历史)表存的是七期历史的数据
		for (int i = 1; i <= nRankSize; ++i)
		{
			readFromDb(SHOW_RANK, i);
		}
	}
	else
	{
		for(int i = 1; i <= nRankSize; i++) //根据表单大小查库，如果没有，则real_rank先insert一条空数据先
		{
			
			readFromDb(REAL_RANK, i);

			//m_show显示榜也要读取，海魔的显示榜是用来做上期排行的
			readFromDb(SHOW_RANK, i);
		}		
	}
}

void Rank::readFromDb(int nType, int nSortNo)
{
	string strDbTableNm = nType == SHOW_RANK ? TBL_RANK_SHOW : TBL_RANK_REAL;
	
	QueryParam paramRank;
    paramRank.tableName = strDbTableNm;
    paramRank.keyFieldName = "rank_type";
	paramRank.key = m_type;
	paramRank.userIdFiledName = "sort_no";
	paramRank.userid = nSortNo; //排行名次

    DataVector vecRecordContb;
    m_pServer->m_pDbMgr->get(paramRank, vecRecordContb);
    if(!vecRecordContb.empty())
    {
        msgpack::unpacked unpack;   
        msgpack::unpack(&unpack, vecRecordContb[0].data(), vecRecordContb[0].size());  
        msgpack::object  obj = unpack.get();
        stringstream ss;
        ss << obj;
        m_pRollLog->normal("Rank::readFromDb strDbTableNm:%s,  (ss:%s)", strDbTableNm.c_str(), ss.str().c_str());

        Json::Reader reader;
        Json::Value value;

        if (reader.parse(ss.str(), value))
        {
            //读取数据初始化
            if(value["uid"].asInt() == 0) //数据被清零了，新数据还没排出来
        	{
        		m_pRollLog->error("Rank::readFromDb strDbTableNm:%s, (ss:%s) uid == 0!", strDbTableNm.c_str(), ss.str().c_str());
	            return;
        	}
			
            Json::Value jsonInfo;
			string strJsoninfo = value["json_info"].asString();
			if(!reader.parse(strJsoninfo, jsonInfo))
	        {
	        	m_pRollLog->error("Rank::readFromDb strDbTableNm:%s, (ss:%s) parse json_info error!", strDbTableNm.c_str(), ss.str().c_str());
	            return;
	        }
	        
            RankData rd;
            rd.uid = jsonInfo["uid"].asInt();
            rd.level = jsonInfo["level"].asInt();
            rd.vip = jsonInfo["vip"].asInt();
            rd.nick = jsonInfo["nick"].asString();
            rd.facelook = jsonInfo["facelook"].asString();
            rd.sign = jsonInfo["sign"].asString();
            rd.tt = jsonInfo["tt"].asInt64();
            if (jsonInfo.isMember("start") && jsonInfo["start"].asInt())
            {
        		rd.start_time = (jsonInfo["start"].asInt() > 0) ? jsonInfo["start"].asInt() : 0;
            }
            else
            {
            	rd.start_time = 0;
            }
	        if (jsonInfo.isMember("end") && jsonInfo["end"].asInt())
            {
        		rd.end_time = (jsonInfo["end"].asInt() > 0) ? jsonInfo["end"].asInt() : 0;
            }
            else
            {
            	rd.end_time = 0;
            }
			
        	for (unsigned int i = 0; i < jsonInfo["item"].size(); i++)
			{
				int nItemid = jsonInfo["item"][i]["id"].asInt();
				long nItemNum = jsonInfo["item"][i]["num"].asInt64();
                rd.items.insert(std::make_pair(nItemid, nItemNum));
			}

			if(nType == REAL_RANK)
			{
				m_real.push_back(rd);

				m_userRank[rd.uid] = nSortNo;
			}
			else
			{
				m_show.push_back(rd);
			}
        
        }
    }
    else
    {
        //insert一条空数据入库，以后只update就可
		msgpack::sbuffer sbuf;
		msgpack::packer<msgpack::sbuffer> pker(&sbuf);
		pker.pack_map(5);
		pker.pack(std::string("rank_type"));
		pker.pack(m_type);
	    pker.pack(std::string("sort_no"));
		pker.pack(nSortNo);
	    pker.pack(std::string("uid"));
		pker.pack(0);
	    pker.pack(std::string("json_info"));
		pker.pack("");
		pker.pack(std::string("updated"));
		pker.pack(0);
		m_pServer->m_pDbMgr->set(paramRank, string(sbuf.data(), sbuf.size()), true); //insert
    }
}

//发榜后清除数据库的榜单数据
void Rank::clearRankDB()
{
	m_pRollLog->debug("Rank::clearRankDB enter");
	int nRankSize = getRankSize();
	for(int i = 1; i <= nRankSize; i++) 
	{
		QueryParam paramRank;
	    paramRank.tableName = TBL_RANK_REAL;
	    paramRank.keyFieldName = "rank_type";
		paramRank.key = m_type;
		paramRank.userIdFiledName = "sort_no";
		paramRank.userid = i; //排行名次

		msgpack::sbuffer sbuf;
		msgpack::packer<msgpack::sbuffer> pker(&sbuf);
		pker.pack_map(5);
		pker.pack(std::string("rank_type"));
		pker.pack(m_type);
	    pker.pack(std::string("sort_no"));
		pker.pack(i);
	    pker.pack(std::string("uid"));
		pker.pack(0);
	    pker.pack(std::string("json_info"));
		pker.pack("");
		pker.pack(std::string("updated"));
		pker.pack(0);
		m_pServer->m_pDbMgr->set(paramRank, string(sbuf.data(), sbuf.size()), false);

		//显示榜也清除
		paramRank.tableName = TBL_RANK_SHOW;
		m_pServer->m_pDbMgr->set(paramRank, string(sbuf.data(), sbuf.size()), false);
	}
	//m_pRollLog->debug("Rank::clearRankDB end");
}


//20190529改成写mysql，每个排行榜自己写
void Rank::saveRank()
{
	//m_pRollLog->debug("Rank::saveRank enter");
	saveRank(SHOW_RANK);
	saveRank(REAL_RANK);
}

void Rank::saveRank(int nType)
{
	int nTimeNow = time(NULL);
	std::vector<RankData> *pList = &m_real;
	if(nType == SHOW_RANK)
	{
		pList = &m_show;
	}
	
    Json::Value root;
	Json::FastWriter writer;
	string str = "";
	QueryParam qp;
	qp.tableName = nType == SHOW_RANK ? TBL_RANK_SHOW : TBL_RANK_REAL;

	int nSort = 0;
    for(std::vector<RankData>::iterator i = pList->begin(); i != pList->end(); ++i)
    {
    	nSort++;
        Json::Value data;
        data["uid"] = i->uid;
        data["level"] = i->level;
        data["vip"] = i->vip;
        data["nick"] = i->nick;
        data["facelook"] = i->facelook;
        data["sign"] = i->sign;
        data["tt"] = i->tt;
        data["start"] = i->start_time;
        data["end"] = i->end_time;
        for(std::map<int, long>::iterator j = i->items.begin(); j != i->items.end(); ++j)
        {
            Json::Value item;
            item["id"] = j->first;
            item["num"] = j->second;
            data["item"].append(item);
        }
       	str = writer.write(data);

		qp.keyFieldName = "rank_type";
		qp.key = m_type;
		qp.userIdFiledName = "sort_no";
		qp.userid = nSort;

		msgpack::sbuffer sbuf;
		msgpack::packer<msgpack::sbuffer> pker(&sbuf);
		pker.pack_map(5);
		pker.pack(std::string("rank_type"));
		pker.pack(m_type);
	    pker.pack(std::string("sort_no"));
		pker.pack(nSort);
	    pker.pack(std::string("uid"));
		pker.pack(i->uid);
	    pker.pack(std::string("json_info"));
		pker.pack(str);
		pker.pack(std::string("updated"));
		pker.pack(nTimeNow);
		m_pServer->m_pDbMgr->set(qp, string(sbuf.data(), sbuf.size()), false); //update
    }

	//防止排行榜减缩了人数，没处理脏数据，导致有玩家重复上榜，所以这里要把redis和mysql没用的数据清掉
	int nRankSize = getRankSize();
	while(nSort < nRankSize)
	{
		nSort++;
		qp.keyFieldName = "rank_type";
		qp.key = m_type;
		qp.userIdFiledName = "sort_no";
		qp.userid = nSort;

		msgpack::sbuffer sbuf;
		msgpack::packer<msgpack::sbuffer> pker(&sbuf);
		pker.pack_map(5);
		pker.pack(std::string("rank_type"));
		pker.pack(m_type);
	    pker.pack(std::string("sort_no"));
		pker.pack(nSort);
	    pker.pack(std::string("uid"));
		pker.pack(0);
	    pker.pack(std::string("json_info"));
		pker.pack("");
		pker.pack(std::string("updated"));
		pker.pack(nTimeNow);
		m_pServer->m_pDbMgr->set(qp, string(sbuf.data(), sbuf.size()), false);//update
	}
}

//20190529改成写mysql，每个排行榜自己写
//发榜的时候把榜单写到历史表，可供查询
void Rank::saveRankHistory()
{
	//m_pRollLog->debug("Rank::saveRankHistory enter");
	std::vector<RankData> *pList = &m_real; //只按照实时榜

	time_t now = time(NULL);
	struct tm tt;
    localtime_r(&now, &tt);
    char date[64] = {0};
    strftime(date, 64, "%Y%m%d%H%M%S", &tt);
	
	string strSort = date;
	
    Json::Value root;
	Json::FastWriter writer;
	string str = "";
	QueryParam qp;
	qp.tableName = TBL_RANK_HISTORY;

	m_pRollLog->debug("Rank::saveRankHistory strSort:%s, size:%d", strSort.c_str(), pList->size());

	int nSort = 0;
	long lSortNo = 0;
    for(std::vector<RankData>::iterator i = pList->begin(); i != pList->end(); ++i)
    {
    	nSort++;
		lSortNo = base::s2l(strSort + base::i2s(nSort)); //排行名次: 发榜年月日时分秒+名次，如: 201905291011231
        Json::Value data;
        data["uid"] = i->uid;
        data["level"] = i->level;
        data["vip"] = i->vip;
        data["nick"] = i->nick;
        data["facelook"] = i->facelook;
        data["sign"] = i->sign;
        data["tt"] = i->tt;
        data["start"] = i->start_time;
        data["end"] = i->end_time;
        for(std::map<int, long>::iterator j = i->items.begin(); j != i->items.end(); ++j)
        {
            Json::Value item;
            item["id"] = j->first;
            item["num"] = j->second;
            data["item"].append(item);
        }
       	str = writer.write(data);

		qp.keyFieldName = "rank_type";
		qp.key = m_type;
		qp.userIdFiledName = "sort_no";
		qp.userid = nSort;

		msgpack::sbuffer sbuf;
		msgpack::packer<msgpack::sbuffer> pker(&sbuf);
		pker.pack_map(5);
		pker.pack(std::string("rank_type"));
		pker.pack(m_type);
	    pker.pack(std::string("sort_no"));
		pker.pack(lSortNo);
	    pker.pack(std::string("uid"));
		pker.pack(i->uid);
	    pker.pack(std::string("json_info"));
		pker.pack(str);
		pker.pack(std::string("updated"));
		pker.pack((int)time(NULL));
		m_pServer->m_pDbMgr->save_db_shm(string(sbuf.data(), sbuf.size()), qp); //只写mysql，不写redis
    }
    //m_pRollLog->debug("Rank::saveRankHistory end");
}

JBRank::JBRank(CmpFunc func, GlobalServer *pServer) : Rank(RANK_TYPE_JB, func, pServer)
{
	
}

JBRank::~JBRank()
{
	
}

int JBRank::add(const RankData &rd)
{
	int ret = filter();
	std::map<int, long>::const_iterator i = rd.items.find(JB_ID);
	if(i == rd.items.end())
	{
		return ret;
	}
	int flag = Rank::add(rd);
	if(flag)
	{
		sort();
	}
	return ret | flag;
}

void JBRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_show.begin(); i != m_show.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(JB_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}

DJRank::DJRank(CmpFunc func, GlobalServer *pServer) : Rank(RANK_TYPE_DJ, func, pServer)
{
}

DJRank::~DJRank()
{
	
}

int DJRank::add(const RankData &rd)
{
	int ret = filter();
	if(rd.level <= 0)
	{
		return ret;
	}
	RankData data = rd;
	data.items.clear();
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
	return ret | flag;
}

void DJRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_show.begin(); i != m_show.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		rd.value = i->level;
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}

DTRank::DTRank(CmpFunc func, GlobalServer *pServer) : Rank(RANK_TYPE_DT, func, pServer)
{
}

DTRank::~DTRank()
{
	
}

int DTRank::add(const RankData &rd)
{
	int ret = filter();
	RankData data = rd;
	data.items.clear();
	for(size_t i = 0; i < DT_NUM; ++i)
	{
		std::map<int, long>::const_iterator itr = rd.items.find(DT[i]);
		if(itr == rd.items.end())
		{
			continue;
		}
		data.items.insert(std::make_pair(itr->first, itr->second));
	}
	if(data.items.empty())
	{
		return ret;
	}
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
	return ret | flag;
}

void DTRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_show.begin(); i != m_show.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		rd.value = 0;
		for(int k = 0; k < DT_NUM; ++k)
		{
			std::map<int, long>::const_iterator j = i->items.find(DT[k]);
			if(j == i->items.end())
			{
				continue;
			}
			proto20rank::proto_rank_value val;
			val.key = j->first;
			val.val = j->second;
			rd.value2.push_back(val);
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}

MJRank::MJRank(CmpFunc func, GlobalServer *pServer) : Rank(RANK_TYPE_MJ, func, pServer)
{
}

MJRank::~MJRank()
{
	
}

int MJRank::add(const RankData &rd)
{
	std::map<int, long>::const_iterator i = rd.items.find(MJ_ID);
	if(i == rd.items.end())
	{
		return 0;
	}
	RankData data = rd;
	data.items.clear();
	data.items.insert(std::make_pair(i->first, i->second));
	int ret = Rank::add(data);
	if(ret)
	{
		sort();
	}
	return ret;
}

int MJRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
		return -1;
	}

	m_next_fresh = m_next_fresh + SECONDS_OF_WEEK;
	int idx = 0;
	m_show.clear();

	protosvr::SvrRankMailReward req;

	req.RANKTYPE = getType();

	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i, ++idx)
	{
		if(idx < 3)
		{
			m_show.push_back(*i);
		}
		req.LIST.push_back(i->uid);
	}

	if(!req.LIST.empty() && m_pServer)
	{
		m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();

	m_real.clear();

	//数据库榜单也清理
	clearRankDB();

	updateHaiMoRankFreshTime(time(NULL));
	return 0;
}

void MJRank::updateHaiMoRankFreshTime(int nTime)
{		
	//更新表数据
	QueryParam para;
	para.tableName = TBL_SYS_INFO; 
	para.keyFieldName = "itemid";
	para.key = SYSINFO_ITEMID_RANKLASTRESETTIME;
	para.userIdFiledName = "svrid";
	para.userid = m_pServer->m_pUserSvrCache->getDefaultPlatformSvrId(); //使用默认的大厅
	
	msgpack::sbuffer sbuf;
	msgpack::packer<msgpack::sbuffer> pker(&sbuf);
	pker.pack_map(4);
	pker.pack(std::string("svrid"));
	pker.pack(m_pServer->m_pUserSvrCache->getDefaultPlatformSvrId()); //使用默认的大厅
	pker.pack(std::string("itemid"));
	pker.pack((int)SYSINFO_ITEMID_RANKLASTRESETTIME);
	pker.pack(std::string("itemvalue"));
	pker.pack(base::i2s(nTime));
	pker.pack(std::string("updated"));
	pker.pack((int)time(NULL));

	m_pServer->m_pDbMgr->set(para, string(sbuf.data(), sbuf.size()));
}

void MJRank::init()
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100029, cfg))
	{
		return;
	}
	const std::string &str = cfg.get_value();
	int weekday = 0, hour = 0, min = 0, sec = 0;
	sscanf(str.c_str(), "%d|%d:%d:%d", &weekday, &hour, &min, &sec);
	m_next_fresh = time(NULL) + TimeFuncDef::getLeftTimeToWday(weekday, hour, min, sec);
}

void MJRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(MJ_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
	for(std::vector<RankData>::const_iterator i = m_show.begin(); i != m_show.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(MJ_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v2.push_back(rd);
	}
}

HSJFDRank::HSJFDRank(CmpFunc func, GlobalServer *pServer) : Rank(RANK_TYPE_JFD, func, pServer)
{
	
}

HSJFDRank::~HSJFDRank()
{
	
}

int HSJFDRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100032, cfg))
	{
		return 0;
	}
    std::map<int, long>::const_iterator i = rd.items.find(1);
    if(i == rd.items.end())
    {
        return 0;
    }
    int num = i->second;
    RankData data = rd;
    data.items.clear();
    data.items[HSJF_ID] = num;
    if(base::s2i(cfg.get_value()) > num)
    {
        return 0;
    }
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
    return flag;
}

int HSJFDRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
		return -1;
	}
	m_next_fresh = m_next_fresh + SECONDS_OF_DAY;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
		m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}

void HSJFDRank::init()
{
    m_next_fresh = TimeFuncDef::getTimeOfDayBegin((int)time(NULL)) + SECONDS_OF_DAY;
}

void HSJFDRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(HSJF_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}

HSJFWRank::HSJFWRank(CmpFunc func, GlobalServer *pServer):HSJFDRank(func, pServer)
{
    Rank::setType(RANK_TYPE_JFW);
}

HSJFWRank::~HSJFWRank()
{
	
}

void HSJFWRank::init()
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100034, cfg))
	{
		return;
	}
	const std::string &str = cfg.get_value();
	int weekday = 0, hour = 0, min = 0, sec = 0;
	sscanf(str.c_str(), "%d|%d:%d:%d", &weekday, &hour, &min, &sec);
	m_next_fresh = time(NULL) + TimeFuncDef::getLeftTimeToWday(weekday, hour, min, sec);
}

int HSJFWRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100033, cfg))
	{
		return 0;
	}
    std::map<int, long>::const_iterator i = rd.items.find(2);
    if(i == rd.items.end())
    {
        return 0;
    }
    int num = i->second;
    RankData data = rd;
    data.items.clear();
    data.items[HSJF_ID] = num;
    if(base::s2i(cfg.get_value()) > num)
    {
        return 0;
    }
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
    return flag;
}

int HSJFWRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
		return -1;
	}
	m_next_fresh = m_next_fresh + SECONDS_OF_WEEK;

	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
		m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}

ZQJFRank::ZQJFRank(CmpFunc func, GlobalServer *pServer) : Rank(RANK_TYPE_ZQJF, func, pServer)
{
	m_next_fresh = -1;
}

ZQJFRank::~ZQJFRank()
{
	
}

void ZQJFRank::init()
{
	CsvConfig::Activity cfg;
	if(!CsvConfigMgr::getInstance().findActivityByKey(1, cfg))
	{
		return;
	}
	char sDate[64] = {0};
	char sTime[64] = {0};
	sscanf(cfg.get_times().c_str(), "%*[^~]~%s %s", sDate, sTime);
	char sDateTime[64] = {0};
	snprintf(sDateTime, sizeof(sDateTime), "%s %s", sDate, sTime);
	struct tm tmEnd;
	strptime(sDateTime, "%F %T", &tmEnd);
	time_t ttEnd = mktime(&tmEnd);
	if(ttEnd < time(NULL))
	{
		m_next_fresh = -1;
	}
	else
	{
		m_next_fresh = ttEnd;
	}
}

int ZQJFRank::add(const RankData &rd)
{
	if(m_next_fresh == -1)
	{
		return 0;
	}
	int ret = filter();
    std::map<int, long>::const_iterator i = rd.items.find(LUCKY_VALUE_ID);
    if(i == rd.items.end())
    {
        return 0;
    }
	RankData data = rd;
	data.items.clear();
	data.items.insert(std::make_pair(i->first, i->second));
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
	return ret | flag;
}

int ZQJFRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || m_next_fresh == -1 || now < m_next_fresh)
	{
		return -1;
	}
	m_next_fresh = -1;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
		m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}

void ZQJFRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(LUCKY_VALUE_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}

GQJFRank::GQJFRank(CmpFunc func, GlobalServer *pServer) : ZQJFRank(func, pServer)
{
	m_next_fresh = -1;
	setType(RANK_TYPE_GQJF);
}

GQJFRank::~GQJFRank()
{
	
}

void GQJFRank::init()
{
	CsvConfig::Activity cfg;
	if(!CsvConfigMgr::getInstance().findActivityByKey(3, cfg))
	{
		return;
	}
	char sDate[64] = {0};
	char sTime[64] = {0};
	sscanf(cfg.get_times().c_str(), "%*[^~]~%s %s", sDate, sTime);
	char sDateTime[64] = {0};
	snprintf(sDateTime, sizeof(sDateTime), "%s %s", sDate, sTime);
	struct tm tmEnd;
	strptime(sDateTime, "%F %T", &tmEnd);
	time_t ttEnd = mktime(&tmEnd);
	if(ttEnd < time(NULL))
	{
		m_next_fresh = -1;
	}
	else
	{
		m_next_fresh = ttEnd;
	}
}

int GQJFRank::add(const RankData &rd)
{
	if(m_next_fresh == -1)
	{
		return 0;
	}
	int ret = filter();
    std::map<int, long>::const_iterator i = rd.items.find(LUCKY_VALUE_ID);
    if(i == rd.items.end())
    {
        return 0;
    }
	RankData data = rd;
	data.items.clear();
	data.items.insert(std::make_pair(i->first, i->second));
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
	return ret | flag;
}


DTJFRank::DTJFRank(CmpFunc func, GlobalServer *pServer) : ZQJFRank(func, pServer)
{
	m_next_fresh = -1;
	setType(RANK_TYPE_DTJF);
}

DTJFRank::~DTJFRank()
{
	
}

void DTJFRank::init()
{
	CsvConfig::Activity cfg;
	if(!CsvConfigMgr::getInstance().findActivityByKey(2, cfg))
	{
		return;
	}
	char sDate[64] = {0};
	char sTime[64] = {0};
	sscanf(cfg.get_times().c_str(), "%*[^~]~%s %s", sDate, sTime);
	char sDateTime[64] = {0};
	snprintf(sDateTime, sizeof(sDateTime), "%s %s", sDate, sTime);
	struct tm tmEnd;
	strptime(sDateTime, "%F %T", &tmEnd);
	time_t ttEnd = mktime(&tmEnd);
	if(ttEnd < time(NULL))
	{
		m_next_fresh = -1;
	}
	else
	{
		m_next_fresh = ttEnd;
	}
}

int DTJFRank::add(const RankData &rd)
{
	if(m_next_fresh == -1)
	{
		return 0;
	}
	int ret = filter();
    std::map<int, long>::const_iterator i = rd.items.find(DTJF_ID);
    if(i == rd.items.end())
    {
        return 0;
    }
	RankData data = rd;
	data.items.clear();
	data.items.insert(std::make_pair(i->first, i->second));
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
	return ret | flag;
}

void DTJFRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(DTJF_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}

CLRank::CLRank(CmpFunc func, GlobalServer *pServer) : Rank(RANK_TYPE_MATERIAL, func, pServer)
{
}

CLRank::~CLRank()
{
	
}

int CLRank::add(const RankData &rd)
{
	int ret = filter();
	RankData data = rd;
	data.items.clear();
	for(size_t i = 0; i < CL_NUM; ++i)
	{
		std::map<int, long>::const_iterator itr = rd.items.find(CL[i]);
		if(itr == rd.items.end())
		{
			continue;
		}
		data.items.insert(std::make_pair(itr->first, itr->second));
	}
	if(data.items.empty())
	{
		return ret;
	}
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
	return ret | flag;
}

void CLRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_show.begin(); i != m_show.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		rd.value = 0;
		for(int k = 0; k < CL_NUM; ++k)
		{
			std::map<int, long>::const_iterator j = i->items.find(CL[k]);
			if(j == i->items.end())
			{
				continue;
			}
			proto20rank::proto_rank_value val;
			val.key = j->first;
			val.val = j->second;
			rd.value2.push_back(val);
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}

RankMgr::RankMgr()
{
	
}

RankMgr::~RankMgr()
{
	for(std::map<int, Rank *>::iterator i = m_ranks.begin(); i != m_ranks.end(); ++i)
	{
		delete i->second;
	}
	m_ranks.clear();
}

Rank *RankMgr::createRank(int type, CmpFunc func, GlobalServer *pServer)
{
	Rank *p = NULL;
	switch(type)
	{
	case RANK_TYPE_JB:
		p = new JBRank(func, pServer);
		break;
	case RANK_TYPE_DJ:
		p = new DJRank(func, pServer);
		break;
	case RANK_TYPE_DT:
		p = new DTRank(func, pServer);
		break;
	case RANK_TYPE_MJ:
		p = new MJRank(func, pServer);
		break;
    case RANK_TYPE_JFD:
        p = new HSJFDRank(func, pServer);
        break;
    case RANK_TYPE_JFW:
        p = new HSJFWRank(func, pServer);
        break;
	case RANK_TYPE_ZQJF:
        p = new ZQJFRank(func, pServer);
        break;
	case RANK_TYPE_GQJF:
        p = new GQJFRank(func, pServer);
        break;		
	case RANK_TYPE_MATERIAL:
        p = new CLRank(func, pServer);
        break;
	case RANK_TYPE_DTJF:
        p = new DTJFRank(func, pServer);
        break;
  	case RANK_TYPE_DAY_BOTTLE:
  		p = new BottleDayRank(func, pServer);
  		break;
  	case RANK_TYPE_WEEK_BOTTLE:
  		p = new BottleWeekRank(func, pServer);
  		break;
  	case RANK_TYPE_DAY_RECHARGE:
		p = new RechargeDayRank(func, pServer);
		break;
	case RANK_TYPE_PET_DOG_DAY:
	    p = new PetDogDayRank(func, pServer);
	    break;
	case RANK_TYPE_PET_DOG_WEEK:
		p = new PetDogWeekRank(func, pServer);
		break;
    case RANK_TYPE_PET_MONKEY_DAY:
    	p = new PetMonkeyDayRank(func, pServer);
		break; 
	case RANK_TYPE_PET_MONKEY_WEEK:
    	p = new PetMonkeyWeekRank(func, pServer);
		break; 
	case RANK_TYPE_PET_DRAGON_DAY:
	    p = new PetDragonDayRank(func, pServer);
	    break;
	case RANK_TYPE_PET_DRAGON_WEEK:
		p = new PetDragonWeekRank(func, pServer);
	    break;
	case RANK_MATCH_AP_DAY:
		p = new AllPeopleMatchDayRank(func, pServer);
	    break;
	case RANK_MATCH_BP_DAY: 
		p = new BigPrizeMatchDayRank(func, pServer);
	    break;
	case RANK_MATCH_BP_WEEK:
		p = new BigPrizeMatchWeekRank(func, pServer);
	    break;
	case RANK_GS_WEEK:
		p = new GuessSizeRank(func, pServer);
		break;
	case RANK_EGG_ACTIVITY:
		p = new EggActivityRank(func, pServer);
		break;
	case RANK_DRAGON_JACKPOT:
		p = new DragonJackpotRank(func, pServer);
		break;
	default:
		break;
	}
	return p;
}

Rank *RankMgr::getRank(int type)
{
	std::map<int, Rank *>::iterator i = m_ranks.find(type);
	if(i == m_ranks.end())
	{
		return NULL;
	}
	return i->second;
}

void RankMgr::reload()
{
	for(std::map<int, Rank *>::iterator i = m_ranks.begin(); i != m_ranks.end(); ++i)
	{
		i->second->init();
	}
}

void RankMgr::init(base::FileConfig &confMgr, base::Logger *logger, GlobalServer *pServer)
{
	try
	{
		m_pRollLog = logger;
		m_pDbMgr = pServer->m_pDbMgr;
/*		//20190530 修改排行榜第一次上线，避免清掉数据，所以先读取旧数据
		//根据/tmp/tbl_rank文件是否存在，来确认是否已经处理过
		char *pFilePath = "/tmp/tbl_rank";
		bool bHadDealOldRedis = false; //旧的redis数据:tbl_rank 表上次启动是否已经被处理过
		if(access(pFilePath, F_OK) == 0)
		{
			bHadDealOldRedis = true;
		}

		m_pDbMgr = pServer->m_pDbMgr;

		QueryParam qp;
		qp.tableName = TBL_RANK;

		DataVector res;
		m_pDbMgr->get_cache_redis(qp, res);
		//20190530 end*/
		
		for(size_t i = 0; i < RANK_NUM; ++i)
		{
			Rank *p = createRank(ranks[i].type, ranks[i].cmp, pServer);
			if(NULL == p)
			{
				m_pRollLog->error("[%s:%d] create rank faid:%d", __FILE__, __LINE__, ranks[i].type);
				continue;
			}
			p->init();
			p->initFromDB();
			m_ranks.insert(std::make_pair(p->getType(), p));
		}

/*		//20190530 修改排行榜第一次上线，避免清掉数据，所以先读取旧数据
		if(res.empty() || bHadDealOldRedis)
		{
			return;
		}

		m_pRollLog->normal("RankMgr::init first time use mysql, read old redis res.size:%d !!", res.size());

		for(DataVector::const_iterator itr = res.begin(); itr != res.end(); ++itr)
		{
			msgpack::unpacked unpack;
			msgpack::unpack(&unpack, itr->data(), itr->length());

			msgpack::object obj = unpack.get();
			std::stringstream ss;
			ss<<obj;

			Json::Reader reader;
			Json::Value value;
			if(!reader.parse(ss.str(), value))
			{
				m_pRollLog->error("[%s:%d] Parse json failed", __FILE__, __LINE__);
				continue;
			}

			int type = value["type"].asInt();
			Rank *pRank = getRank(type);
			if(NULL == pRank)
			{
				m_pRollLog->error("[%s:%d] Rank not exist:%d", __FILE__, __LINE__, type);
				continue;
			}

			std::string real_data(value["real"].asString());
			std::string show_data(value["show"].asString());
			pRank->unserialize(real_data, show_data);
			pRank->saveRank();

		}

		//旧的redis启动后读取完了就可以删掉
		//m_pDbMgr->remove_cache_redis(qp); 

		//改成写个标志文件到/tmp目录下
		FILE      *fp = fopen(pFilePath, "a+");
	    if (fp != (FILE*)NULL)
	    {
	        fprintf(fp, "1");
	        fclose(fp);
	    }
		else
		{
			m_pRollLog->error("RankMgr::init first time create file:%s err !!", pFilePath);
		}*/
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

void RankMgr::fresh(time_t now)
{
	for (std::map<int, Rank *>::iterator i = m_ranks.begin(); i != m_ranks.end(); ++i)
	{
		//m_pRollLog->debug("RankMgr::fresh before i->second->fresh, type:%d", i->second->getType());
		if (0 == i->second->fresh(now))
		{
			//m_pRollLog->debug("RankMgr::fresh after i->second->fresh");
			//saveRank(i->second);
			i->second->saveRank(SHOW_RANK);
		}
	}
}

void RankMgr::saveRank(Rank *p)
{
	/*QueryParam qp;
	qp.tableName = TBL_RANK;
	qp.keyFieldName = "type";
	qp.key = p->getType();
	std::string data;
	p->serialize(data);
	if(data.empty())
	{
		return;
	}
	//m_pDbMgr->set(qp, data, p->needInsert());
	m_pDbMgr->set_cache_redis(qp,data);*/

	p->saveRank();
}

void RankMgr::add(const RankData &rd)
{
	if(rd.items.find(LUCKY_VALUE_ID) != rd.items.end())
	{
		int arr[] = {RANK_TYPE_ZQJF, RANK_TYPE_GQJF};
#define N (sizeof(arr) / sizeof(arr[0]))
		for(size_t i = 0; i < N; ++i)
		{
			std::map<int, Rank *>::iterator itr = m_ranks.find(arr[i]);
			if(itr == m_ranks.end() || itr->second->getNextFreshTime() == -1)
			{
				continue;
			}
			if(itr->second->add(rd))
			{
				//saveRank(itr->second);
				itr->second->saveRank(REAL_RANK);
				//榜单有变化打印出来
				//itr->second->printRank();
			}
			break;
		}
	}
	for(std::map<int, Rank *>::iterator i = m_ranks.begin(); i != m_ranks.end(); ++i)
	{
		if(i->second->getType() == RANK_TYPE_ZQJF || i->second->getType() == RANK_TYPE_GQJF)
		{
			continue;
		}
		if(i->second->add(rd))
		{
			//saveRank(i->second);
			i->second->saveRank(REAL_RANK);
			//榜单有变化打印出来
			//i->second->printRank();
		}
	}
}

void RankMgr::getRankList(int type, std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	Rank *p = getRank(type);
	if(NULL == p)
	{
		return;
	}
	p->getRankList(v1, v2);
}

void RankMgr::setRankIndex(const int nType, const int &nMin, const int &nMax)
{
	Rank *p = getRank(nType);
	if(NULL == p)
	{
		return;
	}
	p->setRankIndex(nMin, nMax);
}

int RankMgr::getTopRank(const int nType)
{
	Rank *p = getRank(nType);
	if(NULL == p)
	{
		return 0;
	}
	return p->getTopRank();
}

int RankMgr::getRankSize(const int nType)
{
	Rank *p = getRank(nType);
	if(NULL == p)
	{
		return 0;
	}
	return p->getCurRankSize();
}

void RankMgr::setUid(const int nType, const int &nUid)
{
	Rank *p = getRank(nType);
	if(NULL != p)
	{
		p->setUid(nUid);
	}
}

bool _Rank_JB_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(JB_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(JB_ID);

	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;

}

bool _Rank_DJ_Compare_Func(const RankData &r1, const RankData &r2)
{
	return r1.level > r2.level;
}

bool _Rank_DT_Compare_Func(const RankData &r1, const RankData &r2)
{
	for(size_t i = 0; i < DT_NUM; ++i)
	{
		std::map<int, long>::const_iterator i1 = r1.items.find(DT[i]);
		std::map<int, long>::const_iterator i2 = r2.items.find(DT[i]);
		if(i1 == r1.items.end())
		{
			if(i2 != r2.items.end())
			{
				return false;
			}
		}
		else
		{
			if(i2 == r2.items.end())
			{
				return true;
			}
			if(i1->second > i2->second)
			{
				return true;
			}
			else if(i1->second < i2->second)
			{
				return false;
			}
		}
	}
	return r1.tt < r2.tt;
}

bool _Rank_MJ_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(MJ_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(MJ_ID);

	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}

bool _Rank_JF_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(HSJF_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(HSJF_ID);

	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}

bool _Rank_BI_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(BOTTLE_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(BOTTLE_ID);

	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}

bool _Rank_LuckyValue_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(LUCKY_VALUE_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(LUCKY_VALUE_ID);
	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}

bool _Rank_DTJF_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(DTJF_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(DTJF_ID);
	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}

bool _Rank_CL_Compare_Func(const RankData &r1, const RankData &r2)
{
	for(size_t i = 0; i < CL_NUM; ++i)
	{
		std::map<int, long>::const_iterator i1 = r1.items.find(CL[i]);
		std::map<int, long>::const_iterator i2 = r2.items.find(CL[i]);
		if(i1 == r1.items.end())
		{
			if(i2 != r2.items.end())
			{
				return false;
			}
		}
		else
		{
			if(i2 == r2.items.end())
			{
				return true;
			}
			if(i1->second > i2->second)
			{
				return true;
			}
			else if(i1->second < i2->second)
			{
				return false;
			}
		}
	}
	return r1.tt < r2.tt;
}

bool _Rank_Day_Recharge_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(DAY_RECHARGE_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(DAY_RECHARGE_ID);

	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}


bool _Rank_Pet_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(PET_INTEGRAL_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(PET_INTEGRAL_ID);

	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}

bool _Rank_MATCH_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(MATCH_INTEGRAL_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(MATCH_INTEGRAL_ID);

	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}

bool _Rank_GS_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(GS_INTEGRAL_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(GS_INTEGRAL_ID);

	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}

bool _Rank_EGG_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(EGG_INTEGRAL_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(EGG_INTEGRAL_ID);

	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}

bool _Rank_DragonJackpot_Compare_Func(const RankData &r1, const RankData &r2)
{
	std::map<int, long>::const_iterator i1 = r1.items.find(DRAGON_JACKPOT_INTEGRAL_ID);
	std::map<int, long>::const_iterator i2 = r2.items.find(DRAGON_JACKPOT_INTEGRAL_ID);

	if(i1 == r1.items.end())
	{
		if(i2 == r2.items.end())
		{
			return r1.tt < r2.tt;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(i2 == r2.items.end())
		{
			return true;
		}
		else
		{
			return i1->second > i2->second;
		}
	}
	return false;
}

BottleDayRank::BottleDayRank(CmpFunc func, GlobalServer *pServer):Rank(RANK_TYPE_DAY_BOTTLE, func, pServer)
{

}

BottleDayRank::~BottleDayRank()
{

}

void BottleDayRank::init()
{
	m_next_fresh = TimeFuncDef::getTimeOfDayBegin((int)time(NULL)) + SECONDS_OF_DAY;
}

int BottleDayRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_next_fresh = m_next_fresh + SECONDS_OF_DAY;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}

int BottleDayRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100045, cfg))
	{
	  return 0;
	}
	std::map<int, long>::const_iterator i = rd.items.find(2009000101);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.items.clear();
	data.items[BOTTLE_ID] = num;
	if(base::s2i(cfg.get_value()) > num)
	{
	  return 0;
	}
	int flag = Rank::add(data);
	if(flag)
	{
	  sort();
	}
	return flag;
}

void BottleDayRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(BOTTLE_ID);
		if(j == i->items.end() || j->second <= 0)
		{
		  continue;
		}
		else
		{
		  rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}

BottleWeekRank::BottleWeekRank(CmpFunc func, GlobalServer *pServer):BottleDayRank(func, pServer)
{
	Rank::setType(RANK_TYPE_WEEK_BOTTLE);
}


BottleWeekRank::~BottleWeekRank()
{

}

void BottleWeekRank::init()
{
	CsvConfig::Common cfg;
  if(!CsvConfigMgr::getInstance().findCommonByKey(100047, cfg))
  {
    return;
  }
  const std::string &str = cfg.get_value();
  int weekday = 0, hour = 0, min = 0, sec = 0;
  sscanf(str.c_str(), "%d|%d:%d:%d", &weekday, &hour, &min, &sec);
  m_next_fresh = time(NULL) + TimeFuncDef::getLeftTimeToWday(weekday, hour, min, sec);
}

int BottleWeekRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_next_fresh = m_next_fresh + SECONDS_OF_WEEK;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}

int BottleWeekRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100046, cfg))
	{
		return 0;
	}
	std::map<int, long>::const_iterator i = rd.items.find(2009000102);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.items.clear();
	data.items[BOTTLE_ID] = num;
	if(base::s2i(cfg.get_value()) > num)
	{
	  return 0;
	}
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
	return flag;
}


RechargeDayRank::RechargeDayRank(CmpFunc func, GlobalServer *pServer):Rank(RANK_TYPE_DAY_RECHARGE, func, pServer)
{

}

RechargeDayRank::~RechargeDayRank()
{

}

void RechargeDayRank::init()
{
	m_next_fresh = TimeFuncDef::getTimeOfDayBegin((int)time(NULL)) + SECONDS_OF_DAY;
	//m_next_fresh = (int)time(NULL) +  60 * 15;
}

int RechargeDayRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
		return -1;
	}

	m_next_fresh = m_next_fresh + SECONDS_OF_DAY;
	//m_next_fresh = m_next_fresh + 60 * 15;

	m_show.clear();

	protosvr::SvrRankMailReward req;

	req.RANKTYPE = getType();

	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		m_show.push_back(*i);
		
		req.LIST.push_back(i->uid);
	}

	if(!req.LIST.empty() && m_pServer)
	{
		m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();

	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}

int RechargeDayRank::add(const RankData &rd)
{
	  std::map<int, long>::const_iterator i = rd.items.find(DAY_RECHARGE_ID);
	  if(i == rd.items.end())
	  {
	    return 0;
	  }
	  int num = i->second;
	  RankData data = rd;
	  data.items.clear();
	  data.items[DAY_RECHARGE_ID] = num;

	  int flag = Rank::add(data);
	  if(flag)
	  {
	    sort();
	  }
	  return flag;
}

void RechargeDayRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(DAY_RECHARGE_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
	for(std::vector<RankData>::const_iterator i = m_show.begin(); i != m_show.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(DAY_RECHARGE_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v2.push_back(rd);
	}
}


PetDogDayRank::PetDogDayRank(CmpFunc func, GlobalServer *pServer):Rank(RANK_TYPE_PET_DOG_DAY, func, pServer)
{

}


PetDogDayRank::~PetDogDayRank()
{

}

void PetDogDayRank::init()
{
	m_next_fresh = TimeFuncDef::getTimeOfDayBegin((int)time(NULL)) + SECONDS_OF_DAY;
}


int PetDogDayRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_show.clear();
	m_next_fresh = m_next_fresh + SECONDS_OF_DAY;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  m_show.push_back(*i);
	  req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}


int PetDogDayRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100054, cfg))
	{
	  return 0;
	}
	std::map<int, long>::const_iterator i = rd.items.find(11001);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.items.clear();
	data.items[PET_INTEGRAL_ID] = num;
	if(base::s2i(cfg.get_value()) > num)
	{
	  return 0;
	}
	int flag = Rank::add(data);
	if(flag)
	{
	  sort();
	}
	return flag;
}

void PetDogDayRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(PET_INTEGRAL_ID);
		if(j == i->items.end() || j->second <= 0)
		{
		  continue;
		}
		else
		{
		  rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}

	for(std::vector<RankData>::const_iterator i = m_show.begin(); i != m_show.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(PET_INTEGRAL_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v2.push_back(rd);
	}
}



PetDogWeekRank::PetDogWeekRank(CmpFunc func, GlobalServer *pServer):PetDogDayRank(func, pServer)
{
	Rank::setType(RANK_TYPE_PET_DOG_WEEK);
}

PetDogWeekRank::~PetDogWeekRank()
{

}

void PetDogWeekRank::init()
{
	CsvConfig::Common cfg;
  	if(!CsvConfigMgr::getInstance().findCommonByKey(100060, cfg))
  	{
    	return;
  	}
  	const std::string &str = cfg.get_value();
  	int weekday = 0, hour = 0, min = 0, sec = 0;
  	sscanf(str.c_str(), "%d|%d:%d:%d", &weekday, &hour, &min, &sec);
  	m_next_fresh = time(NULL) + TimeFuncDef::getLeftTimeToWday(weekday, hour, min, sec);
}

int PetDogWeekRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_show.clear();
	m_next_fresh = m_next_fresh + SECONDS_OF_WEEK;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  m_show.push_back(*i);
	  req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}
	
int PetDogWeekRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100057, cfg))
	{
		return 0;
	}
	std::map<int, long>::const_iterator i = rd.items.find(11002);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.items.clear();
	data.items[PET_INTEGRAL_ID] = num;
	if(base::s2i(cfg.get_value()) > num)
	{
	  return 0;
	}
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
	return flag;
}


PetMonkeyDayRank::PetMonkeyDayRank(CmpFunc func, GlobalServer *pServer):Rank(RANK_TYPE_PET_MONKEY_DAY, func, pServer)
{

}

PetMonkeyDayRank::~PetMonkeyDayRank()
{

}

void PetMonkeyDayRank::init()
{
	m_next_fresh = TimeFuncDef::getTimeOfDayBegin((int)time(NULL)) + SECONDS_OF_DAY;
}
	
int PetMonkeyDayRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_show.clear();
	m_next_fresh = m_next_fresh + SECONDS_OF_DAY;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  m_show.push_back(*i);
	  req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}
	
int PetMonkeyDayRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100055, cfg))
	{
	  return 0;
	}
	std::map<int, long>::const_iterator i = rd.items.find(12001);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.items.clear();
	data.items[PET_INTEGRAL_ID] = num;
	if(base::s2i(cfg.get_value()) > num)
	{
	  return 0;
	}
	int flag = Rank::add(data);
	if(flag)
	{
	  sort();
	}
	return flag;
}
	
void PetMonkeyDayRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(PET_INTEGRAL_ID);
		if(j == i->items.end() || j->second <= 0)
		{
		  continue;
		}
		else
		{
		  rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
	for(std::vector<RankData>::const_iterator i = m_show.begin(); i != m_show.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(PET_INTEGRAL_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v2.push_back(rd);
	}
}

PetMonkeyWeekRank::PetMonkeyWeekRank(CmpFunc func, GlobalServer *pServer):PetMonkeyDayRank(func, pServer)
{
	Rank::setType(RANK_TYPE_PET_MONKEY_WEEK);
}

PetMonkeyWeekRank::~PetMonkeyWeekRank()
{

}

void PetMonkeyWeekRank::init()
{
	CsvConfig::Common cfg;
  	if(!CsvConfigMgr::getInstance().findCommonByKey(100060, cfg))
  	{
    	return;
  	}
  	const std::string &str = cfg.get_value();
  	int weekday = 0, hour = 0, min = 0, sec = 0;
  	sscanf(str.c_str(), "%d|%d:%d:%d", &weekday, &hour, &min, &sec);
  	m_next_fresh = time(NULL) + TimeFuncDef::getLeftTimeToWday(weekday, hour, min, sec);
}

int PetMonkeyWeekRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_show.clear();
	m_next_fresh = m_next_fresh + SECONDS_OF_WEEK;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  m_show.push_back(*i);
	  req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}

int PetMonkeyWeekRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100058, cfg))
	{
		return 0;
	}
	std::map<int, long>::const_iterator i = rd.items.find(12002);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.items.clear();
	data.items[PET_INTEGRAL_ID] = num;
	if(base::s2i(cfg.get_value()) > num)
	{
	  return 0;
	}
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
	return flag;
}


PetDragonDayRank::PetDragonDayRank(CmpFunc func, GlobalServer *pServer):Rank(RANK_TYPE_PET_DRAGON_DAY, func, pServer)
{

}


PetDragonDayRank::~PetDragonDayRank()
{

}

void PetDragonDayRank::init()
{
	m_next_fresh = TimeFuncDef::getTimeOfDayBegin((int)time(NULL)) + SECONDS_OF_DAY;
}


int PetDragonDayRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_show.clear();
	m_next_fresh = m_next_fresh + SECONDS_OF_DAY;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  m_show.push_back(*i);
	  req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}


int PetDragonDayRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100056, cfg))
	{
	  return 0;
	}
	std::map<int, long>::const_iterator i = rd.items.find(13001);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.items.clear();
	data.items[PET_INTEGRAL_ID] = num;
	if(base::s2i(cfg.get_value()) > num)
	{
	  return 0;
	}
	int flag = Rank::add(data);
	if(flag)
	{
	  sort();
	}
	return flag;
}


void PetDragonDayRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(PET_INTEGRAL_ID);
		if(j == i->items.end() || j->second <= 0)
		{
		  continue;
		}
		else
		{
		  rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}

	for(std::vector<RankData>::const_iterator i = m_show.begin(); i != m_show.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(PET_INTEGRAL_ID);
		if(j == i->items.end() || j->second <= 0)
		{
			continue;
		}
		else
		{
			rd.value = j->second;
		}
		rd.sign = i->sign;
		v2.push_back(rd);
	}
}

PetDragonWeekRank::PetDragonWeekRank(CmpFunc func, GlobalServer *pServer):PetDragonDayRank(func, pServer)
{
	Rank::setType(RANK_TYPE_PET_DRAGON_WEEK);
}


PetDragonWeekRank::~PetDragonWeekRank()
{

}


void PetDragonWeekRank::init()
{
	CsvConfig::Common cfg;
  	if(!CsvConfigMgr::getInstance().findCommonByKey(100060, cfg))
 	{
    	return;
  	}
  	const std::string &str = cfg.get_value();
  	int weekday = 0, hour = 0, min = 0, sec = 0;
  	sscanf(str.c_str(), "%d|%d:%d:%d", &weekday, &hour, &min, &sec);
  	m_next_fresh = time(NULL) + TimeFuncDef::getLeftTimeToWday(weekday, hour, min, sec);
}


int PetDragonWeekRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_show.clear();
	m_next_fresh = m_next_fresh + SECONDS_OF_WEEK;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  m_show.push_back(*i);
	  req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}


int PetDragonWeekRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100059, cfg))
	{
		return 0;
	}
	std::map<int, long>::const_iterator i = rd.items.find(13002);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.items.clear();
	data.items[PET_INTEGRAL_ID] = num;
	if(base::s2i(cfg.get_value()) > num)
	{
	  return 0;
	}
	int flag = Rank::add(data);
	if(flag)
	{
		sort();
	}
	return flag;
}



AllPeopleMatchDayRank::AllPeopleMatchDayRank(CmpFunc func, GlobalServer *pServer):Rank(RANK_MATCH_AP_DAY, func, pServer)
{

}


AllPeopleMatchDayRank::~AllPeopleMatchDayRank()
{

}

void AllPeopleMatchDayRank::init()
{
	m_next_fresh = TimeFuncDef::getTimeOfDayBegin((int)time(NULL)) + SECONDS_OF_DAY;
}


int AllPeopleMatchDayRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_show.clear();
	m_userRank.clear();
	m_next_fresh = m_next_fresh + SECONDS_OF_DAY;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	int ranklist = 0;
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  m_show.push_back(*i);
	  req.LIST.push_back(i->uid);

	  ranklist += 1;
	  m_pServer->recordRankLog(i->uid, req.RANKTYPE, ranklist, i->items[MATCH_INTEGRAL_ID]);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}


int AllPeopleMatchDayRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100064, cfg))
	{
		return 0;
	}
	std::map<int, long>::const_iterator i = rd.items.find(RANK_MATCH_AP_DAY);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.items.clear();
	data.items[MATCH_INTEGRAL_ID] = num;
	if(base::s2i(cfg.get_value()) > num)
	{
	  return 0;
	}
	int flag = Rank::add(data);
	if(flag)
	{
	  sort();
	}
	return flag;
}


void AllPeopleMatchDayRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	m_nTopRank = 0;
	int rank_num = 0;

	if (m_clientRank.empty())
	{
		const std::map<int, CsvConfig::RankingList> rankingListMap = CsvConfigMgr::getInstance().getRankingListMap();
		for(std::map<int, CsvConfig::RankingList>::const_iterator it = rankingListMap.begin(); it != rankingListMap.end(); it++)
		{
			if(it->second.get_type() == getType())
			{
				string strRanking = it->second.get_ranking();
				strRanking.erase(std::remove(strRanking.begin(), strRanking.end(), '['), strRanking.end());
				strRanking.erase(std::remove(strRanking.begin(), strRanking.end(), ']'), strRanking.end());
				strRanking.erase(std::remove(strRanking.begin(), strRanking.end(), '{'), strRanking.end());
				strRanking.erase(std::remove(strRanking.begin(), strRanking.end(), '}'), strRanking.end());
				
				std::vector<string> split_array;
				common_utils::split_string(strRanking, split_array, ",");

				if (split_array.size() >= 2)
				{
					m_clientRank.push_back(base::s2i(split_array[1]));
				}
			}
		}
	}
	

	if (m_userRank.find(m_uid) != m_userRank.end())
	{
		m_nTopRank = m_userRank[m_uid];
	}

	int nPreIndex = -1;
	for (int i = 0; i < m_clientRank.size(); i++)
	{
		bool bContinue = true;
		int nIndex = 0;
		if (m_clientRank[i] <= m_real.size())
		{
			nIndex = m_clientRank[i] - 1;
		}
		else
		{
			nIndex = m_real.size() - 1;
			bContinue = false;
		}
		if (nIndex >= 0 && (nIndex != nPreIndex))
		{
			nPreIndex = nIndex;
			proto20rank::proto_rank_data rd;
			proto20rank::proto_rank_value rvalue;
			rd.pid = m_real[nIndex].uid;
			rd.nick = m_real[nIndex].nick;
			rd.facelook = m_real[nIndex].facelook;
			rd.vip = m_real[nIndex].vip;
			rvalue.key = m_real[nIndex].start_time;
			rvalue.val = m_real[nIndex].end_time;
			rd.value2.push_back(rvalue);
			std::map<int, long>::const_iterator j = m_real[nIndex].items.find(MATCH_INTEGRAL_ID);
			if(j == m_real[nIndex].items.end() || j->second <= 0)
			{
			  continue;
			}
			else
			{
			  rd.value = j->second;
			}
			rd.sign = m_real[nIndex].sign;
			v1.push_back(rd);
		}
		
		if (!bContinue)
		{
			break;
		}
	}

	rank_num = 0;
	for(std::vector<RankData>::const_iterator i = m_show.begin(); i != m_show.end(); ++i)
	{
		rank_num += 1;
		if (((rank_num <= m_nRankMax) && (rank_num >= m_nRankMin)) || (m_nRankMax <= 0))
		{
			proto20rank::proto_rank_data rd;
			proto20rank::proto_rank_value rvalue;
			rd.pid = i->uid;
			rd.nick = i->nick;
			rd.facelook = i->facelook;
			rd.vip = i->vip;
			rvalue.key = i->start_time;
			rvalue.val = i->end_time-60;
			rd.value2.push_back(rvalue);
			std::map<int, long>::const_iterator j = i->items.find(MATCH_INTEGRAL_ID);
			if(j == i->items.end() || j->second <= 0)
			{
				continue;
			}
			else
			{
				rd.value = j->second;
			}
			rd.sign = i->sign;
			v2.push_back(rd);
		}
		else
		{
			break;
		}
	}
}


BigPrizeMatchDayRank::BigPrizeMatchDayRank(CmpFunc func, GlobalServer *pServer):AllPeopleMatchDayRank(func, pServer)
{
	Rank::setType(RANK_MATCH_BP_DAY);
}


BigPrizeMatchDayRank::~BigPrizeMatchDayRank()
{

}


int BigPrizeMatchDayRank::add(const RankData &rd)
{
	CsvConfig::Common cfg;
	if(!CsvConfigMgr::getInstance().findCommonByKey(100065, cfg))
	{
		return 0;
	}
	std::map<int, long>::const_iterator i = rd.items.find(RANK_MATCH_BP_DAY);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.items.clear();
	data.items[MATCH_INTEGRAL_ID] = num;
	if(base::s2i(cfg.get_value()) > num)
	{
	  return 0;
	}
	int flag = Rank::add(data);
	if(flag)
	{
	  sort();
	}
	return flag;
}


BigPrizeMatchWeekRank::BigPrizeMatchWeekRank(CmpFunc func, GlobalServer *pServer):BigPrizeMatchDayRank(func, pServer)
{
	Rank::setType(RANK_MATCH_BP_WEEK);
}


BigPrizeMatchWeekRank::~BigPrizeMatchWeekRank()
{

}


void BigPrizeMatchWeekRank::init()
{
	CsvConfig::Common cfg;
  	if(!CsvConfigMgr::getInstance().findCommonByKey(100061, cfg))
 	{
    	return;
  	}
  	const std::string &str = cfg.get_value();
  	int weekday = 0, hour = 0, min = 0, sec = 0;
  	sscanf(str.c_str(), "%d|%d:%d:%d", &weekday, &hour, &min, &sec);
  	m_next_fresh = time(NULL) + TimeFuncDef::getLeftTimeToWday(weekday, hour, min, sec);
}


int BigPrizeMatchWeekRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	//m_show.clear();
	m_userRank.clear();
	m_next_fresh = m_next_fresh + SECONDS_OF_WEEK;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	int ranklist = 0;
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  m_show.push_back(*i);
	  req.LIST.push_back(i->uid);
	  ranklist += 1;
	  m_pServer->recordRankLog(i->uid, req.RANKTYPE, ranklist, i->items[MATCH_INTEGRAL_ID]);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}
	while (m_show.size() > 7)
	{
		vector<RankData>::iterator del = m_show.begin();
    	m_show.erase(del); 
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}


int BigPrizeMatchWeekRank::add(const RankData &rd)
{
	std::map<int, long>::const_iterator i = rd.items.find(RANK_MATCH_BP_WEEK);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.start_time = m_next_fresh - SECONDS_OF_WEEK;
	data.end_time = m_next_fresh;
	data.items.clear();
	data.items[MATCH_INTEGRAL_ID] = num;
	int flag = Rank::add(data);
	if(flag)
	{
	  sort();
	}
	return flag;
}


GuessSizeRank::GuessSizeRank(CmpFunc func, GlobalServer *pServer):Rank(RANK_GS_WEEK, func, pServer)
{

}

GuessSizeRank::~GuessSizeRank()
{

}

void GuessSizeRank::init()
{
	CsvConfig::Activity cfg;
  	if(!CsvConfigMgr::getInstance().findActivityByKey(ACTIVITYTYPE_GUESS_SIZE, cfg))
 	{
    	return;
  	}
  	const std::string &str = cfg.get_times();
  	vector<string> strOne;
  	common_utils::split_string(str, strOne, "~");
  	if (strOne.size() >= 2)
  	{
  		int hour = 0, min = 0, sec = 0, year = 0, month = 0, day = 0;
  		sscanf(strOne[1].c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec);
  		
  		struct tm tmTime;
  		tmTime.tm_year = year-1900;  
		tmTime.tm_mon  = month-1;  
		tmTime.tm_mday = day;  
		tmTime.tm_hour = hour;  
		tmTime.tm_min  = min;  
		tmTime.tm_sec  = sec;  
  		m_next_fresh = (int)mktime(&tmTime);  
  		//cout << strOne[1] << "|" << m_next_fresh << endl;
  	}
}

int GuessSizeRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	//m_pRollLog->debug("GuessSizeRank::fresh now:%d, m_next_fresh:%d, m_real.size:%d, type:%d", (int)now, m_next_fresh, m_real.size(), getType());
	m_userRank.clear();
	m_next_fresh = -1;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	int ranklist = 0;
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  ranklist += 1;
	  m_pServer->recordRankLog(i->uid, req.RANKTYPE, ranklist, i->items[GS_INTEGRAL_ID]);
	  
	  req.LIST.push_back(i->uid);
	  //m_pRollLog->debug("GuessSizeRank::fresh req.LIST uid:%d", i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	  //m_pRollLog->debug("GuessSizeRank::fresh sendSvrMsg LIST:%d", req.LIST.size());
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();

	//m_pRollLog->debug("GuessSizeRank::fresh end");
	
	return 0;
}

int GuessSizeRank::add(const RankData &rd)
{
	std::map<int, long>::const_iterator i = rd.items.find(RANK_GS_WEEK);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.start_time = int(time(NULL));
	data.end_time = m_next_fresh;
	data.items.clear();
	data.items[GS_INTEGRAL_ID] = num;
	int flag = Rank::add(data);
	if(flag)
	{
	  sort();
	}
	return flag;
}

void GuessSizeRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(GS_INTEGRAL_ID);
		if(j == i->items.end() || j->second <= 0)
		{
		  continue;
		}
		else
		{
		  rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}


EggActivityRank::EggActivityRank(CmpFunc func, GlobalServer *pServer):Rank(RANK_EGG_ACTIVITY, func, pServer)
{

}

EggActivityRank::~EggActivityRank()
{

}

void EggActivityRank::init()
{
	CsvConfig::Activity cfg;
  	if(!CsvConfigMgr::getInstance().findActivityByKey(ACTIVITYTYPE_EGG, cfg))
 	{
    	return;
  	}
  	const std::string &str = cfg.get_times();
  	vector<string> strOne;
  	common_utils::split_string(str, strOne, "~");
  	if (strOne.size() >= 2)
  	{
  		int hour = 0, min = 0, sec = 0, year = 0, month = 0, day = 0;
  		sscanf(strOne[1].c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec);
  		
  		struct tm tmTime;
  		tmTime.tm_year = year-1900;  
		tmTime.tm_mon  = month-1;  
		tmTime.tm_mday = day;  
		tmTime.tm_hour = hour;  
		tmTime.tm_min  = min;  
		tmTime.tm_sec  = sec;  
  		m_next_fresh = (int)mktime(&tmTime);  
  		//cout << strOne[1] << "|" << m_next_fresh << endl;
  	}
}

int EggActivityRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_userRank.clear();
	m_next_fresh = -1;
	protosvr::SvrRankMailReward req;
	req.RANKTYPE = getType();
	int ranklist = 0;
	for(std::vector<RankData>::iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
	  ranklist += 1;
	  m_pServer->recordRankLog(i->uid, req.RANKTYPE, ranklist, i->items[EGG_INTEGRAL_ID]);
	  
	  req.LIST.push_back(i->uid);
	}
	if(!req.LIST.empty() && m_pServer)
	{
	  m_pServer->sendSvrMsg(PLATFORM_SERVER, protosvr::SVR_RANKMAILREWARD, req);
	}

	//发榜后写历史表
	saveRankHistory();
	
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	
	return 0;
}

int EggActivityRank::add(const RankData &rd)
{
	std::map<int, long>::const_iterator i = rd.items.find(RANK_EGG_ACTIVITY);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.start_time = int(time(NULL));
	data.end_time = m_next_fresh;
	data.items.clear();
	data.items[EGG_INTEGRAL_ID] = num;
	int flag = Rank::add(data);
	if(flag)
	{
	  sort();
	}
	return flag;
}

void EggActivityRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(EGG_INTEGRAL_ID);
		if(j == i->items.end() || j->second <= 0)
		{
		  continue;
		}
		else
		{
		  rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}

DragonJackpotRank::DragonJackpotRank(CmpFunc func, GlobalServer *pServer):Rank(RANK_DRAGON_JACKPOT, func, pServer)
{

}

DragonJackpotRank::~DragonJackpotRank()
{

}

void DragonJackpotRank::init()
{
	CsvConfig::Common cfg;
  	if(!CsvConfigMgr::getInstance().findCommonByKey(100069, cfg))
 	{
 		m_next_fresh = TimeFuncDef::getTimeOfDayBegin((int)time(NULL)) + SECONDS_OF_DAY;
  	}
  	else
  	{
  		string strConfig = cfg.get_value();
	  	if (!strConfig.empty())
	  	{	
	  		int nNow = time(NULL);
	  		int hour = 0, min = 0, sec = 0;
	        sscanf(strConfig.c_str(), "%d:%d:%d", &hour, &min, &sec);
	        int nAppointTime = TimeFuncDef::getAssignTimeStamp(hour, min, sec);
	        if (nNow > nAppointTime)
	        {
	        	m_next_fresh = nAppointTime + SECONDS_OF_DAY;
	        }
	        else
	        {
	        	m_next_fresh = nAppointTime;
	        }
	  	}
	  	else
	  	{
	  		m_next_fresh = TimeFuncDef::getTimeOfDayBegin((int)time(NULL)) + SECONDS_OF_DAY;
	  	}
	  	//m_pRollLog->normal("DragonJackpotRank::init m_next_fresh:%d", m_next_fresh);
  	}
}

int DragonJackpotRank::fresh(time_t now)
{
	if(m_next_fresh == -1 || now < m_next_fresh)
	{
	  return -1;
	}
	m_userRank.clear();
	m_next_fresh += SECONDS_OF_DAY;
	//发榜后写历史表
	saveRankHistory();
	m_real.clear();

	//数据库榜单也清理
	clearRankDB();
	return 0;
}
	
int DragonJackpotRank::add(const RankData &rd)
{
	std::map<int, long>::const_iterator i = rd.items.find(RANK_DRAGON_JACKPOT);
	if(i == rd.items.end())
	{
	  return 0;
	}
	int num = i->second;
	RankData data = rd;
	data.start_time = int(time(NULL));
	data.end_time = m_next_fresh;
	data.items.clear();
	data.items[DRAGON_JACKPOT_INTEGRAL_ID] = num;
	int flag = Rank::add(data);
	if(flag)
	{
	  sort();
	}
	return flag;
}

void DragonJackpotRank::getRankList(std::vector<proto20rank::proto_rank_data> &v1, std::vector<proto20rank::proto_rank_data> &v2)
{
	for(std::vector<RankData>::const_iterator i = m_real.begin(); i != m_real.end(); ++i)
	{
		proto20rank::proto_rank_data rd;
		rd.pid = i->uid;
		rd.nick = i->nick;
		rd.facelook = i->facelook;
		rd.vip = i->vip;
		std::map<int, long>::const_iterator j = i->items.find(DRAGON_JACKPOT_INTEGRAL_ID);
		if(j == i->items.end() || j->second <= 0)
		{
		  continue;
		}
		else
		{
		  rd.value = j->second;
		}
		rd.sign = i->sign;
		v1.push_back(rd);
	}
}	

