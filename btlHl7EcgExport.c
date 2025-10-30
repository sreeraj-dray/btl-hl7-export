

#include "btlHl7EcgExport.h"
#include "btlHl7XmlNg.h"
#include "btlHl7XmlExtraData.h"
#include "btlHl7ExpParseHl7.h"

//Note:
//btlHl7ExportThreadFunc() is the thread that perfroms the export

BtlHl7ExpParser_t* gBtlHl7ExpParser;
int ackStatus=0;
int gBtlHl7ExpDebugLevel = BTLHL7EXP_DEFAULT_LOG_LEVEL;
/*
static BtlHl7LogCallback_t gBtlHl7LogCallbackFn = NULL;
int gCurrentDebugLevel = 0;

void btlHl7RegisterLogCallback(BtlHl7LogCallback_t cb) {
    gBtlHl7LogCallbackFn = cb;
}
*/

//#######################################################################

static void _btlHl7ExpCloseClientSocket(BtlHl7Export_t* pHl7Exp) {
    if (pHl7Exp->client_socket == 0) {
        return;
    }
#ifdef BTL_HL7_FOR_WINDOWS
    closesocket(pHl7Exp->client_socket);
    //WSACleanup();
#else
    //for linux
    close(pHl7Exp->client_socket);
#endif
    pHl7Exp->client_socket = 0;
    return;
}  //_btlHl7ExpCloseClientSocket()

//#######################################################################

int btlHl7CheckIpAndPort(BtlHl7Export_t* pHl7Exp) {
    if (strlen(pHl7Exp->srvIpAddrStr) < 7) {
        //ip address must be atleast 7 characters : 1.1.1.1
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: Invalid IP address!\n");
        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_IPADDR;
        return -1;
    }
    if (pHl7Exp->srvPort == 0) {
        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_TCP_PORT;
        return -1;
    }
    memset(&pHl7Exp->clientSkAddr, 0, sizeof(pHl7Exp->clientSkAddr));
    pHl7Exp->clientSkAddr.sin_family = AF_INET;
    pHl7Exp->clientSkAddr.sin_port = htons(pHl7Exp->srvPort);
    if (inet_pton(AF_INET, pHl7Exp->srvIpAddrStr, &(pHl7Exp->clientSkAddr.sin_addr)) <= 0) {
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: Invalid IP address!\n");
        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_IPADDR;

        _btlHl7ExpCloseClientSocket(pHl7Exp);
        return -1;
    }

    return 0;
}  //btlHl7CheckIpAndPort()

//#######################################################################

static int _btlHl7ExpConnectSrv(BtlHl7Export_t* pHl7Exp) {
    //non-blocking mode only
    int connectStatus = connect(pHl7Exp->client_socket,
        (struct sockaddr*)&(pHl7Exp->clientSkAddr), sizeof(pHl7Exp->clientSkAddr));

    if (connectStatus == SOCKET_ERROR) {
        //this may not be an error - just an indication of blocking call
#ifdef BTL_HL7_FOR_WINDOWS

        int errorCode = WSAGetLastError();
        if (errorCode != WSAEWOULDBLOCK && errorCode != WSAEINPROGRESS) {
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: Connect to server failed-1 (err=%d)\n", errorCode);
            _btlHl7ExpCloseClientSocket(pHl7Exp);
            return -1;
        }
#else
        if (errno != EINPROGRESS) {
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: Connect to server failed-1\n");
            _btlHl7ExpCloseClientSocket(pHl7Exp);
            return -1;
        }
#endif
        return 0;  //connect (non-blocking) successfully initiated
    }  
    return 0;  //connect (non-blocking) successfully initiated (this would never happen)

}//_btlHl7ExpConnectSrv()

    //#######################################################################

 static int _btlHl7ExpSetSocketOptions(BtlHl7Export_t * pHl7Exp) {
     int retVal;
#ifdef BTL_HL7_FOR_WINDOWS
        u_long mode = 1;
        retVal =  ioctlsocket(pHl7Exp->client_socket, FIONBIO, &mode);
        int option = 1;
        if (setsockopt(pHl7Exp->client_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option)) < 0) {
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: setsockopt SO_REUSEADDR failed: %d\n", WSAGetLastError());
            return -1;
        }
        return retVal;
