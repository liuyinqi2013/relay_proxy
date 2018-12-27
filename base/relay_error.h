#ifndef     __RELAY_ERROR_H__
#define     __RELAY_ERROR_H__

// 这里存放旧relay错误码 -- 大部分应该都不会报出去了,部分错误沿用以前的
enum ErrorType {
    // 核心业务平台报错1000 - 9999  
    
     // 支付网关报错10000-11999
    InternalError = 10001, RequestTypeInvalid, InvalidAccess, ParamNotExist, 
    VersionInvalid = 10005, SocketError, EncryptError, DecryptError, TrpcInfoPoolInvalid, 
    CoreCenterError = 10010, CoreParamError, InvalidBank, GetPayUrlError, ParseBankValueError, 
    TransactionIdError = 10015, GenerateSignError, GenerateCertError, JavaSvrTimeOut, JavaSvrParaError, 
    WriteWaterError = 10020, InvalidTradeStatus, GetBillnoError, SpidHasNotSecret, PayResultPseudo, 
    SystemBusy = 10025, AuthSvrTimeOut, AuthSvrParaError, AuthSvrError, PayMentNotExist, 
    ParamInvalid = 10030, SynUrlError,  SynTimeOut, SynError, LoginUinIsNotQQ, 
    CacheRequestTypeInvalid = 10035, CacheQueryError, CacheUpdateError, NotSupportProtocol, BindPayError, 
    CurTypeError = 10040, BindPayReslutVerifyError, BindPayExceedQuota, NotSupportBindBank, ReqNotMatchReturn, 
    BindRecordNotExist = 10045, TooManyBindRecords, BindRecordNotAuth, ExceedFreqLimit, TooManyChilds4OneReqType, 

    // relay加入ip限制后的错误码
    SysError = 10050, RecvClose, AdminCmdError,

    // added by verminniu
    // 金额过大
    PayNumOutOfRange = 10100,
    
    // 加解密错误 12000-12999
    
    // 银行扣费错误13000-13999
    BankResultError = 13001,        // 银行扣费失败
    BankAuthError = 13002,         // 取银行支付url 失败
    ParseBankResultError = 13003,     // 解析银行扣费结果失败

    // JavaSvr相关错误 14000-14999 

    // C2c 同步错误 15000 - 15999
    SynC2cResultError = 15000, 
    SynC2cApplyAcctError = 15001, // 用户注册同步错误

    // session 操作错误
    SessionError_CreateSession = 16000, 
    SessionError_GetSession = 16001, 
    SessionError_StatusInvalid = 16002, 
    SessionError_DataInvalid = 16003, 
    SessionError_InvalidVerifyType = 16004, 

    // 数据库操作错误
    DBERR_Init = 21001,    // 数据库连接初始化失败
    DBERR_ExecQuery = 21002,      // 数据库执行SQL错误
    DBERR_ExecEmpty = 21003,     //  数据库查询结果为空错误
    DBERR_StatusError = 21004    //  数据记录状态不正确
};

#endif

