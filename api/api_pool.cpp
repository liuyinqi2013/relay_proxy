#include    "api_pool.h"
#include    "middle_api.h"
#include    "relay_api.h"
#include    "trpc_sys_exception.h"
#include     "hasite_conf.h"
#include    "log.h"

CApiPool::CApiPool(CHaSiteConf & hasite_conf, CSystemConf& sys_conf, const  std::string & server_name, 
            const int conn_num, const int max_conn_num, const int time_out)
    :
    _server_name(server_name),
    _need_switch(false),
    _conn_num(conn_num),
    _max_conn_num(max_conn_num),
    _hasite_conf(hasite_conf),
    _sys_conf(sys_conf),
    _time_out(time_out),
    _pool_size(0),
    _switch_factor(0),
    _switch_state(EM_SWITCH_PRIOR) 
{
    _api_pool = &_pool[_switch_factor%2];
}

CApiPool::~CApiPool()
{
}

void  
CApiPool::set_switch_on()
{
    CAutoWLock l(_mutex);

    _need_switch = true;
}

void  
CApiPool::set_switch_off()
{
    CAutoWLock l(_mutex);

    _need_switch = false;
}

CApiBase * CApiPool::get(const char * user, const char * passwd)
{
    // 先加写锁
    CAutoWLock l(_mutex);

    return _get(user, passwd);
}

void CApiPool::release(CApiBase * api, bool force_close)
{
    // 先加写锁
    CAutoWLock l(_mutex);

    return _release(api, force_close);
}

/*
CApiBase * CApiPool::_get(const char * user, const char * passwd)
{
    if (_switch_state == EM_SWITCH_ONGOING)
    {
        trpc_debug_log("EM_SWITCH_ONGOING, clear some apis");

        assert(_switch_factor > 0);
        bool switch_complete = true;

        std::vector<CApiBase *> & pool = _pool[(_switch_factor-1)%2];
        trpc_debug_log("old pool index:=%d, old pool size=%lu", (_switch_factor-1)%2, pool.size());
        for(std::vector<CApiBase *>::iterator it = pool.begin(); it != pool.end(); ++it)
        {
            if ((*it) != NULL && (*it)->is_free())
            {
                trpc_debug_log("delete one invalid free api");
                delete (*it);
                *it = NULL;
            }
        }

        for(std::vector<CApiBase *>::iterator it = pool.begin(); it != pool.end(); ++it)
        {
            if ((*it) != NULL && !(*it)->is_free())
            {
                trpc_debug_log("find a still used api");
                switch_complete = false;
                break;
            }
        }

        if (switch_complete)
        {
           trpc_debug_log("all apis have been cleared, EM_SWITCH_FINISHED");
           pool.clear();
           _switch_state = EM_SWITCH_FINISHED;
        }
    }

    if (!_need_switch)
    {
        // 遍历，查找空闲
        //for (std::vector<CApiBase *>::iterator it = _api_pool.begin(); it != _api_pool.end(); ++it)
        for (std::vector<CApiBase *>::iterator it = _api_pool->begin(); it != _api_pool->end(); ++it)
        {
            if ((*it)->is_free())
            {
                (*it)->set_used();

                //trpc_debug_log("FIND GET_API_INDEX:%d, IDC:%s\n", (*it)->index(), (*it)->idc());

                return (*it);
            }
        }
    }
    else
    {
        std::string next_idc = _sys_conf.next_idc(_cur_idc.c_str());
        trpc_debug_log("API_POOL:need switch from %s to next idc:%s", _cur_idc.c_str(), next_idc.c_str());

        if (next_idc == CHaSiteConf::default_idc)
        {
            trpc_debug_log("already reach the last IDC, do not switch");
            trpc_write_statslog("already reach the last IDC, do not switch\n");
            _need_switch = false;

            for (std::vector<CApiBase *>::iterator it = _api_pool->begin(); it != _api_pool->end(); ++it)
            {
                if ((*it)->is_free())
                {
                    (*it)->set_used();

                    //trpc_debug_log("FIND GET_API_INDEX:%d, IDC:%s\n", (*it)->index(), (*it)->idc());

                    return (*it);
                }
            }
        }
        else
        {
            trpc_write_statslog("begin to switch from %s to next idc:%s\n", _cur_idc.c_str(), next_idc.c_str());

            if (_switch_state == EM_SWITCH_ONGOING)
            {
                trpc_debug_log("switching IDC has not finished, please wait!");
                trpc_write_statslog("switching IDC has not finished, please wait!\n");
                THROW(CServerException, ERR_SERVER_BUSY, "Last switch has not finished, please wait!");
            }
            else if (_switch_state == EM_SWITCH_PRIOR || _switch_state == EM_SWITCH_FINISHED)
            {
                _switch_state = EM_SWITCH_ONGOING;
                trpc_write_statslog("SWITCH_ONGOING\n");
            }

            int index = (++_switch_factor) % 2;
            _api_pool = &_pool[index];
            _pool_size = _api_pool->size();
            trpc_debug_log("new pool index == %d, new pool size == %d", index,  _pool_size);
            std::string last_idc = _cur_idc;
            _cur_idc = next_idc;
            _need_switch = false;
            trpc_debug_log("switch completed, current IDC=%s\n", _cur_idc.c_str());
            trpc_write_statslog("switch completed, current IDC=%s\n", _cur_idc.c_str());

        }
    }

    trpc_debug_log("_need_switch=%d, create an api from :%s\n", _need_switch, _cur_idc.c_str());

    // 检查，如果没有超过最大的值，创建一个新的API，并加入_api_pool中
    if (_pool_size < _max_conn_num)
    {
        return _add_api(user, passwd);
    }
    else
    {
        // 无可用节点
        THROW(CServerException, ERR_SERVER_BUSY, "server_site: %s no free conn max: %d, now: %d",
              _server_name.c_str(), _max_conn_num, _pool_size);
    }

    // never go here
    return NULL;
}
*/

