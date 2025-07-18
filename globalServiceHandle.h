#ifndef _GLOBAL_SERVICE_HANDLE_H
#define _GLOBAL_SERVICE_HANDLE_H
#include "GlobalService.h"
#include "base/logger.h"
#include "base/utils.h"
#include "base/configer.h"
#include "base/fileconfig.h"
#include "dbmgr.h"
#include "daytrace.h"
#include "common_def.h"
#include "errorcode.h"

using namespace  ::gs2global;
using namespace base;

class GlobalServer;
class GlobalServiceHandler : virtual public GlobalServiceIf 
{
public:
    GlobalServiceHandler(GlobalServer* svr);
    ~GlobalServiceHandler(void);

	//1.获取昵称在系统中存在的个数，用于判断昵称是否存在
	virtual int32_t getNickCnt(const std::string& strNick);

	//2.获取系统中绑定了这个手机号的id个数
	virtual int32_t getIdCntBindPhone(const std::string& strPhone);

	//3.获取系统中openid绑定的id数
	virtual int32_t getIdCntBindOpenid(const std::string& strOpenid);

	//4.根据推广码获取玩家id
	virtual int32_t getUidByExtensionCode(const int64_t lnExtensionCode);

	//5.判断openid是否曾被绑定过
	virtual bool isOpenidBound(const std::string& strOpenid);

private:
    //DayTrace *m_pNewUserDayLog;
    base::Logger* m_pRollLog;
    base::FileConfig* m_pConf;
    DBMgr* m_pDbMgr;    
    GlobalServer* m_pSvr;
    //LOCK m_lockHandle;
};
#endif
