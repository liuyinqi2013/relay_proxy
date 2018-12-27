#include    "hasite_conf.h"
#include    "trpc_sys_exception.h"
#include    "trpc_comm_func.h"
#include    <algorithm>

const char * CHaSiteConf::default_idc = "default_idc";

/**
 * 取子节点
 */
const TiXmlNode*  NODE_CHILD(const TiXmlNode* pNode, const char* child=NULL)
{
    const TiXmlNode* pChild = child ?  pNode->FirstChildElement(child) : pNode->FirstChildElement();
    if (!pChild)
    {
        if (child)
        {
            THROW(CConfigException, ERR_CONF, "Get child node failed: %s", child);
        }
    }

    return pChild;
}

/**
 * 取相邻节点
 */
const TiXmlNode* NODE_NEXT(const TiXmlNode* pNode)
{
    return pNode->NextSiblingElement();
}

/**
 * 取结点名称
 */
std::string NODE_NAME(const TiXmlNode* pNode)
{   
    return pNode->Value() ? pNode->Value() : "";
}

/**
 * 取结点值
 */
std::string NODE_STR_VALUE(const TiXmlNode* pNode)
{
    return pNode->ToElement()->GetText() ? pNode->ToElement()->GetText() : "";
}

/**
 * 取结点值
 */
int NODE_INT_VALUE(const TiXmlNode* pNode)
{
    return pNode->ToElement()->GetText() ? atoi(pNode->ToElement()->GetText()) : 0;
}

/**
 * 取结点值
 */
long long NODE_LONG_VALUE(const TiXmlNode* pNode)
{
    return pNode->ToElement()->GetText() ? atoll(pNode->ToElement()->GetText()) : 0;
}

/**
 * 取结点属性
 */
std::string NODE_ATTR(const TiXmlNode* pNode, const char* attribute)
{
    if (pNode->ToElement() == NULL)
    {
        THROW(CConfigException, ERR_CONF, "ToElement fail: Get child node attr error: %s, node type: %d, %d", attribute, pNode->Type(), TiXmlNode::COMMENT);
    }
    
    if (pNode->ToElement()->Attribute(attribute) == NULL)
    {
        THROW(CConfigException, ERR_CONF, "Get child node attr error: %s", attribute);
    }
    
    return pNode->ToElement()->Attribute(attribute);
}

std::string NODE_ATTR_WITH_DEFAULT(const TiXmlNode* pNode, const char* attribute, const std::string& default_value)
{
    if (pNode->ToElement() == NULL)
    {
        THROW(CConfigException, ERR_CONF, "ToElement fail: Get child node attr error: %s, node type: %d, %d", attribute, pNode->Type(), TiXmlNode::COMMENT);
    }
    
    if (pNode->ToElement()->Attribute(attribute) == NULL)
    {
        return default_value;
    }
    
    return pNode->ToElement()->Attribute(attribute);
}

/////////////////////////////////////////////////////

CHaSiteConf::CHaSiteConf()
{
}

CHaSiteConf::~CHaSiteConf()
{
}

// 根据配置文件中的字符串协议类型, 修改为int型的协议类型, 以方便兼容旧配置
int CHaSiteConf::get_protocol_type(const char * protocol_type)
{
    int protocol = -1;
    
    if (strcmp(protocol_type, "tcp") == 0)
    {
        // 以前的tcp协议为middle协议,兼容旧配置
        protocol = EM_ST_MIDDLE;
    }
    else if (strcmp(protocol_type, "relay") == 0)
    {
        // relay协议
        protocol = EM_ST_RELAY;
    }
    else
    {
        // 报错
        THROW(CConfigException, ERR_CONF, "protocol_type error: %s", protocol_type);
    }

    return protocol;
}

// 根据服务站点, 查询可用ip
void CHaSiteConf::get(const char *idc, const char * server_name, std::string& ip, int& port, int& protocol_type)
{
    CAutoRLock l(_mutex);

    _get(idc, server_name, ip, port, protocol_type);
}

std::string
CHaSiteConf::get_current_idc(const char * server_name)
{
    CAutoRLock l(_mutex);

    const ST_HaSiteInfo& hasite_info = _hasite_conf[server_name];
    return hasite_info.current_idc;
}

void
CHaSiteConf::set_current_idc(const char * server_name)
{
    CAutoWLock l(_mutex);

    ST_HaSiteInfo& hasite_info = _hasite_conf[server_name];
    
    std::vector<std::string>::iterator iend = hasite_info.switch_order_list.end();
    std::vector<std::string>::iterator iter = std::find(hasite_info.switch_order_list.begin(),
                                                        iend,
                                                        hasite_info.current_idc);
    if (iter != iend && ++iter != iend)
    {
        hasite_info.current_idc = *iter;
    } 
}