#else
        int flags = fcntl(pHl7Exp->client_socket, F_GETFL, 0);
        if (flags == -1) return -1;
        retVal = fcntl(pHl7Exp->client_socket, F_SETFL, flags | O_NONBLOCK);
        int option = 1;
        if (setsockopt(pHl7Exp->client_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {    
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: setsockopt SO_REUSEADDR failed : %d\n", errno);         
        }
        return retVal;
#endif

    } //_btlHl7ExpSetSocketOptions()


    //#######################################################################

 static int _btlHl7SendDataToSrv(BtlHl7Export_t* pHl7Exp) {
     // Send input
     
     int nBytesTx;
     if (pHl7Exp->outputHl7Len <= 0) {
         return 0; //nothing to do
     }
            //add this chunk size to total message size and transmit the chunk
     pHl7Exp->hl7MsgSize += pHl7Exp->outputHl7Len;
     nBytesTx = send(pHl7Exp->client_socket, pHl7Exp->outputHl7Buf, pHl7Exp->outputHl7Len, 0);
     BTLHL7EXP_DBUG1("BTLHL7EXP: INFO: Transmitting chunk of %d bytes (total till now=%d)\n", pHl7Exp->outputHl7Len, pHl7Exp->hl7MsgSize);
#ifdef BTLHL7EXP_DBUG_HL7_MSG_PRT_EN
     btlHl7ExpMsgDump(pHl7Exp->outputHl7Buf, pHl7Exp->outputHl7Len);
#endif

     if (nBytesTx == SOCKET_ERROR) {
         printf("BTLHL7EXP: ERROR: Socket send failed! Aborting export!\n");
         pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SOCKET_SEND;
         return -1;
     }
        //reset (clear) HL7 transmit buffer for next chunk
     _btlHl7ExpUpdateBufCtl(pHl7Exp, sizeof(pHl7Exp->outputHl7Buf)/*outBufRemaining*/);
     return 0;// data sent successfully
 }  //_btlHl7SendDataToSrv()

 //#######################################################################

// ###  Set IP/port in struct
int btlHl7ExpSetSrvIp(BtlHl7Export_t* pHl7Exp, char* ipAddrStr, uint16_t port) {
    if (pHl7Exp && ipAddrStr && port != 0){
        strncpy_s(pHl7Exp->srvIpAddrStr, sizeof(pHl7Exp->srvIpAddrStr), ipAddrStr, _TRUNCATE);
        pHl7Exp->srvIpAddrStr[sizeof(pHl7Exp->srvIpAddrStr)-1] = '\0';  // Ensure null-termination
        pHl7Exp->srvPort = port;
        return 0;
    }
    return -1;
}

int btlHl7ExpSetProcessingId(BtlHl7Export_t* pHl7Exp, char* procId) {
    if (!pHl7Exp || !procId) {
        return -1;
    }

    if (strcmp(procId, "P") == 0 || strcmp(procId, "T") == 0 || strcmp(procId, "D") == 0) {
        strncpy_s(pHl7Exp->processingId, sizeof(pHl7Exp->processingId), procId, _TRUNCATE);
        return 0;
    }
    return -2; // invalid value
}


int btlHl7ExpSetObxResultStatus(BtlHl7Export_t* pHl7Exp, char* pVal) {
    if (!pHl7Exp || !pVal) {
        return -1;
    }

    if (strcmp(pVal, "P") == 0 || strcmp(pVal, "F") == 0 || strcmp(pVal, "C") == 0) {
        strncpy_s(pHl7Exp->obxResultStatusStr, sizeof(pHl7Exp->obxResultStatusStr), pVal, _TRUNCATE);
        return 0;
    }
    return -2; // invalid value
}

int btlHl7ExpSetOrderStatus(BtlHl7Export_t* pHl7Exp, char* orderStatus)
{
    if (!pHl7Exp || !orderStatus) {
        return -1;
    }

    // Allow CM (modern) and F (legacy)
    if (strcmp(orderStatus, "CM") == 0 || strcmp(orderStatus, "F") == 0) {
        strncpy_s(pHl7Exp->orderStatusStr, sizeof(pHl7Exp->orderStatusStr), orderStatus, _TRUNCATE);
        return 0;
    }

      return -2; // invalid order status
}




//#######################################################################

// ###  Init function (clears + sets IP + starts thread)
int btlHl7ExportInit(BtlHl7Export_t* pHl7Exp, char* ipAddrStr, uint16_t port) {
    if (!pHl7Exp) {
        return -1;
    }
    memset(pHl7Exp, 0, sizeof(BtlHl7Export_t));  // Clear struct

    strncpy_s(pHl7Exp->processingId, sizeof(pHl7Exp->processingId), "P", _TRUNCATE); // default P = Production, MSH.11
    strncpy_s(pHl7Exp->obxResultStatusStr, sizeof(pHl7Exp->obxResultStatusStr), "P", _TRUNCATE); // OBX.11
    strncpy_s(pHl7Exp->orderControl, sizeof(pHl7Exp->orderControl), "RE", _TRUNCATE); // ORC.1 default to Results
    strncpy_s(pHl7Exp->orderStatusStr, sizeof(pHl7Exp->orderStatusStr), "CM", _TRUNCATE);// ORC.5 
    strncpy_s(pHl7Exp->orderType, sizeof(pHl7Exp->orderType), "ECG", _TRUNCATE);



   

    btlHl7ExpSetSrvIp(pHl7Exp, ipAddrStr, port); // Save IP and port
    pHl7Exp->threadSleepTimeMs = BTLHL7EXP_THREAD_SLEEP_IDLE_MS;
    pHl7Exp->ackWaitTimeOutConfigSeconds = BTLHL7EXP_SRV_ACK_TIMEOUT_DEF;  //30 seconds time out waiting for server ack (default)
    pHl7Exp->connectTimeoutConfigSeconds = BTLHL7EXP_SRV_CONNECT_TIMEOUT_DEF;
    strncpy_s(pHl7Exp->sendingApplicationComp1, BTL_HL7_NAME_BUF_SIZE, "BTL_ECG2", _TRUNCATE);
    pHl7Exp->sendingApplicationComp2[0] = '\0';   // default empty
    pHl7Exp->sendingFacilityComp1[0] = '\0';
    pHl7Exp->sendingFacilityComp2[0] = '\0';


    
    strncpy_s(pHl7Exp->hl7VersionDefaultStr, BTL_HL7EXP_VERSION_BUF_SIZE, BTL_HL7_EXP_DEFAULT_HL7_VER_STR, BTL_HL7EXP_VERSION_BUF_SIZE);
    strncpy_s(pHl7Exp->sendingApplication, BTL_HL7_NAME_BUF_SIZE, BTL_HL7_EXP_SENDING_APPLN_STR, BTL_HL7_NAME_BUF_SIZE);
    strncpy_s(pHl7Exp->obxF3TagPdfFileData, BTL_HL7_NAME_BUF_SIZE, BTL_HL7_EXP_OBX3_TAG_DEFAULT_STR, BTL_HL7_NAME_BUF_SIZE);

    pHl7Exp->pdfChunkSize = BTLHL7EXP_PDF_CHUNK_SIZE_DEFAULT;

    btlHl7StartExportThread(pHl7Exp);            // Start background thread
    return 0;
}

//#######################################################################

//Set all file inputs and extra data buffer, then trigger export
int btlHl7ExportPdfReport(BtlHl7Export_t* pHl7Exp,
     char* pdfPath,
     char* diagnosticsXmlPath,
     char* protocolExtraData,
    int protocolExtraDataLen) {
  // if (!pHl7Exp || !pdfPath || !diagnosticsXmlPath || !protocolExtraData || (protocolExtraDataLen <= 0)) {
    if (!pHl7Exp || !pdfPath || !diagnosticsXmlPath || protocolExtraDataLen >= sizeof(pHl7Exp->protocolExtraData)) {
        return BTLHL7EXP_STATUS_INPUT_ERR;
    }
    if (pHl7Exp->exportStatus > BTLHL7EXP_STATUS_NO_ERROR) {
        return BTLHL7EXP_STATUS_BUSY;
    }
        //Copy protocol extra data xml if present to internal buffer
    if (protocolExtraData != NULL && protocolExtraDataLen > 0) {
        strncpy_s(pHl7Exp->protocolExtraData, sizeof(pHl7Exp->protocolExtraData), protocolExtraData, _TRUNCATE);
        pHl7Exp->protocolExtraDataLen = protocolExtraDataLen;
        pHl7Exp->protocolExtraData[protocolExtraDataLen] = 0;

    } else {
        pHl7Exp->protocolExtraData[0] = 0;  // fallback empty string
        pHl7Exp->protocolExtraDataLen = 0;
    }
    // copy the user configured result status code to active buffer
    strncpy_s(pHl7Exp->_orderControl, sizeof(pHl7Exp->_orderControl), pHl7Exp->orderControl, _TRUNCATE);

    //TBDNOW check if pdf file exists and is readable (read the first 16 bytes to check)
            
    strncpy_s(pHl7Exp->pdfFilePath, sizeof(pHl7Exp->pdfFilePath), pdfPath, _TRUNCATE);
    strncpy_s(pHl7Exp->diagnosticsFilePath, sizeof(pHl7Exp->diagnosticsFilePath), diagnosticsXmlPath, _TRUNCATE);
    btlHl7PerformExport(pHl7Exp);  //export will be taken up in the seperate export processing thread
    return 0;
}//btlHl7ExportPdfReport()

//#######################################################################################

int btlHl7ExpSendOrderCancelled(BtlHl7Export_t* pHl7Exp, char* pProtocolExtraData,  int protocolExtraDataLen)
{
    // --- Validation ---
    if (pHl7Exp == NULL || pProtocolExtraData == NULL || protocolExtraDataLen == 0 || protocolExtraDataLen >= sizeof(pHl7Exp->protocolExtraData)) {
        BTLHL7EXP_ERR("BTLHL7EXP: Error: Invalid args to SendOrderCancelled!\n");
        return BTLHL7EXP_STATUS_INPUT_ERR;
}

      // --- Check export status ---
    if (pHl7Exp->exportStatus > BTLHL7EXP_STATUS_NO_ERROR) {
        BTLHL7EXP_ERR("BTLHL7EXP: Export busy, cannot send cancel message.\n");
        return BTLHL7EXP_STATUS_BUSY;
    }

    // --- Store the PED XML in the export structure ---
    strncpy_s(pHl7Exp->protocolExtraData, sizeof(pHl7Exp->protocolExtraData),
              pProtocolExtraData, _TRUNCATE);
    pHl7Exp->protocolExtraDataLen = protocolExtraDataLen;
    pHl7Exp->protocolExtraData[protocolExtraDataLen] = '\0';

    // --- This message does not include a PDF or diagnostics file ---
    pHl7Exp->pdfFilePath[0] = '\0';
    pHl7Exp->diagnosticsFilePath[0] = '\0';

    // --- ORC Control = "CR" (Cancelled as Requested) ---
    strncpy_s(pHl7Exp->_orderControl, sizeof(pHl7Exp->_orderControl), "CR", _TRUNCATE);

    // --- Trigger export process (reuse same thread/queue logic) ---
    _btlHl7SendCancel(pHl7Exp);

    BTLHL7EXP_DBUG1("BTLHL7EXP: SendOrderCancelled initiated (ORC|CR)\n");
    return 0;
}//btlHl7ExpSendOrderCancelled()

//#######################################################################

int btlHl7ExpConnectSrvPoll(BtlHl7Export_t* pHl7Exp) {
    // Check for successful connection to remote server

    int retVal;
    int errorCode;
    BtlHl7ExpPollFd_t pollFd;
    pollFd.fd = pHl7Exp->client_socket;
    pollFd.events = POLLOUT;

    retVal = btlHl7ExpPollFn(&pollFd, 1, 0); //no waiting/blocking

    if (retVal == 0) {
        //not connected yet, no errors either
        return 0;
    }
    else if (retVal < 0) {
        // poll() failed
        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_POLL_FAIL;
        return -1;  //may be out of system memory, otherwise should not happen
    }
    else if (pollFd.revents & POLLOUT) {
        errorCode = 0;
        socklen_t sockLen = sizeof(errorCode);
        if (getsockopt(pHl7Exp->client_socket , SOL_SOCKET, SO_ERROR, 
            (char*)&errorCode, &sockLen) < 0 || errorCode != 0) {
            //Failed to connect, remote server not responding or rejected connection
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_CONNECT;
            return -1;
        }
        // Connection succeeded
        return 1;
    }  

    // Unexpected event
    pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_POLL_FAIL2;
    return -1;

}  //btlHl7ExpConnectSrvPoll()
//######################################################################
static void changeStateToAckWait(BtlHl7Export_t* pHl7Exp, BtlHl7ExportState_t* state) {
                //File export comleted successfully
                *state = BTL_HL7_STATE_SRV_ACK_WAIT;
                BTLHL7EXP_DBUG0("BTLHL7EXP: INFO: Waiting for server ack!\n");
                pHl7Exp->threadSleepTimeMs = 200;
                pHl7Exp->ackWaitTimeOutCounter =
                    (pHl7Exp->ackWaitTimeOutConfigSeconds * 1000) / pHl7Exp->threadSleepTimeMs;
                pHl7Exp->ackMsgSize = 0;  //init rx buf for receiving new ack
                BTLHL7EXP_DBUG1("BTLHL7EXP: Export completed. HL7 Msg Total Length = %d bytes, waiting for srv ack..\n", pHl7Exp->hl7MsgSize);
}//changeStateToAckWait()


//#######################################################################

// State machine thread 
#ifdef BTL_HL7_FOR_WINDOWS
DWORD WINAPI btlHl7ExportThreadFunc(LPVOID arg)
#else  //for linux
void* btlHl7ExportThreadFunc(void* arg)
#endif
{
    int retVal;
    //int nCharWritten;
    //char* outBuf;
    //int outBufRemaining;

    BtlHl7Export_t* pHl7Exp = (BtlHl7Export_t*)arg;
    BtlHl7ExportState_t state = BTL_HL7_STATE_IDLE;
    pHl7Exp->threadRunning = 2;

    while (1) {

            if (pHl7Exp->doShutdown) {
                pHl7Exp->doShutdown = 0;
                pHl7Exp->startPdfExportFlag = 0;
                BTLHL7EXP_DBUG1("BTLHL7EXP: Thread shutdown command received; Exiting thread!\n");
                _btlHl7ExpCloseClientSocket(pHl7Exp);

                if (pHl7Exp->exportStatus > 0) {
                    pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_USER_ABORT;
                }

                pHl7Exp->threadRunning = 0; // Thread is stopping
                return 0;  //EXIT THREAD (SHUTDOWN)
            }

        if (pHl7Exp->abortPdfExportFlag) {
            pHl7Exp->abortPdfExportFlag = 0;
            pHl7Exp->startPdfExportFlag = 0;
            BTLHL7EXP_DBUG1("BTLHL7EXP: Abort export command received; Aborting..\n");
            _btlHl7ExpCloseClientSocket(pHl7Exp);
            if (pHl7Exp->exportStatus > 0) {
                pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_USER_ABORT;
            }
        }

        Sleep(pHl7Exp->threadSleepTimeMs);  // milliseconds

        switch (state) {
        case BTL_HL7_STATE_IDLE:
            pHl7Exp->threadSleepTimeMs = BTLHL7EXP_THREAD_SLEEP_IDLE_MS;
            if (pHl7Exp->startPdfExportFlag) {
                BTLHL7EXP_DBUG1("BTLHL7EXP: Export command received.\n");
                // Reset trigger
                pHl7Exp->startPdfExportFlag = 0;
                //run more often to quckly complete the file export
                pHl7Exp->threadSleepTimeMs = BTLHL7EXP_THREAD_SLEEP_ACTIVE_MS;
                state = BTL_HL7_STATE_INITIATE_SRV_CONNECTION;// BTL_HL7_STATE_PROCESS_EXPORT;

            }
            break;
        case BTL_HL7_STATE_INITIATE_SRV_CONNECTION:
            BTLHL7EXP_DBUG1("BTLHL7EXP: Initiating connection, IP=%s, Port=%d\n", pHl7Exp->srvIpAddrStr, pHl7Exp->srvPort);
            if (btlHl7ExpConnectToSrv(pHl7Exp) != 0) {
                //error occured during connect, abort the export
                state = BTL_HL7_STATE_IDLE;
                break;
            }
            state = BTL_HL7_STATE_SRV_CONNECTION_WAIT;
            BTLHL7EXP_DBUG1("BTLHL7EXP: Connecting - waiting for server response\n");
            pHl7Exp->connectTimeoutCounter =
                (pHl7Exp->connectTimeoutConfigSeconds * 1000) / pHl7Exp->threadSleepTimeMs;
            break;
        case BTL_HL7_STATE_SRV_CONNECTION_WAIT:

            pHl7Exp->connectTimeoutCounter--;
            if (pHl7Exp->connectTimeoutCounter <= 0) {
                BTLHL7EXP_ERR("BTLHL7EXP ERR: Connect failed (timeout) - no response from server!\n");
                pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_CONNECT;

                _btlHl7ExpCloseClientSocket(pHl7Exp);
                state = BTL_HL7_STATE_IDLE;
                break;
            }
            retVal = btlHl7ExpConnectSrvPoll(pHl7Exp);
            if (retVal < 0) {
                //error occured during connect wait, abort the export
                _btlHl7ExpCloseClientSocket(pHl7Exp);
                state = BTL_HL7_STATE_IDLE;
                break;
            }
            if (retVal == 0) {
                break; //not connected yet, check again after some time
            }
            //connected successfully to export server

            //TBDNOW remove below after test
            //BTLHL7EXP_DBUG0("\n####################### WARNING: Socket closed - test mode! ##########\n\n");
            //_btlHl7ExpCloseClientSocket(pHl7Exp);
            state = BTL_HL7_STATE_PROCESS_EXPORT;
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_INPROGRESS;
            break;
        case BTL_HL7_STATE_PROCESS_EXPORT:
            BTLHL7EXP_DBUG1("BTLHL7EXP: Processing export...\n");
            int xmlNgStat = -1;

            if (pHl7Exp->userCmd == BTLHL7_EXP_USER_CMD_SRV_STATUS) {
                    //User request is for checking if server is alive, so stopping here 
                _btlHl7ExpCloseClientSocket(pHl7Exp);
                BTLHL7EXP_DBUG1("BTLHL7EXP: Server status check succeeded!\n");
                pHl7Exp->exportStatus = BTLHL7EXP_STATUS_NO_ERROR;
                state = BTL_HL7_STATE_IDLE;
                break;
            }
            // parse btlxmlng if avaliable 
            memset(g_BtlHl7ExpXmlNgAttributesParsed, 0, sizeof(g_BtlHl7ExpXmlNgAttributesParsed));
           if (pHl7Exp->diagnosticsFilePath[0]!= 0) {
               xmlNgStat =  btlHl7ParseXmlNg(pHl7Exp);
            }
            // Clear previous output
            memset(pHl7Exp->outputHl7Buf, 0, sizeof(pHl7Exp->outputHl7Buf));
            //Add MLLP protocol message start character to beginning of buffer
            pHl7Exp->outputHl7Buf[0] = BTLHL7EXP_MLLP_MSG_START_CHAR;
            pHl7Exp->outputHl7Len = 1;
            pHl7Exp->pHl7BufNext = &(pHl7Exp->outputHl7Buf[1]);
            pHl7Exp->outputHl7SizeLeft = sizeof(pHl7Exp->outputHl7Buf) - pHl7Exp->outputHl7Len;
            pHl7Exp->hl7MsgSize = 0;  //total HL7 message size

            //create a new message control ID and timestamp for the export message
            btlHl7ExpGenTimestampAndMsgId(pHl7Exp->msgControlIdStrBuf,
                pHl7Exp->msgTimestampStrBuf, BTLHL7EXP_MSG_ID_BUF_SIZE);

            pHl7Exp->obxIndex = 0;
            // Add MSH, PID, ORC, etc., from ProtocolExtraData
            if (pHl7Exp->protocolExtraDataLen > 0) {
                // The initial segments for both pdf export and cancel order will happen here
                BTLHL7EXP_DBUG1("BTLHL7EXP: Generating msg meta data from ExtraData XML.\n");
                retVal = btlHl7ParseProtocolExtraData(pHl7Exp); // Full MSH, PID, ORC, OBR
            }
            else {
                
                BTLHL7EXP_DBUG1("BTLHL7EXP: Auto generating msg meta data since no ExtraData XML available!\n");
                retVal = btlHl7BuildMinimalFromXmlNg(pHl7Exp);  // Minimal fallback: MSH + PID

            }
            // Adding initial part (MSH,PID,ORC..etc ) over, now add OBX segments
           
            //if we are doing cancellation of order , nothing more to be done ; send the message generated above
            if (pHl7Exp->userCmd == BTLHL7_EXP_USER_CMD_SEND_CANCEL) {


                if (_btlHl7SendDataToSrv(pHl7Exp)) {
                    //error occured during sending data on socket
                      //abort export
                    _btlHl7ExpCloseClientSocket(pHl7Exp);
                    state = BTL_HL7_STATE_IDLE;
                    break;
            }
                //cancel message sent , now change state to WAIT FOR ACK
                changeStateToAckWait(pHl7Exp, &state);
                break;
            }// if (pHl7Exp->userCmd == BTLHL7_EXP_USER_CMD_SRV_STATUS)
            
            if (xmlNgStat < 0) {


                // XmlNg parsing failed, so proceed with pdf file transfer alone 
                // conclusion Ecg measurments and conclusion cannot be sent 
                           
            }
            else { 
             
                // we have to generate and send the ECG measurments and conclusion 
                //use seperate obx for each measurment if so configured else send all in one OBX
                if (pHl7Exp->enSepObx4EcgMeas == BTLHL7EXP_CFG_OBX_ECG_MEAS_SINGLE) {
                    btlHl7ExpAddEcgMeasToHl7_0( pHl7Exp, gBtlHl7ExpMeasAttrIds);
                }
                else {
                    btlHl7ExpAddEcgMeasToHl7_1(pHl7Exp, gBtlHl7ExpMeasAttrIds);
                }
                //Now add the doctor's statment in a Obx segment if present 
                btlHl7ExpAddDoctorStatement(pHl7Exp);
            }
            

            
            //error occuring in above function is not fatal. We can still attempt to send the pdf
#ifdef UNDEF
            if (btlHl7ParseDiagnosticsXml(pHl7Exp, pHl7Exp->outputHl7Buf, sizeof(pHl7Exp->outputHl7Buf)) != 0) {
                state = BTL_HL7_STATE_ERROR;
                break;
            }
#endif

            if (_btlHl7SendDataToSrv(pHl7Exp)) {
                //error occured during sending data on socket
                  //abort export
                _btlHl7ExpCloseClientSocket(pHl7Exp);
                state = BTL_HL7_STATE_IDLE;
                break;
            }
            //Now start the PDF segment and change state to send the PDF segment by segment

            pHl7Exp->pdfState = BTLHL7EXP_PDF_NOT_INIT;
            state = BTL_HL7_STATE_SEND_PDF_SEGS; // BTL_HL7_STATE_SEND_RESULT;
            break;

        case BTL_HL7_STATE_SEND_PDF_SEGS:
            retVal = _btlHl7SendOnePdfChunk(pHl7Exp);
            if (retVal < 0) {
                //fatal error - abort exporting, error status id 
                // already set by processing function
                state = BTL_HL7_STATE_IDLE;
                break;
            }
            if (pHl7Exp->expFileSizeLeft <= 0) {
                //File export comleted successfully
                changeStateToAckWait(pHl7Exp, &state);
            }

            break;

        case BTL_HL7_STATE_SRV_ACK_WAIT:
            ackStatus = btlHl7ExpCheckSrvAck(pHl7Exp);
            if (ackStatus >= 0) {
                //ack from server received or abort (fail), error code if any woud be set in exportStatus
                BTLHL7EXP_DBUG1("BTLHL7EXP: INFO: Ack received from server!\n");
                state = BTL_HL7_STATE_IDLE;
            }
            pHl7Exp->ackWaitTimeOutCounter--;
            if (pHl7Exp->ackWaitTimeOutCounter <= 0 && state != BTL_HL7_STATE_IDLE) {
                printf("BTLHL7EXP: ERROR: Timeout waiting for server ACK!\n");
                state = BTL_HL7_STATE_IDLE;
                pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_ACK_TIMEOUT;
            }
            break;

        default:
            state = BTL_HL7_STATE_IDLE;
            break;
        }

    }  //while(1)


    return 0;
}


//#######################################################################

// Start the export thread 
int btlHl7StartExportThread(BtlHl7Export_t* pHl7Exp) {
    if (pHl7Exp->threadRunning != 0) {
        return 0; //Thread already running, so returning success
    }
#ifdef BTL_HL7_FOR_WINDOWS
    pHl7Exp->exportThreadHandle = CreateThread(NULL, 0, btlHl7ExportThreadFunc, pHl7Exp, 0, NULL);
#else
    pthread_create(&(pHl7Exp->exportThreadHandle), NULL, btlHl7ExportThreadFunc, pHl7Exp);
    pthread_detach(pHl7Exp->exportThreadHandle);
#endif

    pHl7Exp->threadRunning = 1;
    return 0;  //started thread
}

//#######################################################################

// API trigger 
void btlHl7PerformExport(BtlHl7Export_t* pHl7Exp) {
    
    pHl7Exp->exportStatus = BTLHL7EXP_STATUS_CONNECTING;
    pHl7Exp->userCmd = BTLHL7_EXP_USER_CMD_EXPORT;
    pHl7Exp->startPdfExportFlag = 1;
}
// trigger sending a confirmation of order cancel 
void _btlHl7SendCancel(BtlHl7Export_t* pHl7Exp) {
    
    pHl7Exp->exportStatus = BTLHL7EXP_STATUS_CONNECTING;
    pHl7Exp->userCmd = BTLHL7_EXP_USER_CMD_SEND_CANCEL;
    pHl7Exp->startPdfExportFlag = 1;
}

// API trigger for server status check (no PDF export)
void btlHl7ExpCheckServerStatus(BtlHl7Export_t* pHl7Exp) {

    pHl7Exp->exportStatus = BTLHL7EXP_STATUS_CONNECTING;
    pHl7Exp->userCmd = BTLHL7_EXP_USER_CMD_SRV_STATUS;
    pHl7Exp->startPdfExportFlag = 1;     // trigger state machine like export
}

//#######################################################################

int btlHl7GetExportStatus(BtlHl7Export_t* pHl7Exp) {
    return pHl7Exp->exportStatus;
}
int btlHl7ExpSetHl7Version(BtlHl7Export_t* pHl7Exp, char* verStr) {
    if (!pHl7Exp || !verStr) return -1;
    strncpy(pHl7Exp->hl7VersionStr, verStr, sizeof(pHl7Exp->hl7VersionStr) - 1);
    pHl7Exp->hl7VersionStr[sizeof(pHl7Exp->hl7VersionStr) - 1] = 0;

    // Parse major.minor
    int major = 0, minor = 0;
    if (sscanf(pHl7Exp->hl7VersionStr, "%d.%d", &major, &minor) == 2) {
        pHl7Exp->hl7VersionMajor = major;
        pHl7Exp->hl7VersionMinor = minor;
    }
    else {
        pHl7Exp->hl7VersionMajor = 2;
        pHl7Exp->hl7VersionMinor = 2;
    }
    return 0;
}
char* btlHl7GetEffectiveHl7Version(BtlHl7Export_t* pHl7Exp, char* fallback) {
    if (pHl7Exp && pHl7Exp->hl7VersionStr[0]) return pHl7Exp->hl7VersionStr;
    return fallback;
}


//Send one chunk of pdf after base64 encoding
//chunk size in the tcp output stream is limited to BTLHL7EXP_PDF_CHUNK_SIZE
int _btlHl7SendOnePdfChunk(BtlHl7Export_t* pHl7Exp) {
    int nCharWritten;
    int maxFileReadSize;
    int sizeRead;
    char* pObxSeg;
    char* outBuf = pHl7Exp->pHl7BufNext;
    pObxSeg = outBuf;
    int outBufRemaining = pHl7Exp->outputHl7SizeLeft;


    if (pHl7Exp->pdfState == BTLHL7EXP_PDF_NOT_INIT) {
        pHl7Exp->pdfState = BTLHL7EXP_PDF_INIT_DONE;
        //open the pdf file and initialise file transfer variables
        pHl7Exp->expFilePtr = fopen(pHl7Exp->pdfFilePath, "rb");
        if (!(pHl7Exp->expFilePtr)) {
            printf("BTLHL7EXP ERROR: Unable to open pdf file: %s\n", pHl7Exp->pdfFilePath);
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_PDF_FILE_OPEN_ERR;
            return -1;  //signal abort to caller, fatal error
        }
        fseek(pHl7Exp->expFilePtr, 0, SEEK_END);
        pHl7Exp->expFileSize = ftell(pHl7Exp->expFilePtr);
        pHl7Exp->expFileSizeLeft = pHl7Exp->expFileSize;
        rewind(pHl7Exp->expFilePtr);
        BTLHL7EXP_DBUG1("BTLHL7EXP INFO: Pdf file size is %d bytes\n", (int) pHl7Exp->expFileSizeLeft);

        if (pHl7Exp->pdfFileSplitToMultiObxSeg == 0) {
            pHl7Exp->obxIndex++; //increment the OBX index
            nCharWritten = snprintf(outBuf, outBufRemaining, "OBX|%d|ST|%s||", pHl7Exp->obxIndex, pHl7Exp->obxF3TagPdfFileData);
            outBuf = outBuf + nCharWritten;
            outBufRemaining -= nCharWritten;
        }

    }

        //TBDNOW: send one chunk of BTLHL7EXP_PDF_CHUNK_SIZE

    maxFileReadSize = (((pHl7Exp->pdfChunkSize * 3 / 4) / 3) * 3) - 33; //BTLHL7EXP_PDF_FILE_RD_SIZE;, must be multiple of 3
    BTLHL7EXP_DBUG1("BTLHL7EXP INFO: Reading %d bytes at a time from pdf file.\n", maxFileReadSize);


    if (pHl7Exp->pdfFileSplitToMultiObxSeg != 0) {
          //send pdf in multiple smaller OBX segments
        pHl7Exp->obxIndex++; //increment the OBX index
        nCharWritten = snprintf(outBuf, outBufRemaining, "OBX|%d|ST|%s||", pHl7Exp->obxIndex, pHl7Exp->obxF3TagPdfFileData);
        outBuf = outBuf + nCharWritten;
        outBufRemaining -= nCharWritten;
    }
        //read pdf file chunk
    sizeRead = (int) fread(pHl7Exp->expFileRdBuf, 1, maxFileReadSize, pHl7Exp->expFilePtr);

    if (sizeRead > 0) {
        pHl7Exp->expFileSizeLeft -= sizeRead;
            //encode to base64 directly to HL7 buffer
        nCharWritten = btlHl7ExpBase64Encode(outBuf, pHl7Exp->expFileRdBuf, sizeRead);
       // BTLHL7EXP_DBUG0("BTLHL7EXP INFO: pdf chunk encoded (%d bytes, left=%d): %s\n", nCharWritten, pHl7Exp->expFileSizeLeft, outBuf);
        outBuf = outBuf + nCharWritten;
        outBufRemaining -= nCharWritten;
    }
    else {
            //control should not reach here normally, but just in case
        if (pHl7Exp->pdfFileSplitToMultiObxSeg != 0) {
            pHl7Exp->obxIndex--;
        }
        pHl7Exp->expFileSizeLeft = 0;
        fclose(pHl7Exp->expFilePtr);
        return 0; 
    }
    
    if (pHl7Exp->pdfFileSplitToMultiObxSeg != 0
        || pHl7Exp->expFileSizeLeft <= 0) {
        //Write final part of OBX segment for the chunk if 
        // multiple small OBX segments used

        nCharWritten = snprintf(outBuf, outBufRemaining, "||||||P\r");
        outBuf = outBuf + nCharWritten;
        outBufRemaining -= nCharWritten;

        if (pHl7Exp->expFileSizeLeft <= 0) {
            *outBuf++ = BTLHL7EXP_MLLP_MSG_END_CHAR;
            *outBuf++ = 0x0D;

            outBufRemaining = outBufRemaining-2;
        }


    }
    *outBuf = 0;  //null terminate, just in case
    _btlHl7ExpUpdateBufCtl(pHl7Exp, outBufRemaining);

    if (pHl7Exp->expFileSizeLeft <= 0) {
        fclose(pHl7Exp->expFilePtr);
    }

    BTLHL7EXP_DBUG0("BTLHL7EXP INFO: pdf OBX segment (pdf file left=%d bytes):\n",  (int)pHl7Exp->expFileSizeLeft);

    if (_btlHl7SendDataToSrv(pHl7Exp)) {
          //error during socket send, status set in _btlHl7SendDataToSrv()
        return -1;
    }

    return 0;
}  //_btlHl7SendOnePdfChunk()

//#######################################################################

void _btlHl7ExpUpdateBufCtl(BtlHl7Export_t* pHl7Exp, int outBufRemaining)
{
    pHl7Exp->outputHl7Len = sizeof(pHl7Exp->outputHl7Buf) - outBufRemaining;
    pHl7Exp->pHl7BufNext = pHl7Exp->outputHl7Buf + pHl7Exp->outputHl7Len;
    pHl7Exp->outputHl7SizeLeft = outBufRemaining;
    pHl7Exp->outputHl7Buf[pHl7Exp->outputHl7Len] = 0;  //null terminate, just in case
} //_btlHl7ExpUpdateBufCtl()

//#######################################################################

int btlHl7ExpBase64Encode(char* base64Buf, unsigned char* buffer, int len) {

    static const char base64_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* p = base64Buf;
    int encodedSizeWritten = 0;
    for (int i = 0; i < len; i += 3) {
        int val = (buffer[i] << 16) + ((i + 1 < len ? buffer[i + 1] : 0) << 8) + (i + 2 < len ? buffer[i + 2] : 0);
        *p++ = base64_table[(val >> 18) & 63];
        *p++ = base64_table[(val >> 12) & 63];
        *p++ = (i + 1 < len) ? base64_table[(val >> 6) & 63] : '=';
        *p++ = (i + 2 < len) ? base64_table[val & 63] : '=';

        encodedSizeWritten += 4;
    }
    *p = '\0';

    return encodedSizeWritten;  //null termination is not included in size
}  //btlHl7ExpBase64Encode()


//#######################################################################

int btlHl7ExpConnectToSrv(BtlHl7Export_t* pHl7Exp)
{
    if (btlHl7CheckIpAndPort(pHl7Exp)){
        return -1;
    }

    if (pHl7Exp->tcpState == BTLHL7EXP_TCP_NOT_INIT) {
#ifdef BTL_HL7_FOR_WINDOWS
        // Initialize Winsock
        if (WSAStartup(MAKEWORD(2, 2), &(pHl7Exp->wsa)) != 0) {
            BTLHL7EXP_ERR("Winsock  initialization failed: %d\n", WSAGetLastError());
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_WSA_INIT;
            return -1;
    }
#else
        //nothing to be done for linux
#endif
        pHl7Exp->tcpState = BTLHL7EXP_INIT_DONE;
    }

        //now try connectiing to server
            // Create server socket
    pHl7Exp->client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (pHl7Exp->client_socket == INVALID_SOCKET) {
        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SOCKET_CREATE;
#ifdef BTL_HL7_FOR_WINDOWS
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        pHl7Exp->tcpState = BTLHL7EXP_TCP_NOT_INIT;
#else
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: Socket creation failed!\n");
#endif
        return -1;
    }
    BTLHL7EXP_DBUG1("BTLHL7EXP INFO: Socket creation successful!\n");

    // Set socket to non-blocking mode
    _btlHl7ExpSetSocketOptions(pHl7Exp);

    if (_btlHl7ExpConnectSrv(pHl7Exp)) {
        return -1; //failed to connect, check error status
    }


    return 0;
}  //btlHl7ExpConnectToSrv()

//#######################################################################


static int _btlHl7ExpAckRx(BtlHl7Export_t* pHl7Exp)
{
    //returns -1 if ack rx failed (socket error/ disconnect)
    //returns 0 on succesful ack Rx
    //returns 1 if receive should be tried later (ack not received yet)

    pHl7Exp->ackCurRxSize = recv(pHl7Exp->client_socket, 
        &(pHl7Exp->ackMsgRxBuf[pHl7Exp->ackMsgSize]), 
        BTL_HL7_MAX_HL7_ACK_MSG_LEN-pHl7Exp->ackMsgSize - 1, 0);

    if (pHl7Exp->ackCurRxSize == 0)
    {
        //socket disconnected
        BTLHL7EXP_DBUG0("BTLHL7EXP:INFO: Server disconnected!\n");
        _btlHl7ExpCloseClientSocket(pHl7Exp);
        return -1;
    }  //if (ackCurRxSize == 0)
            //########################################################################
      //No disconnection yet if we reach here
    if (pHl7Exp->ackCurRxSize < 0)
    {		//No bytes received - check for error

#ifdef BTL_HL7_FOR_WINDOWS
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            return 1;  //try rx after some time
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 1;  //try rx after some time
        }
#endif

            //Error occured during receive - close socket
#ifdef BTL_HL7_FOR_WINDOWS
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: Socket error: %d\n", WSAGetLastError());
#else
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: Socket error: %d\n", errno);
#endif
        _btlHl7ExpCloseClientSocket(pHl7Exp);
            return -1;
    }  //if pHl7Exp->ackCurRxSize < 0

    //########################################################################
//Some bytes have been received if we reach here

    pHl7Exp->ackMsgSize += pHl7Exp->ackCurRxSize;
    pHl7Exp->ackMsgRxBuf[pHl7Exp->ackMsgSize] = 0;  //null terminate

    //check if we have received end of HL7 MLLP message
    char* pMsgEndChar = memchr(pHl7Exp->ackMsgRxBuf, BTLHL7EXP_MLLP_MSG_END_CHAR, pHl7Exp->ackMsgSize);
    if (!pHl7Exp->ackMsgRxBuf) {
          //end of message not received yet, so continue receiving
        return 1;
    }
        //message end detected - process the message
    pMsgEndChar++;
    *pMsgEndChar = 0;  //null terminate
    BTLHL7EXP_DBUG1("BTLHL7EXP: INFO: ACK msg recvd!\n");
#ifdef BTLHL7EXP_DBG_PRT_ACK_MSG_RCVD
    btlHl7ExpMsgDump(pHl7Exp->ackMsgRxBuf, pHl7Exp->ackMsgSize);
#endif

    return 0;  //full message received
}  //_btlHl7ExpAckRx()

