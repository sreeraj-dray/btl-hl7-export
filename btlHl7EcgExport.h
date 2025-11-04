#ifndef BTL_HL7_ECG_EXPORT_H
#define BTL_HL7_ECG_EXPORT_H 1

//Feature enable macros
#define  BTLHL7EXP_SSL_EN 1

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
//#if (_WIN32) || (_WIN64)
#define BTL_HL7_FOR_WINDOWS 1
#endif
#ifdef BTL_HL7_FOR_WINDOWS
#define _CRT_SECURE_NO_WARNINGS
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#pragma comment (lib, "Ws2_32.lib")
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>  // WSAPoll

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
    //linux compatibility macros
#define BtlHl7ExpPollFd_t WSAPOLLFD
#define btlHl7ExpPollFn WSAPoll
typedef int socklen_t;
#define strncasecmp(a, b, c) _strnicmp(a, b, c)

#define strtok_r(a, b, c) strtok_s(a, b, c)

#else
//includes and macros for Linux
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
//#include <sys/select.h>
#include <pthread.h>
#include <time.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define BtlHl7ExpPollFd_t struct pollfd
#define btlHl7ExpPollFn poll

#define Sleep(a) usleep(a*1000)
#define strncpy_s(a,b,c,d) strncpy(a,c,b)
#define closesocket(a) close(a)
#define strtok_s strtok_r

#endif

#ifdef BTLHL7EXP_SSL_EN
    #include <openssl/ssl.h>
    #include <openssl/err.h>
    #include <openssl/opensslv.h>
    #include <openssl/x509_vfy.h>
#endif
#include "btlHl7ExpDebug.h"
#include "btlHl7ExpParseHl7.h"

//====================================================================
//Default strings that are used in transmitted HL7 messages

    //Default Sending Application field value in MSH segment
#define BTL_HL7_EXP_SENDING_APPLN_STR "BTL_ECG2"   //Sending Application (default value)
    //Version to use when user has not set it/ no protocol extra data avail (locally generated examination)
#define BTL_HL7_EXP_DEFAULT_HL7_VER_STR "2.2" //do not change this for better interoperability with all servers
#define BTL_HL7_EXP_OBX3_TAG_DEFAULT_STR "PDF^ReportFileData"  //default tag in OBX.3 field for pdf file transfer

//====================================================================

#define BTLHL7_MAX_PATH_LEN             (512)
#define BTLHL7_MAX_EXTRA_DATA_LEN       (65536)
#define BTLHL7_MAX_HL7_DATA_LEN         (65536)
#define BTL_HL7_MAX_HL7_ACK_MSG_LEN     (2048)

#define BTLHL7_SERVER_IP_MAX_LEN                (16)
#define BTLHL7_EXPORT_SERVER_TIMEOUT_SEC        (3)
#define BTLHL7_HOST_NAME_BUF_LEN                (128)


#define BTL_HL7_NAME_BUF_SIZE           (128)  //Sending Application, Facility etc
#define BTL_HL7EXP_VERSION_BUF_SIZE     (8)

// STATUS OF EXPORT OPERATION
#define BTLHL7EXP_STATUS_NO_ERROR       (0)
#define BTLHL7EXP_STATUS_CONNECTING     (1)
#define BTLHL7EXP_STATUS_INPROGRESS     (2)
#define BTLHL7EXP_STATUS_ACK_WAIT       (3)

        //Error Status defines