std::string
CHaSiteConf::get_next_idc(const char * server_name)
{
    CAutoRLock l(_mutex);

    const ST_HaSiteInfo& hasite_info = _hasite_conf[server_name];
    std::vector<std::string>::const_iterator iend = hasite_info.switch_order_list.end();
    std::vector<std::string>::const_iterator iter = std::find(hasite_info.switch_order_list.begin(), 
                                                              iend,
                                                              hasite_info.current_idc);

    std::string next_idc;
    if (iter != iend && ++iter != iend)
    {
        next_idc = *iter;
    }

    return next_idc;
}

void CHaSiteConf::set_valid(const char *idc, const char * server_name, const char * ip, const int port)
{
    CAutoWLock l(_mutex);

    _set_valid(idc, server_name, ip, port);
}

void CHaSiteConf::set_invalid(const char *idc, const char * server_name, const char * ip, const int port)
{
    CAutoWLock l(_mutex);

    _set_invalid(idc, server_name, ip, port, false);
}

void CHaSiteConf::set_invalid_forever(const char *idc, const char * server_name, const char * ip, const int port)
{
    CAutoWLock l(_mutex);

    _set_invalid(idc, server_name, ip, port, true);
}

void CHaSiteConf::read_conf(const char * conf_file)
{
    // 加写锁
    CAutoWLock l(_mutex);

    _get_hasite_conf(conf_file);
}

void CHaSiteConf::reload(const char * conf_file)
{
    // 暂时直接清除旧配置,重新读取配置文件
    CAutoWLock l(_mutex);

    _hasite_conf.clear();

    _get_hasite_conf(conf_file);
}

void CHaSiteConf::_get(const char *idc, const char * server_name, std::string& ip, int& port, int& protocol_type)
{
    const ST_HaSiteInfo& hasite_info = _hasite_conf[server_name];

    bool is_find = false;
    
    // 找到一个可用的结点
    int num = hasite_info.hasite_node_num;

    trpc_debug_log("hasite_node_num=%d\n", num);
    // 设置协议
    protocol_type = hasite_info.protocol_type;
    
    for (int i = 0; i < num; ++i)
    {
        const ST_HaSiteNodeInfo& node = hasite_info.hasite_node[i];
        trpc_debug_log("node[%d]:ip:%s, port:%d, valid:%d", i, node.ip, node.port, node.is_valid);

        if (strncmp(node.idc, CHaSiteConf::default_idc, strlen(CHaSiteConf::default_idc)) == 0
            || strncmp(node.idc, idc, sizeof(node.idc)) == 0)
        {
            if (node.is_forever_invalid)  break;
 
            ip = node.ip;
            port = node.port;

            is_find = true;
            break;
        }
    }        

    if (!is_find)
    {
        trpc_debug_log("NO HASH NODE FOUND\n");
        // 没找到可用ip, 暂时先报错,后续考虑看是采用默认第一个还是继续报错
        THROW(CServerException, ERR_SERVER_MAINTAIN, 
                    "server: %s, no site can use", server_name);
    }
}

void CHaSiteConf::_set_valid(const char *idc, const char * server_name, const char * ip, const int port)
{
    ST_HaSiteInfo& hasite_info = _hasite_conf[server_name];

    // 找到当前ip匹配的信息
    int num = hasite_info.hasite_node_num;
    
    bool is_find = false;

    for (int i = 0; i < num; ++i)
    {
        ST_HaSiteNodeInfo& node = hasite_info.hasite_node[i];

        if (strncmp(ip, node.ip, sizeof(node.ip)) == 0 
            && port == node.port
            && strncmp(idc, node.idc, sizeof(node.idc)) == 0)
        {
            if (node.is_forever_invalid)
            {
                 THROW(CServerException, ERR_SERVER_MAINTAIN, 
                       "server: %s, ip: %s, port: %d, forever invalid, can't be set valid", server_name, ip, port);
            }

            // ip和端口相等, 设置为可用, 不管以前状态
            node.is_valid = true;

            is_find = true;

            break;
        }
        else
        {
            // 继续查找对应的ip, port
        }
    }

    // 对应的ip在该server的hasite里面没有找到,
    // 应该是动态加载了新的ip,报错,让外面关闭这个链接,重新获取ip
    if (!is_find)
    {
        // 没找到可用ip, 暂时先报错,后续考虑看是采用默认第一个还是继续报错
        THROW(CServerException, ERR_SERVER_MAINTAIN, 
                    "server: %s, ip: %s, port: %d, not in hasite, reconnect it", 
                    server_name, ip, port);
    }
}

