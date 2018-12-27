/*
功能: 
       1.  trpc远程过程调用的包格式类定义

Created by Song, 2003-02
Change list:

*/

#include <stdio.h>

#include "trpc_package.h"

void
CTrpcPackage::Print()
{
    fprintf(stdout, "iLen: %d\n", GetLen());
    fprintf(stdout, "iVersion: %d\n", GetVersion());
    fprintf(stdout, "iSeqNo: %d\n", GetSeqNo());
    fprintf(stdout, "iMsgType: %d\n", GetMsgType());
    fprintf(stdout, "sServiceName: [%.*s]\n",
            (int) sizeof(_stMsg.sServiceName), GetServiceName());
    fprintf(stdout, "iResult: %d\n", GetResult());
    fprintf(stdout, "iDataLen: %d\n", GetDataLen());

    fprintf(stdout, "\n");
}

int
CTrpcPackage::Encode(void *buf, int &iBufSize, int iMaxBufSize)
{
    _stMsg.iLen = PackSize(_stMsg.iDataLen);
    iBufSize = _stMsg.iLen;

    if (iBufSize > iMaxBufSize) {
        sprintf(_error_text, "iBufSize==%d, but iMaxBufSize==%d",
                iBufSize, iMaxBufSize);
        return -1;
    }

    Trpc_Package_Net_T *pPack = (Trpc_Package_Net_T *) buf;

    _stMsg.iLen = iBufSize;

    // 包开始分隔符
    pPack->cStx = CTrpcPackage::PACK_STX;
    TRPC_PUT_INT32(pPack->sLen, _stMsg.iLen);
    TRPC_PUT_INT32(pPack->sVersion, _stMsg.iVersion);
    TRPC_PUT_INT32(pPack->sSeqNo, _stMsg.iSeqNo);
    TRPC_PUT_INT32(pPack->sMsgType, _stMsg.iMsgType);
    TRPC_PUT_STR(pPack->sServiceName, _stMsg.sServiceName,
                 sizeof(pPack->sServiceName));
    TRPC_PUT_INT32(pPack->sResult, _stMsg.iResult);
    TRPC_PUT_INT32(pPack->sDataLen, _stMsg.iDataLen);
    if (_stMsg.iDataLen > 0) {
        memcpy(pPack->sData, _stMsg.sData, _stMsg.iDataLen);
    }
    // 包结束分隔符
    *(pPack->sData + _stMsg.iDataLen) = PACK_ETX;

    return 0;
}

int
CTrpcPackage::Decode(const void *buf, int iBufSize)
{
    const Trpc_Package_Net_T *pSrc = (const Trpc_Package_Net_T *) buf;

    _stMsg.cStx = pSrc->cStx;
    TRPC_GET_INT32(_stMsg.iLen, pSrc->sLen);
    TRPC_GET_INT32(_stMsg.iVersion, pSrc->sVersion);
    TRPC_GET_INT32(_stMsg.iSeqNo, pSrc->sSeqNo);
    TRPC_GET_INT32(_stMsg.iMsgType, pSrc->sMsgType);
    TRPC_GET_STR(_stMsg.sServiceName, pSrc->sServiceName,
                 sizeof(_stMsg.sServiceName));
    TRPC_GET_INT32(_stMsg.iResult, pSrc->sResult);
    TRPC_GET_INT32(_stMsg.iDataLen, pSrc->sDataLen);
    _stMsg.sData = pSrc->sData;

    // 判断数据长度是否正确，要把Etx减去
    int iDataLen =
        (const char *) pSrc + iBufSize - (const char *) pSrc->sData - 1;
    if (_stMsg.iDataLen != iDataLen) {
        sprintf(_error_text, "_stMsg.iDataLen==%d, but iDataLen==%d",
                _stMsg.iDataLen, iDataLen);
        return -1;
    }

    return 0;
}
