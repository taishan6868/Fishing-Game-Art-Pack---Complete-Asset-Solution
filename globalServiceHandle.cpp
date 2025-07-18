#include "globalServiceHandle.h"
#include <iostream>
#include "global_server.h"
#include "http_req.h"
#include "global_player.h"

using namespace std;
using namespace  ::gs2global;
using namespace base;

GlobalServiceHandler::GlobalServiceHandler(GlobalServer* svr)
{
    m_pSvr =  svr;
    m_pRollLog = svr->m_pRollLog;
    m_pDbMgr   = svr->m_pDbMgr;
    m_pConf    = &(svr->m_confMgr);            
} 
GlobalServiceHandler::~GlobalServiceHandler(void)
{

}




int32_t GlobalServiceHandler::getNickCnt(const std::string& strNick)
{
    m_pRollLog->debug("GlobalServiceHandler::getNickCnt begining(strNick:%s)", strNick.c_str());
    //GameUserInfo* pGameUserInfo;
    try
    {
        int res = 0;
        //pGameUserInfo = this->_checkUserVaild(nUid, res);
        return res;
    }
    catch(...)
    {
        m_pRollLog->error("GlobalServiceHandler::getNickCnt unknow exception");
        return 0;
    }
}

int32_t GlobalServiceHandler::getIdCntBindPhone(const std::string& strPhone)
{
    m_pRollLog->debug("GlobalServiceHandler::getIdCntBindPhone begining(strPhone:%s)", strPhone.c_str());
    //GameUserInfo* pGameUserInfo;
    try
    {
        int res = 0;
        //pGameUserInfo = this->_checkUserVaild(nUid, res);
        return res;
    }
    catch(...)
    {
        m_pRollLog->error("GlobalServiceHandler::getIdCntBindPhone unknow exception");
        return 0;
    }
}

int32_t GlobalServiceHandler::getIdCntBindOpenid(const std::string& strOpenid)
{
    m_pRollLog->debug("GlobalServiceHandler::getIdCntBindOpenid begining(strOpenid:%s)", strOpenid.c_str());
    //GameUserInfo* pGameUserInfo;
    try
    {
        int res = m_pSvr->m_pGlobalPlayer->getIdCntBindOpenid(strOpenid);
        //pGameUserInfo = this->_checkUserVaild(nUid, res);
        return res;
    }
    catch(...)
    {
        m_pRollLog->error("GlobalServiceHandler::getIdCntBindOpenid unknow exception");
        return 0;
    }
}

int32_t GlobalServiceHandler::getUidByExtensionCode(const int64_t lnExtensionCode)
{
    m_pRollLog->debug("GlobalServiceHandler::getUidByExtensionCode begining(lnExtensionCode:%ld)", lnExtensionCode);
    //GameUserInfo* pGameUserInfo;
    try
    {
        int res = m_pSvr->m_pGlobalPlayer->getUidByExtensionCode(lnExtensionCode);
        //pGameUserInfo = this->_checkUserVaild(nUid, res);
        return res;
    }
    catch(...)
    {
        m_pRollLog->error("GlobalServiceHandler::getIdCntBindOpenid unknow exception");
        return 0;
    }
}
bool GlobalServiceHandler::isOpenidBound(const std::string& strOpenid)
{
	m_pRollLog->debug("GlobalServiceHandler::isOpenidBound begining(strOpenid:%s)", strOpenid.c_str());

    try
    {
        return m_pSvr->m_pGlobalPlayer->isOpenidBound(strOpenid);
    }
    catch(...)
    {
        m_pRollLog->error("GlobalServiceHandler::isOpenidBound unknow exception");
        return false;
    }
}