#define BTLHL7EXP_STATUS_ERR_SOCKET         (-1)
#define BTLHL7EXP_STATUS_ERR_CONNECT        (-2)  //remote server - no response/reject
#define BTLHL7EXP_STATUS_ERR_NETWORK        (-3)
#define BTLHL7EXP_STATUS_ERR_ACK_TIMEOUT    (-4)
#define BTLHL7EXP_STATUS_ERR_ACK            (-5)
#define BTLHL7EXP_STATUS_INPUT_ERR          (-6)
#define BTLHL7EXP_STATUS_EXDATA_XML_ERR     (-7)
#define BTLHL7EXP_STATUS_PROCESSING_ERR     (-8)
#define BTLHL7EXP_STATUS_PDF_FILE_OPEN_ERR  (-9)
#define BTLHL7EXP_STATUS_ERR_IPADDR         (-10)
#define BTLHL7EXP_STATUS_ERR_TCP_PORT       (-11)
#define BTLHL7EXP_STATUS_ERR_WSA_INIT       (-12)  //only in windows
#define BTLHL7EXP_STATUS_ERR_SOCKET_CREATE  (-13)
#define BTLHL7EXP_STATUS_ERR_SRV_CONNECT    (-14)  //failure of connect call
#define BTLHL7EXP_STATUS_ERR_POLL_FAIL      (-15)
#define BTLHL7EXP_STATUS_ERR_POLL_FAIL2     (-16)
#define BTLHL7EXP_STATUS_ERR_SOCKET_SEND    (-17)
#define BTLHL7EXP_STATUS_ERR_SOCKET_RECV    (-18)
#define BTLHL7EXP_STATUS_ERR_USER_ABORT     (-19)
#define BTLHL7EXP_STATUS_XMLNG_ERR1          (-20)
#define BTLHL7EXP_STATUS_XMLNG_ERR2          (-21)
#define BTLHL7EXP_STATUS_XMLNG_NO_PAT_INFO  (-22)
#define BTLHL7EXP_STATUS_XMLNG_NO_PAT_INFO2 (-23)
#define BTLHL7EXP_STATUS_ERR_CFG_XML_READ   (-24)
#define BTLHL7EXP_STATUS_ERR_CFG_XML_ROOTNODE   (-25)
#define BTLHL7EXP_STATUS_ERR_SOCKET_NONBLOCK    (-26)
    //SSL errors
#define BTLHL7EXP_STATUS_ERR_SSL_CTX_CR                 (-51)
#define BTLHL7EXP_STATUS_ERR_SSL_VER_CFG                (-52)
#define BTLHL7EXP_STATUS_ERR_SSL_HOST_MATCH_STR         (-53)
#define BTLHL7EXP_STATUS_ERR_SSL_GET0_PARAM             (-54)
#define BTLHL7EXP_STATUS_ERR_SSL_SET1_IP_ASC            (-55)
#define BTLHL7EXP_STATUS_ERR_SSL_SET1_HOST              (-56)
#define BTLHL7EXP_STATUS_ERR_INVALID_VERIFY_MODE        (-57)
#define BTLHL7EXP_STATUS_ERR_SSL_NO_PEER_CA_FILE        (-58)
#define BTLHL7EXP_STATUS_ERR_SSL_LD_VERIFY_LOCS         (-59)
#define BTLHL7EXP_STATUS_ERR_SSL_OWN_CERT_KEY_FILE_S    (-60)
#define BTLHL7EXP_STATUS_ERR_SSL_NEW_CREATE             (-61)
#define BTLHL7EXP_STATUS_ERR_SSL_SEND                   (-62)

#define BTLHL7EXP_STATUS_BUSY               (-100)

#define BTLHL7EXP_TCP_NOT_INIT  (0)
#define BTLHL7EXP_INIT_DONE     (1)

// User command types
#define BTLHL7_EXP_USER_CMD_EXPORT       (0)  // Normal PDF export
#define BTLHL7_EXP_USER_CMD_SRV_STATUS   (1)  // Only check server connectivity
#define BTLHL7_EXP_USER_CMD_SEND_CANCEL  (2)
    
//ssl enable API argument values
#define BTLHL7EXP_SSL_DISABLE (0)
#define BTLHL7EXP_SSL_ENABLE (1)

// internal thread states
typedef enum {
    BTL_HL7_STATE_IDLE,
    BTL_HL7_STATE_INITIATE_SRV_CONNECTION,
    BTL_HL7_STATE_SRV_CONNECTION_WAIT,
    BTL_HL7_STATE_PROCESS_EXPORT,
    BTL_HL7_STATE_SEND_PDF_SEGS,
    BTL_HL7_STATE_SRV_ACK_WAIT,
    BTL_HL7_STATE_ERROR
    ,BTL_HL7_STATE_CLEANUP
 #ifdef BTLHL7EXP_SSL_EN
    // Secure (TLS/SSL) states start here
    ,BTL_HL7_STATE_INITIATE_SSL_CONNECTION
    ,BTL_HL7_STATE_SSL_HANDSHAKE_WAIT
    ,BTL_HL7_STATE_SSL_RESEND
#endif
} BtlHl7ExportState_t;

