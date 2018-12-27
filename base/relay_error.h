#ifndef     __RELAY_ERROR_H__
#define     __RELAY_ERROR_H__

// �����ž�relay������ -- �󲿷�Ӧ�ö����ᱨ��ȥ��,���ִ���������ǰ��
enum ErrorType {
    // ����ҵ��ƽ̨����1000 - 9999  
    
     // ֧�����ر���10000-11999
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

    // relay����ip���ƺ�Ĵ�����
    SysError = 10050, RecvClose, AdminCmdError,

    // added by verminniu
    // ������
    PayNumOutOfRange = 10100,
    
    // �ӽ��ܴ��� 12000-12999
    
    // ���п۷Ѵ���13000-13999
    BankResultError = 13001,        // ���п۷�ʧ��
    BankAuthError = 13002,         // ȡ����֧��url ʧ��
    ParseBankResultError = 13003,     // �������п۷ѽ��ʧ��

    // JavaSvr��ش��� 14000-14999 

    // C2c ͬ������ 15000 - 15999
    SynC2cResultError = 15000, 
    SynC2cApplyAcctError = 15001, // �û�ע��ͬ������

    // session ��������
    SessionError_CreateSession = 16000, 
    SessionError_GetSession = 16001, 
    SessionError_StatusInvalid = 16002, 
    SessionError_DataInvalid = 16003, 
    SessionError_InvalidVerifyType = 16004, 

    // ���ݿ��������
    DBERR_Init = 21001,    // ���ݿ����ӳ�ʼ��ʧ��
    DBERR_ExecQuery = 21002,      // ���ݿ�ִ��SQL����
    DBERR_ExecEmpty = 21003,     //  ���ݿ��ѯ���Ϊ�մ���
    DBERR_StatusError = 21004    //  ���ݼ�¼״̬����ȷ
};

#endif

