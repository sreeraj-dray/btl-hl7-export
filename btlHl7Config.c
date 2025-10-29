#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "btlHl7EcgExport.h"
#include "btlHl7Config.h"
#include "btlHl7ExpDebug.h"  // include debug macros


static char resultStatusStr1[] = { "P" };
static char resultStatusStr2[] = { "F" };
static char resultStatusStr3[] = { "C" };
static char* resultStatusStrings[] = { 
    resultStatusStr1,
    resultStatusStr2,
    resultStatusStr3,
    NULL
};


int btlHl7ExpLoadXmlConfig(BtlHl7Export_t* pHl7Exp, char* xmlFilePath) {
    xmlDoc* doc = xmlReadFile(xmlFilePath, NULL, 0);
    if (!doc) {
        BTLHL7EXP_ERR("[BTL_HL7_EXP XML Config] ERROR: Could not load config file %s\n", xmlFilePath);
        return BTLHL7EXP_STATUS_ERR_CFG_XML_READ;
    }

    xmlNode* root = xmlDocGetRootElement(doc);
    if (!root || xmlStrcmp(root->name, (xmlChar*)"btlHl7Config") != 0) {
        BTLHL7EXP_ERR("[BTL_HL7_EXP XML Config] ERROR: Invalid root element in config\n");
        xmlFreeDoc(doc);
        return BTLHL7EXP_STATUS_ERR_CFG_XML_ROOTNODE;
    }


    // Temporary storage for Worklist Server IP/Port
    char tempServerIp[64] = { 0 };
    int  tempServerPort = 0;
    // Iterate sections: Worklist + Export
    for (xmlNode* section = root->children; section; section = section->next) {
        if (section->type != XML_ELEMENT_NODE) {
            continue;
        }
        // ============================
        // ---- Export Config ----
        // ============================
        if (xmlStrcmp(section->name, ( xmlChar*)"btlHl7PdfExportConfig") == 0) {
            for (xmlNode* cfg = section->children; cfg; cfg = cfg->next) {
                if (cfg->type != XML_ELEMENT_NODE) {
                    continue;
                }

                xmlChar* idAttr = xmlGetProp(cfg, ( xmlChar*)"id");
                if (!idAttr) {
                    continue;
                }

                int id = atoi(( char*)idAttr);
                xmlFree(idAttr);

                if (id <= 0 || id >= EXP_CFG_LAST_ID) {
                    continue;
                }

                xmlChar* valText = xmlNodeGetContent(cfg);

                switch (id) {
                case EXP_CFG_SERVER_IP: {
                    if (valText == 0) {
                        tempServerIp[0] = 0;
                        break;
                    }
                    strncpy(tempServerIp, (char*)valText, sizeof(tempServerIp) - 1);
                    tempServerIp[sizeof(tempServerIp) - 1] = '\0';
                    BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] ServerIP = %s\n", tempServerIp);
                    break;
                }
                case EXP_CFG_SERVER_PORT: {
                    if (!valText) {
                        break;
                    }
                    tempServerPort = atoi((char*)valText);
                    BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] ServerPort = %d\n", tempServerPort);
                    break;
                }
                case EXP_CFG_SERVER_TIMEOUT: {
                    if (!valText) {
                        break;
                    }
                    int timeout = atoi((char*)valText);
                    if (timeout < BTLHL7EXP_SRV_TIMEOUT_MIN) {
                        BTLHL7EXP_ERR("[BTLHL7EXP XML Config] ERROR: Invalid ServerTimeoutSeconds value (%d), must be more than %d\n", timeout, BTLHL7EXP_SRV_TIMEOUT_MIN);
                        break;
                    }
                    btlHl7ExpSetServerTimeout(pHl7Exp, timeout);
                    BTLHL7EXP_DBUG1("[BTLHL7EXP XML Config] ServerTimeoutSeconds = %d\n", timeout);
                    break;
                }
                case EXP_CFG_PDF_CHUNK_SIZE: {
                    if (!valText) {
                        break;
                    }
                    int chunkSize = atoi((char*)valText);
                    if (chunkSize < BTLHL7EXP_MIN_PDF_CHUNK_SIZE || chunkSize > BTLHL7EXP_MAX_PDF_CHUNK_SIZE) {
                        BTLHL7EXP_ERR("[BTLHL7EXP XML Config] ERROR: Invalid pdf file chunk size (%d), must be between %d and %d\n", chunkSize, BTLHL7EXP_MIN_PDF_CHUNK_SIZE, BTLHL7EXP_MAX_PDF_CHUNK_SIZE);
                        break;
                    }
                    btlHl7ExpSetPdfChunkSize(pHl7Exp, chunkSize);
                    BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] PdfChunkSize = %s\n", valText);
                    break;
                }
                case EXP_CFG_ACK_TIMEOUT: {
                    if (!valText) {
                        break;
                    }
                    int timeout = atoi((char*)valText);
                    if (timeout < BTLHL7EXP_SRV_TIMEOUT_MIN) {
                        BTLHL7EXP_ERR("[BTLHL7EXP XML Config] ERROR: Invalid AckTimeoutSeconds value (%d), must be more than %d\n", timeout, BTLHL7EXP_SRV_TIMEOUT_MIN);
                        break;
                    }
                    btlHl7ExpSetAckTimeout(pHl7Exp, timeout);
                    BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] AckTimeoutSeconds = %d\n", timeout);
                    break;
                }
                case EXP_CFG_DEBUG_LEVEL: {
                    if (valText) {
                        int logLevel = atoi((char*)valText);
                        BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] Debug Log Level = %s (0x%x)\n", valText, logLevel);
                        if (logLevel == 0) {
                            BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] WARNING: ALL LOGS ARE DISABLED INCLUDING ERROR LOGS!\n");
                        }
                        btlHl7ExpSetLogLevel(atoi((char*)valText));
                    }
                    break;
                }

                case EXP_CFG_SENDING_APP: {
                    xmlChar* NamespaceID = xmlGetProp(cfg, ( xmlChar*)"NamespaceID");
                    xmlChar* UniversalID = xmlGetProp(cfg, ( xmlChar*)"UniversalID");
                    btlHl7ExpSetSendingApplication(pHl7Exp, ( char*)NamespaceID, ( char*)UniversalID);

                    if (NamespaceID) {
                        BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] SendingApplication NamespaceID = %s\n", NamespaceID);
                        xmlFree(NamespaceID);
                    }
                    if (UniversalID) {
                        BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] SendingApplication UniversalID = %s\n", UniversalID);
                        xmlFree(UniversalID);
                    }
                    break;
                }
                case EXP_CFG_SENDING_FACILITY: {
                    xmlChar* NamespaceID = xmlGetProp(cfg, ( xmlChar*)"NamespaceID");
                    xmlChar* UniversalID = xmlGetProp(cfg, ( xmlChar*)"UniversalID");
                    btlHl7ExpSetSendingFacility(pHl7Exp, ( char*)NamespaceID, ( char*)UniversalID);

                    if (NamespaceID) {
                        BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] SendingFacility NamespaceID = %s\n", NamespaceID);
                        xmlFree(NamespaceID);
                    }
                    if (UniversalID) {
                        BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] SendingFacility UniversalID = %s\n", UniversalID);
                        xmlFree(UniversalID);
                    }

                    break;
                }
                case EXP_CFG_RECEIVING_APP: {
                    xmlChar* NamespaceID = xmlGetProp(cfg, ( xmlChar*)"NamespaceID");
                    xmlChar* UniversalID = xmlGetProp(cfg, ( xmlChar*)"UniversalID");
                    btlHl7ExpSetReceivingApplication(pHl7Exp, ( char*)NamespaceID, ( char*)UniversalID);


                    if (NamespaceID) {
                        BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] ReceivingApplication NamespaceID = %s\n", NamespaceID);
                        xmlFree(NamespaceID);
                    }
                    if (UniversalID) {
                        BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] ReceivingApplication UniversalID = %s\n", UniversalID);
                        xmlFree(UniversalID);
                    }

                    break;
                }
                case EXP_CFG_RECEIVING_FACILITY: {
                    xmlChar* NamespaceID = xmlGetProp(cfg, ( xmlChar*)"NamespaceID");
                    xmlChar* UniversalID = xmlGetProp(cfg, ( xmlChar*)"UniversalID");
                    btlHl7ExpSetReceivingFacility(pHl7Exp, ( char*)NamespaceID, ( char*)UniversalID);

                    if (NamespaceID) {
                        BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] ReceivingFacility NamespaceID = %s\n", NamespaceID);
                        xmlFree(NamespaceID);
                    }
                    if (UniversalID) {
                        BTLHL7EXP_DBUG1("[BTL_HL7_EXP XML Config] ReceivingFacility UniversalID = %s\n", UniversalID);
                        xmlFree(UniversalID);
                    }

                    break;
                }
                case EXP_CFG_USE_SEP_OBX: {
                    if (!valText) {
                        break;
                    }
                    
                    if (strncasecmp((char*)valText, "Yes", 3) == 0) {
                        btlHl7ExpSetObxEcgMeasCfg(pHl7Exp, BTLHL7EXP_CFG_OBX_ECG_MEAS_MULTI);
                        BTLHL7EXP_DBUG1("[BTLHL7EXP XML Config] UseSeperateObxEcgMeas = %s\n", valText);

                    }
                    else if (strncasecmp((char*)valText, "No", 2) == 0) {
                        btlHl7ExpSetObxEcgMeasCfg(pHl7Exp, BTLHL7EXP_CFG_OBX_ECG_MEAS_SINGLE);
                        BTLHL7EXP_DBUG1("[BTLHL7EXP XML Config] UseSeperateObxEcgMeas = %s\n", valText);
                    } else{
                        //neither yes nor no , invalid input 
                        BTLHL7EXP_ERR("[BTLHL7EXP XML Config] ERROR: Invalid UseSeperateObxEcgMeas value (%s), must be Yes or No \n", valText);
                  
                    }
                   
                    break;
                }//case EXP_CFG_USE_SEP_OBX

                case EXP_CFG_OBX_RESULT_STATUS: {
                    if (!valText) {
                        break;
                    }
                    char* matchedStr = btlHl7ExpGetMatchedString((char*)valText, resultStatusStrings, (int)strlen(valText));

                    if (matchedStr == NULL) {
                        // invalid input
                        BTLHL7EXP_ERR("[BTLHL7EXP XML Config] ERROR: Invalid ObxResultStatus value (%s), must be P, F, or C\n", valText);
                        break;
                    }
                    btlHl7ExpSetObxResultStatus(pHl7Exp, matchedStr);
                    BTLHL7EXP_DBUG1("[BTLHL7EXP XML Config] ObxResultStatus = %s\n", matchedStr);
                                         
                    break;
                } // case EXP_CFG_OBX_RESULT_STATUS


                default: {
                    break;
                }
                }

                if (valText) { 
                    xmlFree(valText); 
                    valText = 0;
                }

            }  //for loop iterating export config nodes
        }  //if xml node is that of export config



    }  //for loop iterating top level child nodes

                // After parsing, set IP and Port
    if (strlen(tempServerIp) > 0 && tempServerPort > 0) {
        btlHl7ExpSetSrvIp(pHl7Exp, tempServerIp, (uint16_t)tempServerPort);
        BTLHL7EXP_DBUG1("BTLHL7WLSRV: [WL XML Config] IP/Port set: %s:%d\n", tempServerIp, tempServerPort);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 0;
}