//check if server ACK is received
int btlHl7ExpCheckSrvAck(BtlHl7Export_t* pHl7Exp) {

        //check if anything received on socket
        //we expect only one ack message
    int ackRxStat;
    ackRxStat = _btlHl7ExpAckRx(pHl7Exp);
        //_btlHl7ExpAckRx() :
        //returns -1 if ack rx failed (socket error/ disconnect)
        //returns 0 on succesful full ack message received
        //returns 1 if receive should be tried later (ack not received yet)
    if (ackRxStat < 0) {

        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SOCKET_RECV;
        return 0;  //terminate waiting for ACK state
    }
    if (ackRxStat == 1) {
        return -1;  //continue waiting for ACK state till timeout or ACK receive
    }
        //We have received a complete ACK message if we reach here
        //parse ACK message

    int nKnownSegmentsFound;
    btlHl7ExpInitHl7Parser(&pHl7Exp->parser);
    nKnownSegmentsFound = btlHl7ExpParseHL7Message_L1(&pHl7Exp->parser, pHl7Exp->ackMsgRxBuf);
    //BTLHL7EXP_DBUG0("BTLHL7EXP: INFO: ACK msg nKnownSegmentsFound=%d\n", nKnownSegmentsFound);
    //parse L2 and check the ack code received
    btlHl7ExpParseHL7Message_L2(&pHl7Exp->parser, pHl7Exp->ackMsgRxBuf, nKnownSegmentsFound);

    HL7MsgAttributeParsed_t* pAttr = btlHl7ExpGetAttrById(&pHl7Exp->parser, BTLHL7EXP_MSA_ACK_STAT);
    if (pAttr->size <= 0 || pAttr->size >4) {
        //error parsing ACK
        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_ACK;
    }
    else {
        if (strncmp(pAttr->value, "AA", pAttr->size)) {
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_ACK;
            BTLHL7EXP_DBUG1("BTLHL7EXP: INFO: ACK Status Rx is (NOT OK): %s\n", pAttr->value);
        }
        else {
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_NO_ERROR;  //completed export, no error
            BTLHL7EXP_DBUG1("BTLHL7EXP: INFO: ACK Status Rx is (OK): %s\n", pAttr->value);
        }
    }

    return 0;  //ACK received, terminate waiting for ACK
}  //btlHl7ExpCheckSrvAck()