#define BTLHL7EXP_PDF_NOT_INIT (0)
#define BTLHL7EXP_PDF_INIT_DONE (1)

    //CHANGE BELOW SIZE TO REDUCE THE SIZE PER OBX SEGMENT or PDF CHUNK
        //Each OBX segment will be transmitted with some 
        // delay (BTLHL7EXP_THREAD_SLEEP_ACTIVE_MS) in between
#define BTLHL7EXP_PDF_CHUNK_SIZE_DEFAULT (8000) //(3000)
#define BTLHL7EXP_MIN_PDF_CHUNK_SIZE (512)
#define BTLHL7EXP_MAX_PDF_CHUNK_SIZE (48000)

#define BTLHL7EXP_OBX_FIXED_BYTES_SIZE (200)
    //making file read size a multiple of 3 for base 64 encoding
#define BTLHL7EXP_PDF_FILE_RD_SIZE ((int)(((BTLHL7EXP_PDF_CHUNK_SIZE_DEFAULT - BTLHL7EXP_OBX_FIXED_BYTES_SIZE)*3/4)/3)*3)
#define BTLHL7EXP_PDF_FILE_RD_SIZE_MAX ((int)(((BTLHL7EXP_MAX_PDF_CHUNK_SIZE - BTLHL7EXP_OBX_FIXED_BYTES_SIZE)*3/4)/3)*3)


#define BTLHL7EXP_THREAD_SLEEP_IDLE_MS (100)
#define BTLHL7EXP_THREAD_SLEEP_ACTIVE_MS (20)
#define BTLHL7EXP_SRV_ACK_TIMEOUT_DEF (30)     //time out (seconds) waiting for server ack
#define BTLHL7EXP_SRV_CONNECT_TIMEOUT_DEF (30)     //time out (seconds) waiting for server ack
#define BTLHL7EXP_SRV_TIMEOUT_MIN (3)

//Internal buffer sizes - do not change unless required
#define BTLHL7EXP_MSG_ID_BUF_SIZE (28)
#define BTLHL7EXP_MSG_TIMESTAMP_BUF_SIZE BTLHL7EXP_MSG_ID_BUF_SIZE  //must be same as above
#define BTLHL7EXP_MAX_ATTR_LEN (512) //increase if higher attribute sizes are required 

// enSepObx4EcgMeas values
#define BTLHL7EXP_CFG_OBX_ECG_MEAS_SINGLE  (0)
#define BTLHL7EXP_CFG_OBX_ECG_MEAS_MULTI  (1)

#define BTLHL7_EXP_OBX_RESULT_STATUS_LEN (8)

#define BTLHL7EXP_ORC_ORDER_CONTROL_SIZE (2)

typedef struct HL7ExpAttributeParsed {
    int size;
    int found;
    void* pRelated1;
    char value[BTLHL7EXP_MAX_ATTR_LEN];
} HL7ExpAttributeParsed_t;