void CHaSiteConf::_set_invalid(const char *idc, const char * server_name, const char * ip, const int port, bool forever)
{
    ST_HaSiteInfo& hasite_info = _hasite_conf[server_name];

    // 找到当前ip匹配的信息
    int num = hasite_info.hasite_node_num;
    
    bool is_find = false;

    for (int i = 0; i < num; ++i)
    {
        ST_HaSiteNodeInfo& node = hasite_info.hasite_node[i];

        if (strncmp(ip, node.ip, sizeof(node.ip)) == 0 
            && port == node.port
            && strncmp(idc, node.idc, sizeof(node.idc)) == 0) 
        {
            trpc_debug_log("set_invalid, IP:%s, PORT:%d\n", ip, port);
            // ip和端口相等, 设置为不可用, 不管以前状态
            node.is_valid = false;
            node.invalid_time = time(0);

            if (forever)
            {
                node.is_forever_invalid = true;
            }

            is_find = true;

            break;
        }
        else
        {
            // 继续查找对应的ip, port
        }
    }

    // 对应的ip在该server的hasite里面没有找到,
    // 应该是动态加载了新的ip,报错,让外面关闭这个链接,重新获取ip
    if (!is_find)
    {
        // 没找到可用ip, 暂时先报错,后续考虑看是采用默认第一个还是继续报错
        THROW(CServerException, ERR_SERVER_MAINTAIN, 
                    "server: %s, ip: %s, port: %d, not in hasite, reconnect it", 
                    server_name, ip, port);
    }
}

void CHaSiteConf::_get_hasite_conf(const char * conf_file)
{
    // 获取配置信息
    TiXmlDocument  doc(conf_file);	
    if(!doc.LoadFile())
    {
        THROW(CConfigException, ERR_CONF, "Load xml config file: %s error: %s", conf_file, doc.ErrorDesc());
    }


    const TiXmlNode* site_node = doc.FirstChildElement("hasite");

    if (site_node == NULL)
    {
        THROW(CConfigException, ERR_CONF, "Load xml config file error path: /hasite not found");
    }
    
    site_node = site_node->FirstChildElement("site");
    if (site_node == NULL)
    {
        THROW(CConfigException, ERR_CONF, "Load xml config file error path: /hasite/site not found");
    }
    
    for(; site_node; site_node=NODE_NEXT(site_node))
    {
        // 获取每个site信息
        _get_site_info(site_node);
    }
}

void CHaSiteConf::_get_site_info(const TiXmlNode* site_node)
{
    std::string server_name = NODE_ATTR(site_node, "server_name");

    ST_HaSiteInfo& site_info = _hasite_conf[server_name];
    
    site_info.estop_time = atoi(NODE_ATTR(site_node, "estop_time").c_str());
    site_info.protocol_type = get_protocol_type(NODE_ATTR(site_node, "protocol").c_str());
    std::string switch_order = NODE_ATTR_WITH_DEFAULT(site_node, "switch_order", "default_switch_order");
    site_info.switch_order_list.clear();
    if (switch_order != "default_switch_order")
    {
        split(switch_order.c_str(), ":", MAX_HASITENODE, site_info.switch_order_list);
    }

    site_info.hasite_node_num = 0;

    const TiXmlNode* server_node = site_node->FirstChildElement("sitenode");
    for(; server_node; server_node = NODE_NEXT(server_node))
    {
        // 获取每个sitenode信息
        ST_HaSiteNodeInfo& node = site_info.hasite_node[site_info.hasite_node_num];

        node.invalid_time = 0;
        node.valid_time = 0;
        node.port = atoi(NODE_ATTR(server_node, "port").c_str());
        node.is_valid = true;
        node.is_forever_invalid = false;
        
        strncpy(node.ip, NODE_ATTR(server_node, "ip").c_str(), sizeof(node.ip) - 1);
        node.ip[sizeof(node.ip) - 1] = '\0';

        strncpy(node.idc, NODE_ATTR_WITH_DEFAULT(server_node, "idc", CHaSiteConf::default_idc).c_str(), sizeof(node.idc) - 1);

        if (switch_order == "default_switch_order")
        {
            site_info.switch_order_list.push_back(node.idc);
        }

        ++site_info.hasite_node_num;

        trpc_debug_log("site_name: %s, ip: %s, port: %d, protocol_type: %d", 
                server_name.c_str(), node.ip, node.port, site_info.protocol_type );
    }

    if (site_info.hasite_node_num == 0)
    {
        // 站点数量为0, 为系统错误, 报错退出
        THROW(CConfigException, ERR_CONF, "site_info's node_num is 0");
    }

    if (site_info.hasite_node_num != (int)site_info.switch_order_list.size())
    {
        THROW(CConfigException, ERR_CONF, "switch_order attribute idc num != site_node_num");
    }

    site_info.current_idc = site_info.switch_order_list[0];

    trpc_debug_log("server_name=========[%s]", server_name.c_str());
    trpc_debug_log("switch_order_list:");
    for (size_t i = 0; i < site_info.switch_order_list.size(); ++i)
    {
        trpc_debug_log("    switch order %d, %s", (int)i, site_info.switch_order_list[i].c_str());
    }

    trpc_debug_log("current_idc====[%s]", site_info.current_idc.c_str());
}
