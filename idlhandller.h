#include <iostream>
#include <sys/time.h>
#include "base/thread.h"
#include <stdio.h>
#include "global_server.h"
#include "common_utils.h"
#include "globalServiceHandle.h"
#include <protocol/TBinaryProtocol.h>
#include <server/TThreadPoolServer.h>
#include <transport/TServerSocket.h>
#include <server/TSimpleServer.h> 
#include <transport/TBufferTransports.h>
#include "processor/TMultiplexedProcessor.h"
#include "server/TNonblockingServer.h"
#include "concurrency/PosixThreadFactory.h"
#include "concurrency/ThreadManager.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::concurrency;
using namespace base;
using namespace std;
using boost::shared_ptr;

class GlobalServer;

class IDLHandller : public base::Thread
{
public:
    IDLHandller(GlobalServer*    proxy){m_pSproxy = proxy;};
    ~IDLHandller(){};
private:
      GlobalServer* m_pSproxy;
      
protected:
    virtual int run()
    {
        while(1)
        {
            if(m_threadstate == 0)
            {
                return 0;
            }
            
            try
            { 
                int nPort = base::s2i(m_pSproxy->m_confMgr["globalserver\\thrift\\port"]);
                string strIp = m_pSproxy->m_confMgr["globalserver\\thrift\\ip"];
                int nThreadNum = base::s2i(m_pSproxy->m_confMgr["globalserver\\thrift\\threadnum"]);
                 
                 shared_ptr<GlobalServiceHandler> handler(new GlobalServiceHandler(m_pSproxy));
                 shared_ptr<TProcessor> spMsgProcessor(new GlobalServiceProcessor(handler));
                 //shared_ptr<TServerTransport> serverTransport(new TServerSocket(strIp, nPort));
                 shared_ptr<TServerTransport> serverTransport(new TServerSocket(nPort));
                 shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
                 shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
                
                 //shared_ptr<TMultiplexedProcessor> cMultiProcessor(new TMultiplexedProcessor());
                // cMultiProcessor->registerProcessor("LoginService", spLoginProcessor);
                
                 shared_ptr<ThreadManager> spThreadManager(ThreadManager::newSimpleThreadManager(nThreadNum));
                 shared_ptr<ThreadFactory> spPthreadFactory(new PosixThreadFactory());
                 spThreadManager->threadFactory(spPthreadFactory);
                 spThreadManager->start();
                
                 //TThreadPoolServer server(cMultiProcessor, serverTransport, transportFactory, protocolFactory, spThreadManager);
                 TNonblockingServer server(spMsgProcessor,protocolFactory,nPort,spThreadManager);
                 server.setTaskExpireTime(3000);
                 server.serve();

            }
			catch(conf_load_error &ex)
			{
				cout<<"IDLHandller::run failed:"<<ex.what()<<endl;
				throw ex;
			}
			catch(conf_not_find &ex)
			{
				cout<<"IDLHandller::run conf_not_find:"<<ex.what()<<endl;
				throw ex;
			}
            catch (...)
            {
                //m_pSproxy->m_pRollLog->error("IDLHandller:except,exit");
				cout<<"IDLHandller:except,exit"<<endl;
                throw "IDLHandller:except,exit";
            }
        }
    };
};
