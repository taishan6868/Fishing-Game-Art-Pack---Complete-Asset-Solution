#ifndef __GLOBAL_PLAYER_H__
#define __GLOBAL_PLAYER_H__

#include <vector>
#include <time.h>
#include <string>
#include "dbmgr.h"
#include "common_def.h"


class GlobalServer;


//全局玩家信息，手机号，昵称，openid绑定的uid数等等

class GlobalPlayer
{
public:
	GlobalPlayer();

	~GlobalPlayer();


public:
	void init(GlobalServer *pServer);

	void loadNick();

	void loadWxInfo();

	void updatePlayerNick(const char* data,unsigned int nLength);

	bool isNickExist(string strNick);

	//获取最早绑定这个openid的玩家id
	int getFirstUidBindOpenid(string strOpenid);

	//获取绑定这个openid的所有玩家id，返回字符串，用|分割
	string getUidBindOpenid(string strOpenid);

	//绑定玩家id到微信openid
	void bindUid2Openid(int nUid, string strOpendId);

	//解除绑定微信openid
	void unBindUid2Openid(int nUid, string strOpendId);

	int getIdCntBindOpenid(const std::string strOpenid);

	//判断openid是否曾被绑定过
	bool isOpenidBound(string strOpendId);

	//程序启动推广码玩家id加载
	void loadExtensionCode(); 

	//上报推广码玩家id
	void uploadExtensionCode(int nUid, long lnExtensionCode);

	//根据推广码获取玩家id
	int getUidByExtensionCode(const long lnExCode);

	

private:
	DBMgr *m_pDbMgr;
	base::Logger *m_pRollLog;
	GlobalServer *m_pServer;

	std::set<string> m_setNick; //全局玩家昵称集合
	base::Lock m_lockNick ;

	std::map<string, std::list<int> > m_mapOpenid2UidList; //map的key是openid，value是绑定这个openid的所有玩家id的列表，列表绑定时间升序排列，最早绑定的id放在第一位
	std::set<string> m_setOpenid; //在游戏绑定过的openid集合

	std::map<long, int> m_mapExtensionCode2Uid; //key 玩家推广码 , value 玩家id
};
#endif