int btlHl7ExpSetServerTimeout(BtlHl7Export_t* pHl7Exp, int timeoutSeconds) {
    if (timeoutSeconds < BTLHL7_EXPORT_SERVER_TIMEOUT_SEC) {
        BTLHL7EXP_ERR("BTLHL7EXP Server timeout is invalid: %d\n", timeoutSeconds);
        return -1;
    }
    pHl7Exp->connectTimeoutConfigSeconds = timeoutSeconds;
    return 0;
}

void btlHl7ExpAbortExport(BtlHl7Export_t* pHl7Exp) {
    pHl7Exp->abortPdfExportFlag = 1;
    return;
}

int btlHl7ExpShutdown(BtlHl7Export_t* pHl7Exp) {
    pHl7Exp->doShutdown = 1;
    return 0;
}


int btlHl7ExpIsThreadRunning(BtlHl7Export_t* pHl7Exp) {
    return pHl7Exp->threadRunning;  //running if threadRunning != 0
}

void btlHl7ExpGenTimestampAndMsgId(char* pMsgId, char* pTimestamp, int bufSize) {

    int nCharWritten;
    int totCharWritten = 0;
    // Generate timestamp
    struct tm tm_info;

#ifdef BTL_HL7_FOR_WINDOWS
    struct _timeb t;
    _ftime_s(&t);
    localtime_s(&tm_info, &t.time);

    //char tzOffset[8];
    int tzMins = -(t.timezone);
    int tzHours = tzMins / 60;
    int tzRemMins = abs(tzMins % 60);

    nCharWritten = snprintf(pTimestamp + totCharWritten, bufSize - totCharWritten,
        "%04d%02d%02d%02d%02d%02d",
        tm_info.tm_year + 1900,
        tm_info.tm_mon + 1,
        tm_info.tm_mday,
        tm_info.tm_hour,
        tm_info.tm_min,
        tm_info.tm_sec
    );
    totCharWritten += nCharWritten;
    nCharWritten = snprintf(pTimestamp + totCharWritten, bufSize - totCharWritten,
        ".%03u%+03d%02d", t.millitm, tzHours, tzRemMins);

#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm_info);

    snprintf(pTimestamp, bufSize, "%04d%02d%02d%02d%02d%02d.%03d+0000",
        tm_info.tm_year + 1900,
        tm_info.tm_mon + 1,
        tm_info.tm_mday,
        tm_info.tm_hour,
        tm_info.tm_min,
        tm_info.tm_sec,
        (int)(tv.tv_usec / 1000)
    );
#endif

    // Generate new control ID for ACK (timestamp + 6-digit random)

    snprintf(pMsgId, bufSize, "%04d%02d%02d%02d%02d%02d%06u",
        tm_info.tm_year + 1900,
        tm_info.tm_mon + 1,
        tm_info.tm_mday,
        tm_info.tm_hour,
        tm_info.tm_min,
        tm_info.tm_sec,
        rand() % 1000000);

    return;
}  //btlHl7ExpGenTimestampAndMsgId()