CApiBase * CApiPool::_get(const char * user, const char * passwd)
{
    if (_switch_state == EM_SWITCH_ONGOING)
    {
        trpc_debug_log("EM_SWITCH_ONGOING, clear some apis");

        assert(_switch_factor > 0);
        bool switch_complete = true;

        std::vector<CApiBase *> & pool = _pool[(_switch_factor-1)%2];
        trpc_debug_log("old pool index:=%d, old pool size=%lu", (_switch_factor-1)%2, pool.size());
        for(std::vector<CApiBase *>::iterator it = pool.begin(); it != pool.end(); ++it)
        {
            if ((*it) != NULL && (*it)->is_free())
            {
                trpc_debug_log("delete one invalid free api");
                delete (*it);
                *it = NULL;
            }
        }

        for(std::vector<CApiBase *>::iterator it = pool.begin(); it != pool.end(); ++it)
        {
            if ((*it) != NULL && !(*it)->is_free())
            {
                trpc_debug_log("find a still used api");
                switch_complete = false;
                break;
            }
        }

        if (switch_complete)
        {
           trpc_debug_log("all apis have been cleared, EM_SWITCH_FINISHED");
           pool.clear();
           _switch_state = EM_SWITCH_FINISHED;
        }
    }

    if (!_need_switch)
    {
        // 遍历，查找空闲
        //for (std::vector<CApiBase *>::iterator it = _api_pool.begin(); it != _api_pool.end(); ++it)
        for (std::vector<CApiBase *>::iterator it = _api_pool->begin(); it != _api_pool->end(); ++it)
        {
            if ((*it)->is_free())
            {
                (*it)->set_used();

                //trpc_debug_log("FIND GET_API_INDEX:%d, IDC:%s\n", (*it)->index(), (*it)->idc());

                return (*it);
            }
        }
    }
    else
    {
        std::string next_idc = _hasite_conf.get_next_idc(_server_name.c_str());
        if (next_idc == "")
        {
            trpc_debug_log("already reach the last IDC, do not switch");
            trpc_write_statslog("already reach the last IDC, do not switch\n");
            _need_switch = false;

            for (std::vector<CApiBase *>::iterator it = _api_pool->begin(); it != _api_pool->end(); ++it)
            {
                if ((*it)->is_free())
                {
                    (*it)->set_used();

                    //trpc_debug_log("FIND GET_API_INDEX:%d, IDC:%s\n", (*it)->index(), (*it)->idc());

                    return (*it);
                }
            }
        }
        else
        {
            if (_switch_state == EM_SWITCH_ONGOING)
            {
                trpc_debug_log("switching IDC has not finished, please wait!");
                trpc_write_statslog("switching IDC has not finished, please wait!\n");
                THROW(CServerException, ERR_SERVER_BUSY, "Last switch has not finished, please wait!");
            }
            else if (_switch_state == EM_SWITCH_PRIOR || _switch_state == EM_SWITCH_FINISHED)
            {
                _switch_state = EM_SWITCH_ONGOING;
                trpc_write_statslog("SWITCH_ONGOING\n");
            }

            int index = (++_switch_factor) % 2;
            _api_pool = &_pool[index];
            _pool_size = _api_pool->size();
            trpc_debug_log("new pool index == %d, new pool size == %d", index,  _pool_size);
            _hasite_conf.set_current_idc(_server_name.c_str());
            _need_switch = false;
            trpc_debug_log("switch completed, current IDC=%s\n", next_idc.c_str());
            trpc_write_statslog("switch completed, current IDC=%s\n", next_idc.c_str());

            /*
            if (last_idc != _hasite_conf.default_idc)
            {
                _hasite_conf.set_invalid_forever(last_idc.c_str(), _server_name.c_str(), _ip_addr.c_str(), _port);
            }
            */
        }
    }

    // 检查，如果没有超过最大的值，创建一个新的API，并加入_api_pool中
    if (_pool_size < _max_conn_num)
    {
        return _add_api(user, passwd);
    }
    else
    {
        // 无可用节点
        THROW(CServerException, ERR_SERVER_BUSY, "server_site: %s no free conn max: %d, now: %d",
              _server_name.c_str(), _max_conn_num, _pool_size);
    }

    // never go here
    return NULL;
}
CApiBase * CApiPool::_add_api(const char * user, const char * passwd)
{
    // 获取ip和端口
    std::string ip;
    int port;
    int protocol;
    CApiBase * api = NULL;

    std::string idc = _need_switch ? 
                       _hasite_conf.get_next_idc(_server_name.c_str()) 
                      : _hasite_conf.get_current_idc(_server_name.c_str());
    _hasite_conf.get(idc.c_str(), _server_name.c_str(), ip, port, protocol);

    _ip_addr = ip;
    _port = port;

    trpc_debug_log("get ip success, idc:%s, server_name: %s, ip: %s, port: %d, protocol: %d",
                    idc.c_str(), _server_name.c_str(), ip.c_str(), port, protocol);

    api = _create_api(idc, ip, port, protocol, user, passwd);
    if (api == NULL)
    {
        THROW(CServerException, ERR_SYS_UNKNOWN, "create api error idc:%s, ip: %s, port: %d, protocol: %d",
                        idc.c_str(), ip.c_str(), port, protocol);
    }

    // 加入到池中
    api->set_used();
    _api_pool->push_back(api);
    ++_pool_size;

    // test assert
    if (_pool_size != (int)_api_pool->size())
    {
        THROW(CTrpcException, ERR_SYS_UNKNOWN, "_poolsize: %d != api_pool: %d",
                _pool_size, (int)_api_pool->size());
    }

    //trpc_debug_log("GET_API_INDEX:%d, IDC:%s\n", api->index(), api->idc());

    return api;
}

void CApiPool::_release(CApiBase * api, bool force_close)
{
    bool auto_close = false;

    // 如果index大于必须长存的链接数量，自动关闭
    if (api->index() >= _conn_num)
    {
        auto_close = true;
    }
    else
    {
        auto_close = force_close;
    }
    
    //(*_api_pool)[api->index()]->set_free(auto_close);
    api->set_free(auto_close);
}

CApiBase * 
CApiPool::_create_api(const std::string& idc,
                     const std::string& ip,
                     int                port,
                     int                protocol,
                     const char         *user,
                     const char         *passwd)
{
    CApiBase * api = NULL;

    if (protocol == EM_ST_RELAY)
    {
        api = new CRelayApi(idc.c_str(), ip.c_str(), port, _pool_size, _time_out);
    }
    else if (protocol == EM_ST_MIDDLE)
    {
        api = new CMiddleApi(idc.c_str(), ip.c_str(), port, user, passwd, _pool_size, _time_out);
    }
    else
    {
        THROW(CConfigException, ERR_CONF, "idc:%s, server_name: %s, protocol error: %d", 
                    idc.c_str(), _server_name.c_str(), protocol);
    }

    return api;
}