typedef struct BtlHl7Export {
    BtlHl7ExportState_t state;
    BtlHl7ExportState_t nextState;

    char diagnosticsFilePath[BTLHL7_MAX_PATH_LEN];   // Full path to .diagnostics.xml
    char pdfFilePath[BTLHL7_MAX_PATH_LEN];           //  PDF path
    char protocolExtraData[BTLHL7_MAX_EXTRA_DATA_LEN];     // XML string for MSH, PID, etc.
    int protocolExtraDataLen;

    char outputHl7Buf[BTLHL7_MAX_HL7_DATA_LEN];          // Output HL7 message
    int outputHl7Len;
    int outputHl7SizeLeft;
    char* pHl7BufNext;
    int hl7MsgSize;   //total number of bytes in HL7 message
    //int isRestrictedExport;
    // Inside BtlHl7Export_t #####
    char srvIpAddrStr[BTLHL7_SERVER_IP_MAX_LEN];     // Holds server IP string
    uint16_t srvPort;          // Holds server port
    struct sockaddr_in clientSkAddr;
    int tcpState;
    int pdfChunkSize;   // Max PDF chunk size for OBX segments
    int debugLevel;     // Per-export debug level

    int obxIndex;

    volatile int startPdfExportFlag;          // Thread trigger
    volatile int abortPdfExportFlag;          // Thread trigger
    volatile int doShutdown;                  // Thread trigger

    volatile int threadRunning;

    int pdfState;
    int pdfFileSplitToMultiObxSeg;
    FILE* expFilePtr;
    long expFileSize;
    long expFileSizeLeft;
    unsigned char expFileRdBuf[BTLHL7EXP_PDF_FILE_RD_SIZE_MAX + 256];

    int exportStatus;
    char placerOrderNumber[128];
    char fillerOrderNumber[128];
    char processingId[8];
    char orderControl[BTLHL7EXP_ORC_ORDER_CONTROL_SIZE + 2];          // ORC-1 (like NW, CA, IP, CM, etc.)
    char _orderControl[BTLHL7EXP_ORC_ORDER_CONTROL_SIZE + 2]; //This is active buffer used to populate the message
    char universalServiceId[128];  // OBR-4 (like ^ECG, ^XRAY, etc.)
    char requestedDateTime[64];       // optional OBR-6
    char observationDateTime[64];     // optional OBR-7

    // ---- HL7 version override 
    char hl7VersionStr[BTL_HL7EXP_VERSION_BUF_SIZE];  // User config e.g., "2.2", "2.3", "2.6"
    int  hl7VersionMajor;      // parsed from hl7VersionStr
    int  hl7VersionMinor;      // parsed from hl7VersionStr
    char hl7VersionDefaultStr[BTL_HL7EXP_VERSION_BUF_SIZE];     // e.g., "2.2", "2.3", "2.6"

    char sendingApplication[BTL_HL7_NAME_BUF_SIZE];
    char sendingFacility[BTL_HL7_NAME_BUF_SIZE];
        //Remote export server application, facility
    char receivingApplication[BTL_HL7_NAME_BUF_SIZE];
    char receivingFacility[BTL_HL7_NAME_BUF_SIZE];

    char receivingApplicationComp1[BTL_HL7_NAME_BUF_SIZE];
    char receivingApplicationComp2[BTL_HL7_NAME_BUF_SIZE];
    char receivingFacilityComp1[BTL_HL7_NAME_BUF_SIZE];
    char receivingFacilityComp2[BTL_HL7_NAME_BUF_SIZE];

    char sendingApplicationComp1[BTL_HL7_NAME_BUF_SIZE];
    char sendingApplicationComp2[BTL_HL7_NAME_BUF_SIZE];
    char sendingFacilityComp1[BTL_HL7_NAME_BUF_SIZE];
    char sendingFacilityComp2[BTL_HL7_NAME_BUF_SIZE];
        //Tag to place in OBX.3 field for PDF file data transfer
    char obxF3TagPdfFileData[BTL_HL7_NAME_BUF_SIZE];

#ifdef BTL_HL7_FOR_WINDOWS
    HANDLE exportThreadHandle;
    SOCKET client_socket;
    WSADATA wsa;
#else
    pthread_t exportThreadHandle;
    int client_socket;
#endif
    int sslEnable;   // 1 = enable SSL/TLS export, 0 = normal TCP
#ifdef BTLHL7EXP_SSL_EN
    int sslInitDone;
    BtlHl7ExportState_t sslSendNextState;

    char peer_ca_file[BTLHL7_MAX_PATH_LEN];
    char own_cert_file[BTLHL7_MAX_PATH_LEN];
    char own_key_file[BTLHL7_MAX_PATH_LEN];
    SSL_CTX* sslCtx;    
    SSL* ssl;
    int sslUseOwnCertificate; //client must use certificate for authentication
    int sslPeerVerifyMode;   //user config
    char sslPeerHostNameMatchStr[BTLHL7_HOST_NAME_BUF_LEN];
#endif

    long threadSleepTimeMs;
    int connectTimeoutCounter;
    int connectTimeoutConfigSeconds;
    int ackWaitTimeOutCounter;
    int ackWaitTimeOutConfigSeconds;
    
    char ackMsgRxBuf[BTL_HL7_MAX_HL7_ACK_MSG_LEN];
    int ackMsgSize;
    int ackCurRxSize;
    int ackStatus;      // 0 = no ack, 1 = received
  //  int exportStatus;   // 0 = idle, 1 = sending, -1 = error
    BtlHl7ExpParser_t parser;

    char msgControlIdStrBuf[BTLHL7EXP_MSG_ID_BUF_SIZE + 4];
    char msgTimestampStrBuf[BTLHL7EXP_MSG_TIMESTAMP_BUF_SIZE + 4];
    volatile int userCmd;   // Determines if it's normal export or server status check
    // allow unordered exam exports when ProtocolExtraData is NULL */
   // int allowUnorderedExport;
    char enSepObx4EcgMeas; // 0 means send all measurment in one OBX segment (default), 1 means send each measurment in seprate OBX
    char obxResultStatusStr[BTLHL7_EXP_OBX_RESULT_STATUS_LEN];
    char orderStatusStr[8];  // ORC-5: default "CM", configurable (e.g. "F")   
    char orderType[64];   // Examination Type, default = "ECG"
  char orderRequestedTimeStr[64];   // OBR-6 (requested time from ProtocolExtraData)
  char examPerformedTimeStr[64];    // OBR-7 (createdDatetime from btlXmlNG)
  char orderTransactionTimeStr[64]; // ORC-9 (Date/Time of Transaction)


} BtlHl7Export_t;

