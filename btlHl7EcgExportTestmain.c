//#define TEST_HL7 1 //NOTE: This is included in the build properties (Visual Studio) or on compile command line

#ifdef TEST_HL7

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "btlHl7EcgExport.h"
#include "btlHl7ExpDebug.h" 
#include "btlHl7Config.h"
#include "btlHl7ExpSslCommon.h"

#define BTLHL7EXP_TEST_LOAD_CFG_FROM_XML 1
#ifdef BTLHL7EXP_SSL_EN  //from library.h file or compiler command line
    #define BTLHL7EXP_TEST_SSL_EN 1  //enable SSL for all tests (WARNING: cfg from XML will override if enabled above!)
        #define BTLHL7EXP_TEST_SSL_USE_OWN_CERT_EN 1    //enables the use of a client certificate and key, both must be set by user
        #define  BTLHL7EXP_TEST_SET_PEER_VERIFY_MODE_EN 1       //enables verification of remote server certificate, certificate+IPaddress or certificate+hostname 
                //BTLHL7EXP_SSL_VERIFY_NONE=disable verification of peer, BTLHL7EXP_SSL_VERIFY_CA= verify only certificate
                // BTLHL7EXP_SSL_VERIFY_HOST=verify hostname, BTLHL7EXP_SSL_VERIFY_IP=verify IP address
            #define BTLHL7EXP_SSL_TEST_PEER_VERIFY_MODE BTLHL7EXP_SSL_VERIFY_HOST   
            #define BTLHL7EXP_SSL_TEST_PEER_VERIFY_HOST_NAME 1  //for this test suite to load the hostname to match for allowing connection
                char gBtlHl7ExpTest_PeerVerifyHostNameMatchStr[] = "myserver.local"; // "myserver.local";
#endif
//=====================================================================================================
//====================== TEST configuration - define only one below (enable SSL above) ================

//#define BTLHL7EXP_TESTCASE_ADHOC 1 //DO NOT ENABLE unless checking new test modifications
#define BTLHL7EXP_TESTCASE_USE_EXDATA 1
//#define BTLHL7EXP_TESTCASE_USE_EXDATA_WITH_USER_RECV_APP 1
//#define BTLHL7EXP_TESTCASE_NO_EXDATA_DEF_VER_2P2 1
//#define BTLHL7EXP_TESTCASE_NO_EXDATA_USER_SET_VER_APP_FAC 1
  //#define BTLHL7EXP_TESTCASE_SERVER_STATUS_CHECK 1
//#define BTLHL7EXP_TESTCASE_SEND_CANCEL 1

#define BTLHL7EXP_TEST_SSL_CERT1 1
    char ownCert1[] = "certs/client.crt";
    char ownKey1[] = "certs/client.key";
    char peerCaCert1[] = "certs/ca.pem";



#ifdef BTL_HL7_FOR_WINDOWS
#include <direct.h>  //for _getcwd()
#else
  //unistd.h already included
#define _getcwd getcwd
#endif
    //Instantiate an export control struct. You may also malloc the size of BtlHl7Export_t instead and 
        //assign to a pointer (BtlHl7Export_t*).
BtlHl7Export_t gBtlHl7export;

//IP address and port of the export server for test
#ifdef WSL_LINUX
char gHl7ExpSrvIpAddrStr[16] = "172.20.80.1"; //"127.0.0.1";
#else
char gHl7ExpSrvIpAddrStr[16] = "127.0.0.1";
#endif
uint16_t gHl7ExpSrvPort = 23856;
//uint16_t gHl7ExpSrvPort = 23727;

//XML Configuration settings file
char gXmlConfigFileName[256] = "..\\xmlConfig\\Btlhl7_config.xml";
//PDF file to be exported for test
char gPdfFileName[256] = "..\\testInputFiles\\hl7Test_1.pdf";

        //BTL XML-NG file for getting meta data for the export (Uncomment one of the following)