//#############################################################################

char* btlHl7ExpPutStrToMsg(char* outBuf, int* outBufRemaining, char* str,
    char endChar, char fieldSep, int* fieldSkipCount) {

    int i;
    int nCharWritten;

    if (str && str[0] != 0) {
        //str avail to add to msg
        //add field sep/ sub comp sep as required by fieldSkipCount
        if (fieldSkipCount  && *fieldSkipCount > 0 && fieldSep != 0) {

            for (i = 0; i < *fieldSkipCount; i++) {
                nCharWritten = snprintf(outBuf, *outBufRemaining, "%c", fieldSep);
                outBuf += nCharWritten;
                *outBufRemaining -= nCharWritten;
            }
        }
        if (fieldSkipCount) {
            *fieldSkipCount = 1; //reset since one field/ sub comp is being written now
        }
        //Add the string at the correct field/ subcomponent

        nCharWritten = snprintf(outBuf, *outBufRemaining, "%s", str);
        outBuf += nCharWritten;
        *outBufRemaining -= nCharWritten;

        if (endChar != 0) {
            nCharWritten = snprintf(outBuf, *outBufRemaining, "%c", endChar);
            outBuf += nCharWritten;
            *outBufRemaining -= nCharWritten;
        }

    } //if (str && str[0] != 0)
    else{
        if (fieldSkipCount) {
            (*fieldSkipCount)++;
        }
    }

    return outBuf;
}  //btlHl7ExpPutStrToMsg()