typedef struct BtlHl7ExpPidData {
    char* pid;
    char* fn;
    char* mn;
    char* ln;
    char* title;
    char* dob;
    char* sex;
    char* cls;
} btlHl7ExpPidData_t;

//externs

extern int gBtlHl7ExpDebugLevel;
extern int gDebugMarker1;
//####################### Function Prototypes ###################

    //#################### User API functions ###################

//Logging API
void btlHl7ExpRegisterLogCallback(BtlHl7LogCallback_t cb);
void btlHl7SetDebugLevel(int logLevel);
int  btlHl7GetDebugLevel(void);
void btlHl7ExportPrintf(int logLevel,  char* format, ...);


        // Set HL7 version override; empty string disables override
        int btlHl7ExpSetHl7Version(BtlHl7Export_t* p, char* verStr);

        // Get effective HL7 version for MSH-12
         char* btlHl7GetEffectiveHl7Version(BtlHl7Export_t* p, char* fallback);

        // Build MSH + PID (minimal) from BTL-XML-NG when ProtocolExtraData is absent
       // int btlHl7BuildMinimalFromXmlNg(BtlHl7Export_t* p);



    
    int  btlHl7ExportInit(BtlHl7Export_t* pHl7Exp, char* ipAddrStr, uint16_t port);
    int btlHl7ExpSetSrvIp(BtlHl7Export_t* pHl7Exp,  char* ipAddrStr, uint16_t port);
    int btlHl7ExpSetServerTimeout(BtlHl7Export_t* pHl7Exp, int timeoutSeconds);
    int btlHl7GetExportStatus(BtlHl7Export_t* pHl7Exp);

    int btlHl7ExportPdfReport(BtlHl7Export_t* pHl7Exp,
                          char* pdfPath,
                          char* diagnosticsXmlPath,
                          char* protocolExtraData,
                          int protocolExtraDataLen);
    void btlHl7ExpAbortExport(BtlHl7Export_t* pHl7Exp);
#ifdef UNDEF //DEPRECATED functions #############################################
    int btlHl7ExpSetReceivingApplicationStr(BtlHl7Export_t* pHl7Exp, char* pStr);
    int btlHl7ExpSetReceivingFacilityStr(BtlHl7Export_t* pHl7Exp, char* pStr);
    int btlHl7ExpSetSendingApplicationStr(BtlHl7Export_t* pHl7Exp, char* pStr);
    int btlHl7ExpSetSendingFacilityStr(BtlHl7Export_t* pHl7Exp, char* pStr);
