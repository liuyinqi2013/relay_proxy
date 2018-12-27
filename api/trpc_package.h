#ifndef __C_TRPC_PACKAGE_H
#define __C_TRPC_PACKAGE_H

/*
功能: 
       1.  trpc远程过程调用的包格式类

Created by Song, 2003-02
Change list:

*/

#include <string.h>
#include <netinet/in.h>

struct Trpc_Login_Body_T
{
    char sUserName[32];
    char sPasswd[32];
};

class CTrpcPackage
{
public:
    CTrpcPackage(){}
    ~CTrpcPackage(){}

    const char * get_error_text() const {return _error_text;}

public:
    int Encode(void * buf, int& iBufSize, int iMaxBufSize);
    
    int Decode(const void * buf, int iBufSize);

    void Print();

    static int MinPackSize(){return MIN_PACK_SIZE;}
    static int PackSize(int iDataLen)
    {
        //  减去sData，加上Etx
        return iDataLen + sizeof(Trpc_Package_Net_T) - 1 + 1;
    }

public:
    void SetVersion(int iVersion){_stMsg.iVersion = iVersion;}
    void SetSeqNo(int iSeqNo){_stMsg.iSeqNo = iSeqNo;}
    void SetMsgType(int iMsgType){_stMsg.iMsgType = iMsgType;}
    void SetServiceName(const char * sServiceName)
    {
        strncpy(_stMsg.sServiceName, sServiceName, sizeof(_stMsg.sServiceName));
        _stMsg.sServiceName[sizeof(_stMsg.sServiceName) - 1] = '\0';
    }

    void SetResult(int iResult){_stMsg.iResult = iResult;}
    void SetDataLen(int iDataLen){_stMsg.iDataLen = iDataLen;}
    void SetData(const void * sData){_stMsg.sData = (const char *)sData;}

    int GetLen() const {return _stMsg.iLen;}
    int GetVersion() const {return _stMsg.iVersion;}
    int GetSeqNo() const {return _stMsg.iSeqNo;}
    int GetMsgType() const {return _stMsg.iMsgType;}
    const char * GetServiceName() const
        {return _stMsg.sServiceName;}
    int GetResult() const {return _stMsg.iResult;}
    int GetDataLen() const {return _stMsg.iDataLen;}
    const char * GetData() const {return _stMsg.sData;}

public:
    static const char PACK_STX = 0x02;
    static const char PACK_ETX = 0x03;

    // MsgType
    static const int TYPE_LOGIN_REQUEST = 1;
    static const int TYPE_LOGIN_RESPOND = 2;
    static const int TYPE_CALL_REQUEST = 3;
    static const int TYPE_CALL_RESPOND = 4;

    // 异步调用返回包用TYPE_CALL_RESPOND
    static const int TYPE_ASYN_CALL_REQUEST = 5;

    // Version
    static const int VERSION = 1000000;

private:
    struct Trpc_Package_T
    {
        char cStx;  // char
        int iLen;
        int iVersion;
        int iSeqNo;
        int iMsgType;
        char sServiceName[32];  // string
        char sReserve[32];
        int iResult;
        int iDataLen;
        const char * sData;
    };

    // Trpc_Package_T的网络传送包结构
    struct Trpc_Package_Net_T
    {
        char cStx;  // char
        char sLen[4];  // int
        char sVersion[4];  // int
        char sSeqNo[4];  // int
        char sMsgType[4];  // int
        char sServiceName[32];  // string
        char sResult[4];  // int
        char sReserve[32]; // 保留
        char sDataLen[4];  // int
        char sData[1];  // 实际不止1个字节 
        // cEtx  没有在这里显示
    };

    // 减去sData，加上Etx
    static const int MIN_PACK_SIZE = sizeof(Trpc_Package_Net_T) - 1 + 1;

private:
    Trpc_Package_T _stMsg;
    char _error_text[256];
};

inline void TRPC_GET_STR(char * data, const void * src_buf, size_t len)
{
    strncpy(data, (const char *)src_buf, len);
    data[len - 1] = '\0';
}

inline void TRPC_GET_INT32(int32_t& data, const void * src_buf)
{
    memcpy(&data, src_buf, sizeof(int32_t));
    data = ntohl(data);
}

inline void TRPC_GET_INT16(int16_t& data, const void * src_buf)
{
    memcpy(&data, src_buf, sizeof(int16_t));
    data = ntohs(data);
}

inline void TRPC_GET_CHAR(char& data, const void * src_buf)
{
    data = *(char *)src_buf;
}


inline void TRPC_PUT_STR(void * buf, const char * data, size_t len)
{
    strncpy((char *)buf, data, len);
}

inline void TRPC_PUT_INT32(void * buf, int32_t data)
{
    data  = htonl(data);
    memcpy(buf, &data, sizeof(int32_t));
}

inline void TRPC_PUT_INT16(void * buf, int16_t data)
{
    data = htons(data);
    memcpy(buf, &data, sizeof(int16_t));
}

inline void TRPC_PUT_CHAR(void * buf, char data)
{
    *(char *)buf = data;
}

/*
static void
PrintHex(const void *p, size_t sLen)
{
    size_t i;

    for (i = 0; i < sLen; i++)
    {
        if ((i != 0) && (i % 8 == 0))
        {
            printf("  ");
        }

        if ((i != 0) && (i % 16 == 0))
        {
            printf("\n");
        }

        printf("%02x ", (unsigned char) (*((char *) p + i)));
    }

    printf("\n");
}
*/

#endif