char* btlHl7ExpPutFieldSepToMsg(char* outBuf, int* outBufRemaining,
    char fieldSep, int* fieldSkipCount) {
    int i;
    int nCharWritten;
    if (fieldSkipCount && *fieldSkipCount > 0 && fieldSep != 0) {

        for (i = 0; i < *fieldSkipCount; i++) {
            nCharWritten = snprintf(outBuf, *outBufRemaining, "%c", fieldSep);
            outBuf += nCharWritten;
            *outBufRemaining -= nCharWritten;
        }
        *fieldSkipCount = 0;
    }
    return outBuf;
}  //btlHl7ExpPutFieldSepToMsg()

int btlHl7ExpSetReceivingApplication(BtlHl7Export_t* pHl7Exp, char* comp1, char* comp2) {
    if (!pHl7Exp) {
        return -1;
    }
    int len1 = BTL_HL7_NAME_BUF_SIZE - 4; //reserve few bytes for adding component seperator and null termination
    int comp1Len = 0;
    int lenLeft = len1;
    int comp2Len = 0;

    if (comp1) {
        comp1Len = (int) strlen(comp1);
    }
    if (comp2) {
        comp2Len = (int) strlen(comp2);
    }

    if ((comp1Len + comp2Len) > len1) {
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: receivingApplication combined strings length too big!\n");
        return -1; //do not change existing setting
    }

    if (comp1) {
        strncpy(pHl7Exp->receivingApplicationComp1, comp1, len1);
        strncpy(pHl7Exp->receivingApplication, comp1, len1);
        lenLeft = lenLeft - comp1Len;
    }
    else {
        pHl7Exp->receivingApplicationComp1[0] = 0;
        pHl7Exp->receivingApplication[0] = 0;
    }

    if (comp2 && (comp2[0] != 0)) {
        strncpy(pHl7Exp->receivingApplicationComp2, comp2, len1);
        pHl7Exp->receivingApplication[comp1Len] = '^';  //add component seperator first to combined string
        strncpy(&(pHl7Exp->receivingApplication[comp1Len + 1]), comp2, lenLeft);
    }
    else {
        pHl7Exp->receivingApplicationComp2[0] = 0;
    }

    BTLHL7EXP_DBUG1("BTL_HL7_EXP: receivingApplication set to %s\n", pHl7Exp->receivingApplication);

    return 0;
}  //btlHl7ExpSetReceivingApplication()
int btlHl7ExpSetReceivingFacility(BtlHl7Export_t* pHl7Exp, char* comp1, char* comp2) {
    if (!pHl7Exp) {
        return -1;
    }
    int len1 = BTL_HL7_NAME_BUF_SIZE - 4; //reserve few bytes for adding component seperator and null termination
    int comp1Len = 0;
    int lenLeft = len1;
    int comp2Len = 0;

    if (comp1) {
        comp1Len = (int) strlen(comp1);
    }
    if (comp2) {
        comp2Len = (int) strlen(comp2);
    }

    if ((comp1Len + comp2Len) > len1) {
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: receivingFacility combined strings length too big!\n");
        return -1; //do not change existing setting
    }

    if (comp1) {
        strncpy(pHl7Exp->receivingFacilityComp1, comp1, len1);
        strncpy(pHl7Exp->receivingFacility, comp1, len1);
        lenLeft = lenLeft - comp1Len;
    }
    else {
        pHl7Exp->receivingFacilityComp1[0] = 0;
        pHl7Exp->receivingFacility[0] = 0;
    }

    if (comp2 && (comp2[0] != 0)) {
        strncpy(pHl7Exp->receivingFacilityComp2, comp2, len1);
        pHl7Exp->receivingFacility[comp1Len] = '^';  //add component seperator first to combined string
        strncpy(&(pHl7Exp->receivingFacility[comp1Len + 1]), comp2, lenLeft);
    }
    else {
        pHl7Exp->receivingFacilityComp2[0] = 0;
    }

    BTLHL7EXP_DBUG1("BTL_HL7_EXP: receivingFacility set to %s\n", pHl7Exp->receivingFacility);

    return 0;
}  //btlHl7ExpSetReceivingFacility()