#endif

    int btlHl7ExpSetReceivingApplication(BtlHl7Export_t* pHl7Exp, char* comp1, char* comp2);
    int btlHl7ExpSetReceivingFacility(BtlHl7Export_t* pHl7Exp, char* comp1, char* comp2);

    int btlHl7ExpSetSendingApplication(BtlHl7Export_t* pHl7Exp, char* comp1, char* comp2);
    int btlHl7ExpSetSendingFacility(BtlHl7Export_t* pHl7Exp, char* comp1, char* comp2);

    int btlHl7ExpSetObx3PdfFileTagStr(BtlHl7Export_t* pHl7Exp, char* pStr);

    
    //int btlHl7ExpSetAllowUnorderedExport(BtlHl7Export_t* pHl7Exp, int allow);
    // API: Send HL7 Order Status (ORC/OBR only, no PDF)
    int btlHl7ExpSendOrderStatus(BtlHl7Export_t* pHl7Exp, char* orderControlCode);

    int btlHl7ExpShutdown(BtlHl7Export_t* pHl7Exp);
    // API: Only check if server is connectable (no PDF export)
    void btlHl7ExpCheckServerStatus(BtlHl7Export_t* pHl7Exp);
    int btlHl7ExpSetPdfChunkSize(BtlHl7Export_t* pHl7Exp, int size);
    int btlHl7ExpSetAckTimeout(BtlHl7Export_t* pHl7Exp, int timeoutSeconds);
    int btlHl7ExpSetProcessingId(BtlHl7Export_t* pHl7Exp, char* pStr);
    int btlHl7ExpSetObxEcgMeasCfg(BtlHl7Export_t* pHl7Exp, int cfgVal);
    int btlHl7ExpSetObxResultStatus(BtlHl7Export_t* pHl7Exp, char* pVal);
    int btlHl7ExpSendOrderCancelled(BtlHl7Export_t* pHl7Exp, char* pProtocolExtraData, int protocolExtraDataLen);
    int btlHl7ExpSetOrderControl(BtlHl7Export_t* pHl7Exp, char* orderControl);
    int btlHl7ExpSetOrderStatus(BtlHl7Export_t* pHl7Exp,  char* orderStatus);
    int btlHl7ExpSetOrderType(BtlHl7Export_t* pHl7Exp, char* examType);

    //SSL support API (Available even if BTLHL7EXP_SSL_EN is disabled, but will return -1)
    void btlHl7ExpEnableSsl(BtlHl7Export_t* pHl7Exp, int enable);
    int btlHl7ExpSetOwnCertFile(BtlHl7Export_t* pHl7Exp, char* pStr);
    int btlHl7ExpSetOwnKeyFile(BtlHl7Export_t* pHl7Exp, char* pStr);
    int btlHl7ExpSetPeerCaFile(BtlHl7Export_t* pHl7Exp, char* pStr);
    int btlHl7ExpSetPeerVerifyMode(BtlHl7Export_t* pHl7Exp, int mode);
    int btlHl7ExpSslSetPeerHostMatchStr(BtlHl7Export_t* pHl7Exp, char* pStr);

    //#################### Internal functions  ##################

    void btlHl7PerformExport(BtlHl7Export_t* pHl7Exp);
    void _btlHl7ExpUpdateBufCtl(BtlHl7Export_t* pHl7Exp, int outBufRemaining);
    int btlHl7ExpBase64Encode(char* base64Buf, unsigned char* buffer, int len);
    int _btlHl7SendOnePdfChunk(BtlHl7Export_t* pHl7Exp);
        //Socket communication related functions (Internal)
    int btlHl7ExpConnectToSrv(BtlHl7Export_t* pHl7Exp);
    int btlHl7ExpConnectSrvPoll(BtlHl7Export_t* pHl7Exp);
    int btlHl7ExpCheckSrvAck(BtlHl7Export_t* pHl7Exp);
    void btlHl7ExpGenTimestampAndMsgId(char* pMsgId, char* pTimestamp, int bufSize);
    char* btlHl7ExpPutStrToMsg(char* outBuf, int* outBufRemaining, char* str,
        char endChar, char fieldSep, int* fieldSkipCount);
    char* btlHl7ExpPutFieldSepToMsg(char* outBuf, int* outBufRemaining,
        char fieldSep, int* fieldSkipCount);
    int btlHl7StartExportThread(BtlHl7Export_t* pHl7Exp);
    int btlHl7ExpIsThreadRunning(BtlHl7Export_t* pHl7Exp);
    int btlHl7BuildMinimalFromXmlNg(BtlHl7Export_t* p);
    char* btlHl7ExpPutPidToMsg(char* outBuf, int* pOutBufRemaining, btlHl7ExpPidData_t* pPidData, int* pErrStat);
    char* btlHl7ExpGetMatchedString(char* inStr, char** matchStrings, int inStrLen);
    void _btlHl7SendCancel(BtlHl7Export_t* pHl7Exp);
    int btlHl7ExpConvertIsoToHl7Ts(char* iso_timestamp, char* hl7_timestamp_out, size_t buffer_size);
    void _btlHl7ExpCloseClientSocket(BtlHl7Export_t* pHl7Exp);

    unsigned long long btlhl7ExpGetTickMs(void);

#endif // BTL_HL7_ECG_EXPORT_H 

//END OF .h FILE