char gBtlXmlNgFileName[256] = "..\\testInputFiles\\d60b513e-d780-4ba0-8e57-a66d2ff8d42b.EcgRest.diagnostics.xml"; //EcgRest
//char gBtlXmlNgFileName[256] = "..\\testInputFiles\\84ba0f4d-d75b-4eeb-9004-6a606e6dc563.EcgRhythm-WithResultsTag.diagnostics.xml"; //EcgRhythm with conclusion tag/ doctor's statement
//char gBtlXmlNgFileName[256] = "..\\testInputFiles\\983ff92d-5f75-4530-bc8b-6d3641043d3b.EcgRhythm-NoConclusionTag.diagnostics.xml"; //EcgRhythm without conclusion tag/ doctor's statement
//char gBtlXmlNgFileName[256] = "..\\testInputFiles\\44e5d2c6-521f-4773-8e94-212578975b51.saecg.diagnostics.xml"; //saecg

//Meta data from incoming HL7 workorder for use while exporting
// Actual ProtocolExtraData XML sample created from incoming workorder used below
char gExtraData2a[] = "<ProtocolExtraData Name=\"HL7\">\
<Segment id=\"MSH\">\
<SendingApplication>BTL_ECG2</SendingApplication>\
<SendingFacility></SendingFacility>\
<ReceivingApplication>BTL HIS Simulator</ReceivingApplication>\
<ReceivingFacility>Facility-1^Annex-A</ReceivingFacility>\
<HL7Version>2.3</HL7Version>\
</Segment>\
<Segment id=\"PID\">\
<PatientGuid>fff41a28-aad4-4c7a-96a7-c133ab9b1271</PatientGuid>\
<FirstName>ram</FirstName>\
<MiddleName></MiddleName>\
<LastName>sita</LastName>\
<Title>Mr</Title>\
<BirthDate></BirthDate>\
<Sex>F</Sex>\
<Classification></Classification>\
</Segment>\
<Segment id=\"ORC\">\
<OrderControl>NW</OrderControl>\
<PlacerOrderNumber></PlacerOrderNumber>\
<FillerOrderNumber>1e197810-9ecc-46a8-b906-9f8af451428d</FillerOrderNumber>\
</Segment>\
<Segment id=\"OBR\">\
</Segment>\
<PlacerOrderNumber></PlacerOrderNumber>\
<FillerOrderNumber>1e197810-9ecc-46a8-b906-9f8af451428d</FillerOrderNumber>\
</ProtocolExtraData>";

char gExtraData2[] ="<ProtocolExtraData Name=\"HL7\">\
<Segment id=\"MSH\" >\
<SendingApplication>BTL HIS Simulator</SendingApplication>\
<SendingFacility>Facility-1^Annex-A</SendingFacility>\
<ReceivingApplication>msh5^1234567890</ReceivingApplication >\
<ReceivingFacility>msh6^12345</ReceivingFacility >\
<HL7Version>2.3</HL7Version >\
</Segment>\
<Segment id=\"PID\">\
<PatientGuid>fff41a28-aad4-4c7a-96a7-c133ab9b1271</PatientGuid>\
<FirstName>ram</FirstName>\
<MiddleName></MiddleName>\
<LastName>sita</LastName>\
<Title></Title>\
<BirthDate></BirthDate>\
<Sex>F</Sex>\
<Classification>NortheastAsian</Classification>\
</Segment>\
<Segment id=\"ORC\">\
<OrderControl>NW</OrderControl>\
<PlacerOrderNumber>e72d211a-e90f-4ba4-9090-a2af7e08bbed</PlacerOrderNumber>\
<FillerOrderNumber></FillerOrderNumber>\
</Segment>\
<Segment id=\"OBR\">\
<PlacerOrderNumber>e72d211a-e90f-4ba4-9090-a2af7e08bbed</PlacerOrderNumber>\
<FillerOrderNumber></FillerOrderNumber>\
<UniversalServiceId></UniversalServiceId>\
<RequestedDateTime>20250405192013</RequestedDateTime>\
<ObservationDateTime></ObservationDateTime>\
</Segment>\
</ProtocolExtraData>";

//#########################  Main Test Program Start #########################
//############################################################################

char g_cwdStr[512];  //for getting working directory (for debug)



void myLogger(int logLevel, char* msg) {
    printf("CUSTOM LOG [Level=%d]: %s", logLevel, msg);
    //fputs(msg, stdout);
}