int btlHl7ExpSetSendingApplication(BtlHl7Export_t* pHl7Exp, char* comp1, char* comp2) {
    if (!pHl7Exp) {
        return -1;
    }
    int len1 = BTL_HL7_NAME_BUF_SIZE - 4; //reserve few bytes for adding component seperator and null termination
    int comp1Len = 0;
    int lenLeft = len1;
    int comp2Len = 0;

    if (comp1) {
        comp1Len = (int) strlen(comp1);
    }
    if (comp2) {
        comp2Len = (int) strlen(comp2);
    }

    if ((comp1Len + comp2Len) > len1) {
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: sendingApplication combined strings length too big!\n");
        return -1; //do not change existing setting
    }

    if (comp1) {
        strncpy(pHl7Exp->sendingApplicationComp1, comp1, len1);
        strncpy(pHl7Exp->sendingApplication, comp1, len1);
        lenLeft = lenLeft - comp1Len;
    }
    else {
        pHl7Exp->sendingApplicationComp1[0] = 0;
        pHl7Exp->sendingApplication[0] = 0;
    }

    if (comp2 && (comp2[0] != 0)) {
        strncpy(pHl7Exp->sendingApplicationComp2, comp2, len1);
        pHl7Exp->sendingApplication[comp1Len] = '^';  //add component seperator first to combined string
        strncpy(&(pHl7Exp->sendingApplication[comp1Len + 1]), comp2, lenLeft);
    }
    else {
        pHl7Exp->sendingApplicationComp2[0] = 0;
    }

    BTLHL7EXP_DBUG1("BTL_HL7_EXP: sendingApplication set to %s\n", pHl7Exp->sendingApplication);

    return 0;
}  //btlHl7ExpSetSendingApplication()


int btlHl7ExpSetSendingFacility(BtlHl7Export_t* pHl7Exp, char* comp1, char* comp2) {
    if (!pHl7Exp) {
        return -1;
    }
    int len1 = BTL_HL7_NAME_BUF_SIZE - 4; //reserve few bytes for adding component seperator and null termination
    int comp1Len = 0;
    int lenLeft = len1;
    int comp2Len = 0;

    if (comp1) {
        comp1Len = (int) strlen(comp1);
    }
    if (comp2) {
        comp2Len = (int) strlen(comp2);
    }

    if ((comp1Len + comp2Len) > len1) {
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: sendingFacility combined strings length too big!\n");
        return -1; //do not change existing setting
    }

    if (comp1) {
        strncpy(pHl7Exp->sendingFacilityComp1, comp1, len1);
        strncpy(pHl7Exp->sendingFacility, comp1, len1);
        lenLeft = lenLeft - comp1Len;
    }
    else {
        pHl7Exp->sendingFacilityComp1[0] = 0;
        pHl7Exp->sendingFacility[0] = 0;
    }

    if (comp2 && (comp2[0] != 0)) {
        strncpy(pHl7Exp->sendingFacilityComp2, comp2, len1);
        pHl7Exp->sendingFacility[comp1Len] = '^';  //add component seperator first to combined string
        strncpy(&(pHl7Exp->sendingFacility[comp1Len + 1]), comp2, lenLeft);
    }
    else {
        pHl7Exp->sendingFacilityComp2[0] = 0;
    }
    BTLHL7EXP_DBUG1("BTL_HL7_EXP: SendingFacility set to %s\n", pHl7Exp->sendingFacility);

    return 0;
}  //btlHl7ExpSetSendingFacility()



int btlHl7ExpSetObx3PdfFileTagStr(BtlHl7Export_t* pHl7Exp, char* pStr) {
    if (!pHl7Exp || !pStr) {
        return -1;
    }
    strncpy(pHl7Exp->obxF3TagPdfFileData, pStr, sizeof(pHl7Exp->obxF3TagPdfFileData) - 1);
    pHl7Exp->obxF3TagPdfFileData[sizeof(pHl7Exp->obxF3TagPdfFileData) - 1] = 0;

    return 0;
} //btlHl7ExpSetObx3PdfFileTagStr()




char* btlHl7ExpPutPidToMsg(char *outBuf, int* pOutBufRemaining, btlHl7ExpPidData_t* pPidData, int* pErrStat){

    int outBufRemaining = *pOutBufRemaining;
    int fieldSkipCount = 0;
    *pErrStat = 0;

    char* pid = pPidData->pid;
    char* fn = pPidData->fn;
    char* mn = pPidData->mn;
    char* ln = pPidData->ln;
    char* title = pPidData->title;
    char* dob = pPidData->dob;
    char* sex = pPidData->sex;
    char* cls = pPidData->cls;

    int namePresent = 0;
    int pidPresent = 0;

    if (pid && pid[0] != 0) {
        pidPresent = 1;
    }
    if (ln && ln[0] != 0) {
        namePresent = 1;
    }
    if (mn && mn[0] != 0) {
        namePresent = 1;
    }
#ifdef UNDEF
    if (title && title[0] != 0) {
        namePresent = 1;
    }
#endif
    if (pidPresent || namePresent) {
        outBuf = btlHl7ExpPutStrToMsg(outBuf, &outBufRemaining, "PID|1",
            0 /*endChar*/, 0 /*fieldSep*/, 0 /*&fieldSkipCount*/);

        //Write Patient UID if available
        fieldSkipCount = 2;
        //Field-3 : PID
        outBuf = btlHl7ExpPutStrToMsg(outBuf, &outBufRemaining, pid,
            0 /*endChar*/, '|' /*fieldSep*/, &fieldSkipCount);

        //add one more '|' to reach the patient name field
        fieldSkipCount++;
        //Field-5 : Name
        if (namePresent) {

            outBuf = btlHl7ExpPutFieldSepToMsg(outBuf, &outBufRemaining,
                '|' /*fieldSep*/, &fieldSkipCount);

            fieldSkipCount = 0;
            outBuf = btlHl7ExpPutStrToMsg(outBuf, &outBufRemaining, ln,
                0 /*endChar*/, 0 /*fieldSep*/, &fieldSkipCount);
            outBuf = btlHl7ExpPutStrToMsg(outBuf, &outBufRemaining, fn,
                0 /*endChar*/, '^' /*fieldSep*/, &fieldSkipCount);
            outBuf = btlHl7ExpPutStrToMsg(outBuf, &outBufRemaining, mn,
                0 /*endChar*/, '^' /*fieldSep*/, &fieldSkipCount);
            fieldSkipCount++;
            outBuf = btlHl7ExpPutStrToMsg(outBuf, &outBufRemaining, title,
                0 /*endChar*/, '^' /*fieldSep*/, &fieldSkipCount);
            fieldSkipCount = 2;
        } //if namePresent
        else {
            fieldSkipCount += 2;
        }
        //Field-6 : Not used 
        //Field-7 :  Date of birth
        outBuf = btlHl7ExpPutStrToMsg(outBuf, &outBufRemaining, dob,
            0 /*endChar*/, '|' /*fieldSep*/, &fieldSkipCount);

        //Field-8 : Sex
        outBuf = btlHl7ExpPutStrToMsg(outBuf, &outBufRemaining, sex,
            0 /*endChar*/, '|' /*fieldSep*/, &fieldSkipCount);
        

        //Field-9 : Unused
        fieldSkipCount++;

        if (cls && cls[0] != 0) {
            outBuf = btlHl7ExpPutFieldSepToMsg(outBuf, &outBufRemaining,
                '|' /*fieldSep*/, &fieldSkipCount);

            fieldSkipCount = 1; //Class to be at sub-component-2
            outBuf = btlHl7ExpPutStrToMsg(outBuf, &outBufRemaining, cls,
                0 /*endChar*/, '^' /*fieldSep*/, &fieldSkipCount);
        }
        //write the \r to mark the segment end
        outBuf = btlHl7ExpPutStrToMsg(outBuf, &outBufRemaining, "\r",
            0 /*endChar*/, 0 /*fieldSep*/, 0 /*&fieldSkipCount*/);
        /*
                             nCharWritten = snprintf(outBuf, outBufRemaining,
                                 "PID|1||%s||%s^%s^%s^^%s||%s|%s||%s\r",
                                 pid ? pid : "", ln ? ln : "", fn ? fn : "", mn ? mn : "", title ? title : "",
                                 dob ? dob : "", sex ? sex : "", cls ? cls : "");
                             //strncat_s(outBuf, outBufRemaining, temp, _TRUNCATE);
                             if (nCharWritten < 0) {
                                 printf("BTLHL7EXP: Error: Failed HL7 buffer write-2!\n");
                                 pHl7Exp->exportStatus = BTLHL7EXP_STATUS_PROCESSING_ERR;
                                 return -1;
                             }
                             outBuf = outBuf + nCharWritten;
                             outBufRemaining = outBufRemaining - nCharWritten;
         */
    }//if pid or name is present
    else {
        *pErrStat = -1;
    }
    *pOutBufRemaining = outBufRemaining;
    return outBuf;
}  //btlHl7ExpPutPidToMsg()



int btlHl7ExpSetPdfChunkSize(BtlHl7Export_t* pHl7Exp, int size) {
    if (!pHl7Exp) return -1;
    pHl7Exp->pdfChunkSize = size;
    return 0;
}

int btlHl7ExpSetAckTimeout(BtlHl7Export_t* pHl7Exp, int timeoutSeconds) {
    if (!pHl7Exp) return -1;
    pHl7Exp->ackWaitTimeOutConfigSeconds = timeoutSeconds;
    return 0;
}


int btlHl7ExpSetObxEcgMeasCfg(BtlHl7Export_t* pHl7Exp, int cfgVal) {
    if (!pHl7Exp) {
        return -1;
    }
    if (cfgVal< BTLHL7EXP_CFG_OBX_ECG_MEAS_SINGLE || cfgVal>BTLHL7EXP_CFG_OBX_ECG_MEAS_MULTI) {
        return -2;
    }
    pHl7Exp->enSepObx4EcgMeas = cfgVal;
    return 0;
}


int btlHl7ExpSendOrderStatus(BtlHl7Export_t* pHl7Exp, char* orderControlCode) {
    if (!pHl7Exp || !orderControlCode) {
        return BTLHL7EXP_STATUS_INPUT_ERR;
    }

    // Make sure we have order numbers
    if (pHl7Exp->placerOrderNumber[0] == '\0' && pHl7Exp->fillerOrderNumber[0] == '\0') {
        btlHl7ExportPrintf(1, "[OrderStatus] ERROR: Missing Placer/Filler order numbers.\n");
        return BTLHL7EXP_STATUS_INPUT_ERR;
    }

    // Reset buffer
    pHl7Exp->outputHl7Len = 0;
    pHl7Exp->outputHl7SizeLeft = sizeof(pHl7Exp->outputHl7Buf);
    pHl7Exp->pHl7BufNext = pHl7Exp->outputHl7Buf;

    int errStat = 0;

    // --- MSH Header ---
    btlHl7ExpPutStrToMsg(pHl7Exp->pHl7BufNext, &pHl7Exp->outputHl7SizeLeft,
        "MSH|^~\\&|", '\r', '|', &errStat);

    // Sending/Receiving details
    btlHl7ExpPutStrToMsg(pHl7Exp->pHl7BufNext, &pHl7Exp->outputHl7SizeLeft,
        pHl7Exp->sendingApplication, '|', '|', &errStat);
    btlHl7ExpPutStrToMsg(pHl7Exp->pHl7BufNext, &pHl7Exp->outputHl7SizeLeft,
        pHl7Exp->sendingFacility, '|', '|', &errStat);
    btlHl7ExpPutStrToMsg(pHl7Exp->pHl7BufNext, &pHl7Exp->outputHl7SizeLeft,
        pHl7Exp->receivingApplication, '|', '|', &errStat);
    btlHl7ExpPutStrToMsg(pHl7Exp->pHl7BufNext, &pHl7Exp->outputHl7SizeLeft,
        pHl7Exp->receivingFacility, '|', '\r', &errStat);

    // --- ORC Segment ---
    char orcSeg[256];
    snprintf(orcSeg, sizeof(orcSeg), "ORC|%s|%s|%s\r",
        orderControlCode,
        pHl7Exp->placerOrderNumber,
        pHl7Exp->fillerOrderNumber);
    btlHl7ExpPutStrToMsg(pHl7Exp->pHl7BufNext, &pHl7Exp->outputHl7SizeLeft,
        orcSeg, '\0', '\0', &errStat);

    // --- OBR Segment ---
    snprintf(orcSeg, sizeof(orcSeg),
         "ORC|%s|%s|%s||||||||%s\r",
         orderControlCode,
         pHl7Exp->placerOrderNumber,
         pHl7Exp->fillerOrderNumber,
         pHl7Exp->orderTransactionTimeStr[0] ? pHl7Exp->orderTransactionTimeStr : "");

    pHl7Exp->hl7MsgSize = (int) strlen(pHl7Exp->outputHl7Buf);

    // Debug print
    btlHl7ExportPrintf(1, "[OrderStatus] HL7 Msg:\n%s\n", pHl7Exp->outputHl7Buf);

    // Send to server
    int sockStatus = btlHl7ExpConnectToSrv(pHl7Exp);
    if (sockStatus < 0) {
        return sockStatus;
    }

    int sendRes = send(pHl7Exp->client_socket, pHl7Exp->outputHl7Buf, pHl7Exp->hl7MsgSize, 0);
    closesocket(pHl7Exp->client_socket);
    if (sendRes <= 0) {
        return BTLHL7EXP_STATUS_ERR_SOCKET_SEND;
    }

    return BTLHL7EXP_STATUS_NO_ERROR;
} //btlHl7ExpSendOrderStatus()





char* btlHl7ExpGetMatchedString(char* inStr, char** matchStrings, int inStrLen) {
// match strings will end with null pointer
    for (char** p = matchStrings; *p != NULL; p++) {
        if (strncmp(*p, inStr, inStrLen) == 0) {
            return *p;
        }

    }
    return NULL;
}//btlHl7ExpGetMatchedString

int btlHl7ExpSetOrderControl(BtlHl7Export_t* pHl7Exp, char* orderControl) {
    if (!pHl7Exp || !orderControl) {
        return -1;
    }
    // user must take care to set only valid hl7 codes
     
    strncpy_s(pHl7Exp->orderControl, BTLHL7EXP_ORC_ORDER_CONTROL_SIZE, orderControl, _TRUNCATE);
    return 0; 

}
int btlHl7ExpSetOrderType(BtlHl7Export_t* pHl7Exp, char* orderType) {
    if (!pHl7Exp || !orderType) {
        return -1;
    }

    strncpy_s(pHl7Exp->orderType, sizeof(pHl7Exp->orderType), orderType, _TRUNCATE);
    pHl7Exp->orderType[sizeof(pHl7Exp->orderType) - 1] = '\0';
    return 0;
}

/* @param buffer_size The size of the output buffer (must be at least 15 for YYYYMMDDHHMMSS\0).
 /* @return int 0 on success, -1 on error (e.g., NULL pointers, insufficient buffer size).
 */
int btlHl7ExpConvertIsoToHl7Ts(char *iso_timestamp, char *hl7_timestamp_out, size_t buffer_size) {
    if (iso_timestamp == NULL || hl7_timestamp_out == NULL) {
        return -1; // Null pointer check
    }

    // HL7 DTM (YYYYMMDDHHMMSS) requires 14 characters plus the null terminator ('\0').
    const size_t REQUIRED_LENGTH = 14; 
    if (buffer_size < (REQUIRED_LENGTH + 1)) {
        return -1; // Buffer too small
    }

    // Expected ISO 8601 format length without timezone (YYYY-MM-DDTHH:MM:SS) is 19 characters.
    if (strlen(iso_timestamp) < 19) {
        return -1; // Input string too short
    }

    // --- Core Conversion Logic ---

    // 1. Copy Year (YYYY)
    strncpy(hl7_timestamp_out, iso_timestamp, 4); 
    
    // 2. Copy Month (MM) - Skip '-' at index 4, start at index 5
    strncpy(hl7_timestamp_out + 4, iso_timestamp + 5, 2); 
    
    // 3. Copy Day (DD) - Skip '-' at index 7, start at index 8
    strncpy(hl7_timestamp_out + 6, iso_timestamp + 8, 2); 
    
    // 4. Copy Hour (HH) - Skip 'T' at index 10, start at index 11
    strncpy(hl7_timestamp_out + 8, iso_timestamp + 11, 2); 
    
    // 5. Copy Minute (MM) - Skip ':' at index 13, start at index 14
    strncpy(hl7_timestamp_out + 10, iso_timestamp + 14, 2); 
    
    // 6. Copy Second (SS) - Skip ':' at index 16, start at index 17
    strncpy(hl7_timestamp_out + 12, iso_timestamp + 17, 2); 

    // 7. Null-terminate the string
    hl7_timestamp_out[14] = '\0'; 

    return 0; // Success
}