//--------------------------------------------------------
void waitForExportCompletion(BtlHl7Export_t* pHl7Exp) {
    int timeout = 15;
    while (timeout--) {
        Sleep(2000);
        int expStatus = btlHl7GetExportStatus(pHl7Exp);
        if (expStatus == BTLHL7EXP_STATUS_INPROGRESS) {
            printf("Export in progress...\n");

        }
        else if (expStatus == BTLHL7EXP_STATUS_ACK_WAIT) {
            printf("Export done, waiting for ACK...\n");
        }
        else if (expStatus == BTLHL7EXP_STATUS_NO_ERROR) {
            printf("Export completed successfully.\n");
            return;
        }
        else if (expStatus < 0) {
            printf("Export failed with status: %d\n", expStatus);

            return;
        }
        else {
            printf("Status=%d\n", expStatus);
        }
    }
}
void runExportCase(BtlHl7Export_t* pHl7Exp,  char* pdf,  char* diagXml,  char* extraData, int extraLen,  char* testLabel) {
    printf("\n--- Running Test Case: %s ---\n", testLabel);
    int status = btlHl7ExportPdfReport(pHl7Exp, pdf, diagXml, extraData, extraLen);
    if (status != 0) {
     //   printf("Export FAILED for %s: status = %d\n", testLabel, status);
        printf("Export FAILED for %s: status = %d\n", testLabel, status);

        return;
    }
    waitForExportCompletion(pHl7Exp);
    Sleep(3);  //wait before sending next export (only for dummy test server
    return;
}


int main() {

    int extraDataSize;
    int expStatus;
    char* pCwdStr;

    BtlHl7Export_t* pHl7Exp = &gBtlHl7export;
    int expTimeOut;
    printf("Starting BTL HL7 PDF export test program..\n");
    pCwdStr = _getcwd(g_cwdStr, sizeof(g_cwdStr) - 1);
    if (pCwdStr) {
        printf("Executing from directory: %s\n================================\n\n", g_cwdStr);
    }

    //printf("PDF File=%s, Server=%s:%u\n", gPdfFileName, gHl7ExpSrvIpAddrStr, gHl7ExpSrvPort);

    printf("Remote HIS/ PACS Server Ip : Port (initial defaults): %s : %u\n", gHl7ExpSrvIpAddrStr, gHl7ExpSrvPort);
    printf("\nStarting export for PDF file: %s\n", gPdfFileName);

    // Test the logger redirection
    btlHl7ExpRegisterLogCallback(myLogger);  // Register the log callback
   // btlHl7ExpSetLogLevel(BTLHL7EXP_DEFAULT_LOG_LEVEL); //passing 0 will disable all logs, default is no logs
   // btlHl7ExpSetLogLevel(64);  //disable logging
    btlHl7ExpSetLogLevel(BTL_HL7EXP_DBG_LVL_ERR |
        BTL_HL7EXP_DBG_LVL_DBG0 |
        BTL_HL7EXP_DBG_LVL_DBG1);   // = 50

    BTLHL7EXP_DBUG0("Logger test : If you see this, DBUG0 prints are fine (BTL Log level 16)\n");
    BTLHL7EXP_DBUG1("Logger test: If you see this, DBUG1 prints are fine (BTL Log level 32)\n");
    BTLHL7EXP_ERR("Logger test : If you see this, ERR prints are fine(BTL Log level 2)\n");

   //printf("\n############# Disabling logs!  #######################################################\n\n");
    //btlHl7ExpSetLogLevel(0);  //disable logging
   //btlHl7ExpSetLogLevel(32);

#ifdef UNDEF //DO NOT USE BELOW FUNCTIONS : DEPRECATED
    // Case A: both comp1 and comp2
    //btlHl7ExpSetSendingApplicationStr(pHl7Exp, "BTLApp1^Serial123");
   // btlHl7ExpSetSendingFacilityStr(pHl7Exp, "ClinicA^DeptB");

    // Case B: only comp2
     btlHl7ExpSetSendingApplicationStr(pHl7Exp, "^Serial123");
     btlHl7ExpSetSendingFacilityStr(pHl7Exp, "^DeptB");

    // Case C: only comp1
    // btlHl7ExpSetSendingApplicationStr(pHl7Exp, "BTLApp1");
    // btlHl7ExpSetSendingFacilityStr(pHl7Exp, "ClinicA");

#endif


    // Initialize export module
    //ipaddr and port can be null.it can be later configured using btlHl7ExpSetSrvIp()
    //Init function below will start a thread for performing the export
    // only one export can be triggered at a time and next one can be triggered only after its completion
    btlHl7ExportInit(pHl7Exp, gHl7ExpSrvIpAddrStr, gHl7ExpSrvPort);

    // ip address and port can be set at any time using below api call
   // btlHl7ExpSetSrvIp(pHl7Exp, gHl7ExpSrvIpAddrStr, gHl7ExpSrvPort);
    
    //optionally set the server timeout (how much time to wait for server responses)
    //Default is 30 seconds if not configured with below call



#ifdef BTLHL7EXP_TEST_SSL_EN
    btlHl7ExpEnableSsl(pHl7Exp, BTLHL7EXP_SSL_ENABLE); 

    #ifdef BTLHL7EXP_TEST_SSL_CERT1
        btlHl7ExpSslSetOwnCertFile(pHl7Exp, ownCert1);      //required if server verifies client by its certificate
        btlHl7ExpSslSetOwnKeyFile(pHl7Exp, ownKey1);        //required if server verifies client by its certificate
        btlHl7ExpSslSetPeerCaFile(pHl7Exp, peerCaCert1);    //required if client needs to verify the server certificate, hostname, IPaddress etc
        //btlHl7ExpSslSetPeerCaFile(pHl7Exp, ownCert1);    //Negative test - this will fail SSL handshake as we pass the client cert instead of peer CA

    #endif
    #ifdef BTLHL7EXP_TEST_SSL_USE_OWN_CERT_EN
        btlHl7ExpSslUseOwnCertificate(pHl7Exp, BTLHL7EXP_SSL_ENABLE); ///enables the use of a client certificate and key, both must be set by user
    #endif

    #ifdef BTLHL7EXP_TEST_SET_PEER_VERIFY_MODE_EN
            //enables/ disables verifcation of server (remote's) certificate and optionally ipAddress OR hostname (based on BTLHL7EXP_SSL_TEST_PEER_VERIFY_MODE)
            // BTLHL7EXP_SSL_VERIFY_NONE=disable verification of peer certificate, ipaddress/hostname
            // BTLHL7EXP_SSL_VERIFY_CA=verify peer certificate (skip ipaddress/hostname verification)
            // BTLHL7EXP_SSL_VERIFY_IP=verify IP address (and verify peer certificate)
            // BTLHL7EXP_SSL_VERIFY_HOST=verify hostname (and verify peer certificate)
            //NOTE: SSL handshake/connection will fail if other than BTLHL7EXP_SSL_VERIFY_NONE is set and an appropriate peer CA file is not set
                    //WARNING: if hostname to match is set to empty string (default), hostname match will be DISABLED inspite of BTLHL7EXP_SSL_VERIFY_HOST being set!
        btlHl7ExpSslSetPeerVerifyMode(pHl7Exp, BTLHL7EXP_SSL_TEST_PEER_VERIFY_MODE);
    #endif
    #ifdef BTLHL7EXP_SSL_TEST_PEER_VERIFY_HOST_NAME
        btlHl7ExpSslSetPeerHostMatchStr(pHl7Exp, gBtlHl7ExpTest_PeerVerifyHostNameMatchStr);
    #endif

    #ifdef BTLHL7EXP_TEST_LOAD_CFG_FROM_XML
        btlHl7ExpLoadXmlConfig(pHl7Exp, gXmlConfigFileName);
    #endif
#endif
#ifdef  BTLHL7EXP_TESTCASE_ADHOC 
    
    //btlHl7ExpLoadXmlConfig(pHl7Exp, gXmlConfigFileName);
 //   runExportCase(pHl7Exp, gPdfFileName, gBtlXmlNgFileName, NULL, 0, "TESTCASE : Config from Xml ");
    extraDataSize = (int)strlen(gExtraData2);
    //runExportCase(pHl7Exp, gPdfFileName, gBtlXmlNgFileName, gExtraData2, extraDataSize, "TESTCASE : Config from Xml along with ExtraData");

    //runExportCase(pHl7Exp, gPdfFileName, gBtlXmlNgFileName, 0, 0, "TESTCASE : Config from Xml + NO ExtraData");
    // new — pass the extra data buffer + length
  // runExportCase(pHl7Exp, gPdfFileName, gBtlXmlNgFileName, gExtraData2, (int)strlen(gExtraData2), "TESTCASE : Config from Xml + ExtraData");

    //Case with no BtlXmlNg
 //  runExportCase(pHl7Exp, gPdfFileName, 0 , gExtraData2, (int)strlen(gExtraData2), "TESTCASE : Config from Xml + ExtraData");

    //goto test_end;
    
     //runExportCase(pHl7Exp,
               //   gPdfFileName,
               ////   NULL,                 // Pass NULL for BtlXmlNg file
                //  gExtraData2,
                //  (int)strlen(gExtraData2),
                 // "CASE 2 - PED only (No XML)");
                  
#endif
   // btlHl7ExpSetDebugLevel(pHl7Exp, 1);   // or higher


   // btlHl7ExpSetServerTimeout(pHl7Exp, 20); //20 seconds timeout set

#ifdef BTLHL7EXP_TESTCASE_SERVER_STATUS_CHECK



    // --- Test server status check API ---
    printf("\nRunning Server Status Check Test...\n");
    btlHl7ExpCheckServerStatus(pHl7Exp);

    btlHl7ExpSetProcessingId(pHl7Exp, "T"); // Training mode
 
    // Wait until the check completes
    int timeout = 10;
    while (timeout--) {
        Sleep(1000);
        int srvStatus = btlHl7GetExportStatus(pHl7Exp);

        if (srvStatus == BTLHL7EXP_STATUS_NO_ERROR) {
            printf("Server status check: SUCCESS (TCP connectable)\n");
            break;
        }
        else if (srvStatus < 0) {
            printf("Server status check: FAILED, status=%d\n", srvStatus);
            break;
        }
        else {
            printf("Server status check in progress...\n");
        }
    }

#endif

    //########### Now the export client is ready for performing export operations

#ifdef BTLHL7EXP_TESTCASE_USE_EXDATA
  //Start and monitor an export operation to its completion
    printf("CASE 1 - Normal flow - using HL7 version, Receiving Application etc from ProtocolExtraData XML.\n");
    extraDataSize = (int)strlen(gExtraData2);

    // Start the export using PDF path, diagnostics XML, and extra data
    expStatus = btlHl7ExportPdfReport(pHl7Exp, gPdfFileName, gBtlXmlNgFileName, gExtraData2, extraDataSize);
  

    if (expStatus != 0) {
        if (expStatus == BTLHL7EXP_STATUS_BUSY) {
            printf("BTLHL7EXP ERROR: Export Pdf failed: Previous export not yet completed!\n");
        }
        else if (expStatus == BTLHL7EXP_STATUS_INPUT_ERR) {
            printf("BTLHL7EXP ERROR: Export Pdf failed: Invalid inputs!\n");
        }
        else {
            printf("BTLHL7EXP ERROR: Export Pdf failed: %d\n", expStatus);
        }
        return 1;
    }
    //-------------
    waitForExportCompletion(pHl7Exp);

    //goto test_end;
#endif


    
#ifdef BTLHL7EXP_TESTCASE_SEND_CANCEL
  //Start and monitor an export operation to its completion
    printf("CASE n - Cancel test.\n");
    extraDataSize = (int)strlen(gExtraData2);


    // Start the cancel using extra data
   
    expStatus = btlHl7ExpSendOrderCancelled(&gBtlHl7export, gExtraData2, extraDataSize);
  

    if (expStatus != 0) {
        if (expStatus == BTLHL7EXP_STATUS_BUSY) {
            printf("BTLHL7EXP ERROR: Export Pdf failed: Previous export not yet completed!\n");
        }
        else if (expStatus == BTLHL7EXP_STATUS_INPUT_ERR) {
            printf("BTLHL7EXP ERROR: Export Pdf failed: Invalid inputs!\n");
        }
        else {
            printf("BTLHL7EXP ERROR: Export Pdf failed: %d\n", expStatus);
        }
        return 1;
    }
    //-------------
    waitForExportCompletion(pHl7Exp);

    goto test_end;
#endif



#ifdef BTLHL7EXP_TESTCASE_USE_EXDATA_WITH_USER_RECV_APP
    //Start and monitor an export operation to its completion
    printf("CASE 1 -Using metadata from ProtocolExtraData XML except ReceivingApplication, Facility and Version.\n");
    extraDataSize = (int)strlen(gExtraData2);

    btlHl7ExpSetReceivingApplicationStr(pHl7Exp, "UserSetRecvApplication");
    btlHl7ExpSetReceivingFacilityStr(pHl7Exp, "UserSetRecvFacility");
    btlHl7ExpSetHl7Version(pHl7Exp, "2.5.1");

    // Start the export using PDF path, diagnostics XML, and extra data
    expStatus = btlHl7ExportPdfReport(pHl7Exp, gPdfFileName, gBtlXmlNgFileName, gExtraData2, extraDataSize);


    if (expStatus != 0) {
        if (expStatus == BTLHL7EXP_STATUS_BUSY) {
            printf("BTLHL7EXP ERROR: Export Pdf failed: Previous export not yet completed!\n");
        }
        else if (expStatus == BTLHL7EXP_STATUS_INPUT_ERR) {
            printf("BTLHL7hhEXP ERROR: Export Pdf failed: Invalid inputs!\n");
        }
        else {
            printf("BTLHL7EXP ERROR: Export Pdf failed: %d\n", expStatus);
        }
        return 1;
    }
    //-------------
    waitForExportCompletion(pHl7Exp);

    goto test_end;
#endif

  
#ifdef BTLHL7EXP_TESTCASE_NO_EXDATA_DEF_VER_2P2

    //  CASE 2: ProtocolExtraData = NULL  fallback, expect MSH, PID, OBX, version 2.2
    runExportCase(pHl7Exp, gPdfFileName, gBtlXmlNgFileName, NULL, 0, "CASE 2 - Fallback (Default 2.2)");

    // After export completes, send order status notifications
    printf("\n--- Sending Order Status Notifications ---\n");
    int osStatus;

    osStatus = btlHl7ExpSendOrderStatus(pHl7Exp, "IP");  // In Progress
    printf("OrderStatus [IP] result = %d\n", osStatus);

    osStatus = btlHl7ExpSendOrderStatus(pHl7Exp, "CM");  // Completed
    printf("OrderStatus [CM] result = %d\n", osStatus);

    goto test_end;

#endif
#ifdef BTLHL7EXP_TESTCASE_NO_EXDATA_USER_SET_VER_APP_FAC

    //  CASE 3: Fallback and  Version override
    btlHl7ExpSetReceivingApplicationStr(pHl7Exp, "UserSetRecvApplication");
    btlHl7ExpSetReceivingFacilityStr(pHl7Exp, "UserSetRecvFacility");
    btlHl7ExpSetSendingApplicationStr(pHl7Exp, "UserSetSendingApplication");
    btlHl7ExpSetSendingFacilityStr(pHl7Exp, "UserSetSendingFacility");
    btlHl7ExpSetHl7Version(pHl7Exp, "2.5.1");
        //If required change the OBX field #3 value to "application/pdf"
            //Default is as used in CONNECTin : "PDF^ReportFileData"
    btlHl7ExpSetObx3PdfFileTagStr(pHl7Exp, "application/pdf");

    runExportCase(pHl7Exp, gPdfFileName, gBtlXmlNgFileName, NULL, 0, "CASE 3 - Fallback with Version Override (2.5.1)");
    goto test_end;

#endif
    //=======================================================================
    //=======================================================================

    test_end:
    expTimeOut = 10;
    btlHl7ExpShutdown(pHl7Exp);
    while (expTimeOut--) {

        Sleep(200);//200 millisecond sleep
        if (btlHl7ExpIsThreadRunning(pHl7Exp)) {
            continue;
        }
        else {
            printf("BTLHL7EXP TEST: Client thread shutdown completed!\n");
            return 0;
        }
    }
    printf("BTLHL7EXP TEST: Client thread shutdown failed!\n");
    return 0;
}  //main()
#endif
