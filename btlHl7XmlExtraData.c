//File: btlHl7XmlExtraData.c
//To parse the data collected from received work order HL7 message for use
//while exporting pdf report

#include "btlHl7XmlExtraData.h"
#include "btlHl7XmlNg.h"

char* btlHl7GetNodeText(xmlNode* root, char* tag) {
    for (xmlNode* cur = root; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE && strcmp(cur->name, tag) == 0) {
            if (cur->children && cur->children->content) {
                return cur->children->content;
            }
        }
        char* result = btlHl7GetNodeText(cur->children, tag);
        if (result) {
            return result;
        }
    }
    return NULL;
}
// Utility: split "ABC^XYZ" into comp1="ABC", comp2="XYZ"
static void splitComponent(const char* src, char* comp1, char* comp2, size_t bufSize) {
    if (!src) { comp1[0] = comp2[0] = '\0'; return; }
    const char* sep = strchr(src, '^');
    if (sep) {
        size_t len1 = sep - src;
        if (len1 >= bufSize) len1 = bufSize - 1;
        strncpy_s(comp1, bufSize, src, len1);
        strncpy_s(comp2, bufSize, sep + 1, _TRUNCATE);
    }
    else {
        strncpy_s(comp1, bufSize, src, _TRUNCATE);
        comp2[0] = '\0';
    }
}



int btlHl7ParseProtocolExtraData(BtlHl7Export_t* pHl7Exp){
    int nCharWritten;
    char* xml = pHl7Exp->protocolExtraData;
    char* outBuf = pHl7Exp->pHl7BufNext;
    int outBufRemaining = pHl7Exp->outputHl7SizeLeft;
    int xmlSize = pHl7Exp->protocolExtraDataLen;
    pHl7Exp->universalServiceId[0] = 0;

    xmlDoc* doc = xmlReadMemory(xml, xmlSize, "noname.xml", NULL, 0);
    if (!doc) {
        BTLHL7EXP_ERR("BTLHL7EXP: Error: failed to parse protocol extra data XML!\n");
        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_EXDATA_XML_ERR;
        return -1;
    }

    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* node = root->children; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE || 
            strcmp(( char*)node->name, "Segment") != 0) {
            continue;
        }
         char* id = ( char*)xmlGetProp(node, ( xmlChar*)"id");
        if (!id) { 
            continue;
        }

            //char* pSendingApp = pHl7Exp->sendingApplication;
      //  char* pSendingFac = pHl7Exp->sendingFacility;
        char* pReceivingApp = pHl7Exp->receivingApplication;
        char* pReceivingFac = pHl7Exp->receivingFacility;
        char* pHl7Version = pHl7Exp->hl7VersionStr;

        if (strcmp(id, "MSH") == 0) {
#ifdef UNDEF  // sending facility and application always comes from xmlconfiguration , hence below code is diabled      
            // #### MODIFIED: parse SendingApplication / SendingFacility into comp1, comp2
            char* xmlSendingApp = btlHl7GetNodeText(node, "SendingApplication");
            char* xmlSendingFac = btlHl7GetNodeText(node, "SendingFacility");

            // If user already set values, keep them. Else take from XML if available.
            if (pHl7Exp->sendingApplicationComp1[0] == 0 && pHl7Exp->sendingApplicationComp2[0] == 0) {
                if (xmlSendingApp) {
                    splitComponent(xmlSendingApp,
                        pHl7Exp->sendingApplicationComp1, //TBDNOWBUG  writing into user setting , must use local buffer 
                        pHl7Exp->sendingApplicationComp2,//TBDNOWBUG  writing into user setting , must use local buffer 
                        BTL_HL7_NAME_BUF_SIZE);
                }
            }
            //TBDNOWBUG sending fac should not be taken from ped it should be taken from user setting even if it is empty
            if (pHl7Exp->sendingFacilityComp1[0] == 0 && pHl7Exp->sendingFacilityComp2[0] == 0) {
                if (xmlSendingFac) {
                    splitComponent(xmlSendingFac,
                        pHl7Exp->sendingFacilityComp1,//TBDNOWBUG  writing into user setting , must use local buffer 
                        pHl7Exp->sendingFacilityComp2,//TBDNOWBUG  writing into user setting , must use local buffer 
                        BTL_HL7_NAME_BUF_SIZE);
                }
            }
#endif
                //if user has set either/both of recv appln, facility, then use those,else
                    //use the values from protocol extra data if present, else empty string
            if (pReceivingApp[0] != 0 || pReceivingFac[0] != 0) {
                //user has set the value(s)
            }else {
                pReceivingApp = btlHl7GetNodeText(node, "ReceivingApplication");
                if (!pReceivingApp) {
                    pReceivingApp = pHl7Exp->receivingApplication;
                }

                pReceivingFac = btlHl7GetNodeText(node, "ReceivingFacility");
                if (!pReceivingFac) {
                    pReceivingFac = pHl7Exp->receivingFacility;
                }

            }
                //Determine the version to use. If user has set a value, then use that, else
                    //use the value from protocol extra data if available, else use default
            if (pHl7Version[0] == 0) {
                    //user has not set a version
                pHl7Version = btlHl7GetNodeText(node, "HL7Version");
                if (!pHl7Version) {
                        //No version in protocol extra data xml, use that set by user
                    pHl7Version = pHl7Exp->hl7VersionStr;
                }
                if (pHl7Version[0] == 0) {
                        //No version from XML, not set by user either - so use default
                    pHl7Version = pHl7Exp->hl7VersionDefaultStr;
                }
            }
 
    // Default message type for export
             const char* msgType = "ORU^R01";

             // Append message structure for version >= 2.3
             if (pHl7Version && (strcmp(pHl7Version, "2.3") >= 0)) {
                 msgType = "ORU^R01^ORU_R01";
             }

             // #### ADDED: rebuild sendingAppField and sendingFacField with ^
             char sendingAppField[2 * BTL_HL7_NAME_BUF_SIZE] = "";
             char sendingFacField[2 * BTL_HL7_NAME_BUF_SIZE] = "";
#ifdef _WIN32
             // Windows uses strcat_s
             if (pHl7Exp->sendingApplicationComp1[0] != '\0') {
                 strcat_s(sendingAppField, sizeof(sendingAppField), pHl7Exp->sendingApplicationComp1);
             }
             if (pHl7Exp->sendingApplicationComp2[0] != '\0') {
                 if (sendingAppField[0] != '\0')
                     strcat_s(sendingAppField, sizeof(sendingAppField), "^");
                 strcat_s(sendingAppField, sizeof(sendingAppField), pHl7Exp->sendingApplicationComp2);
             }

             if (pHl7Exp->sendingFacilityComp1[0] != '\0') {
                 strcat_s(sendingFacField, sizeof(sendingFacField), pHl7Exp->sendingFacilityComp1);
             }
             if (pHl7Exp->sendingFacilityComp2[0] != '\0') {
                 if (sendingFacField[0] != '\0')
                     strcat_s(sendingFacField, sizeof(sendingFacField), "^");
                 strcat_s(sendingFacField, sizeof(sendingFacField), pHl7Exp->sendingFacilityComp2);
             }

#else
             // Linux uses strncat with bounds
             if (pHl7Exp->sendingApplicationComp1[0] != '\0') {
                 strncat(sendingAppField, pHl7Exp->sendingApplicationComp1,
                     sizeof(sendingAppField) - strlen(sendingAppField) - 1);
             }
             if (pHl7Exp->sendingApplicationComp2[0] != '\0') {
                 if (sendingAppField[0] != '\0')
                     strncat(sendingAppField, "^", sizeof(sendingAppField) - strlen(sendingAppField) - 1);
                 strncat(sendingAppField, pHl7Exp->sendingApplicationComp2,
                     sizeof(sendingAppField) - strlen(sendingAppField) - 1);
             }

             if (pHl7Exp->sendingFacilityComp1[0] != '\0') {
                 strncat(sendingFacField, pHl7Exp->sendingFacilityComp1,
                     sizeof(sendingFacField) - strlen(sendingFacField) - 1);
             }
             if (pHl7Exp->sendingFacilityComp2[0] != '\0') {
                 if (sendingFacField[0] != '\0')
                     strncat(sendingFacField, "^", sizeof(sendingFacField) - strlen(sendingFacField) - 1);
                 strncat(sendingFacField, pHl7Exp->sendingFacilityComp2,
                     sizeof(sendingFacField) - strlen(sendingFacField) - 1);
             }
#endif

             // Debug print to confirm recombined fields
             printf("DEBUG: Recombined Sending Application: %s\n", sendingAppField);
             printf("DEBUG: Recombined Sending Facility: %s\n", sendingFacField);

             
             nCharWritten = snprintf(outBuf, outBufRemaining,
                 "MSH|^~\\&|%s|%s|%s|%s|%s||%s|%s|%s|%s||||||UNICODE UTF-8\r",
                 sendingAppField,
                 sendingFacField,
                 pReceivingApp,
                 pReceivingFac,
                 pHl7Exp->msgTimestampStrBuf,
                 msgType,
                 pHl7Exp->msgControlIdStrBuf,
                 pHl7Exp->processingId,   
                 pHl7Version);




            if (nCharWritten < 0) {
                printf("BTLHL7EXP: Error: Failed HL7 buffer write (MSH)!\n");
                pHl7Exp->exportStatus = BTLHL7EXP_STATUS_PROCESSING_ERR;
                return -1;
            }
            outBuf += nCharWritten;
            outBufRemaining -= nCharWritten;
            
        }

        else if (strcmp(id, "PID") == 0) {
            btlHl7ExpPidData_t pidData;

             pidData.pid = btlHl7GetNodeText(node, "PatientGuid");
             pidData.fn = btlHl7GetNodeText(node, "FirstName");
             pidData.mn = btlHl7GetNodeText(node, "MiddleName");
             pidData.ln = btlHl7GetNodeText(node, "LastName");
             pidData.title = btlHl7GetNodeText(node, "Title");
             pidData.dob = btlHl7GetNodeText(node, "BirthDate");
             pidData.sex = btlHl7GetNodeText(node, "Sex");
             pidData.cls = btlHl7GetNodeText(node, "Classification");

             int errStat;
             outBuf=btlHl7ExpPutPidToMsg(outBuf, &outBufRemaining, &pidData, &errStat);
#ifdef UNDEF
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
             if (title && title[0] != 0) {
                 namePresent = 1;
             }
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
#endif
        }
                else if (strcmp(id, "ORC") == 0) {
                     char* orderControl = btlHl7GetNodeText(node, "OrderControl");
                     char* placer = btlHl7GetNodeText(node, "PlacerOrderNumber");
                    // char* filler = btlHl7GetNodeText(node, "FillerOrderNumber");
                   //  char* reqTime = btlHl7GetNodeText(node, "RequestedDateTime");  // for ORC-9, we should use the examination conducted time from btlxmlNg
                     char* orderStatus = (pHl7Exp->orderStatusStr[0] != '\0')
                                ? pHl7Exp->orderStatusStr
                                : "CM";  // default  ORC-5 = CM (Complete). 'F' can be set via XML or API


                    if (placer && placer[0]) {
                        strncpy_s(pHl7Exp->placerOrderNumber, sizeof(pHl7Exp->placerOrderNumber), placer, _TRUNCATE);
                    }
                    else {
                        pHl7Exp->placerOrderNumber[0] = 0;
                    }
                    const char* ocField = (pHl7Exp->_orderControl[0]) ? pHl7Exp->_orderControl : "RE";
                    const char* placerField = (pHl7Exp->placerOrderNumber[0]) ? pHl7Exp->placerOrderNumber : "";
                    const char* fillerField = (pHl7Exp->fillerOrderNumber[0]) ? pHl7Exp->fillerOrderNumber : "";

                    nCharWritten = snprintf(outBuf, outBufRemaining,
                        "ORC|%s|%s|%s||%s|||||%s\r",
                        ocField,
                        placerField,
                        fillerField,
                        orderStatus,
                        pHl7Exp->examPerformedTimeStr[0] ? pHl7Exp->examPerformedTimeStr : "");

                    if (nCharWritten < 0) {
                        printf("BTLHL7EXP: Error: Failed HL7 buffer write-ORC!\n");
                        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_PROCESSING_ERR;
                        return -1;
                    }
                    outBuf += nCharWritten;
                    outBufRemaining -= nCharWritten;
                    

                    // ---------------------------------------------------------
                    //  ECG measurement OBX segments --------
                   _btlHl7ExpUpdateBufCtl(pHl7Exp, outBufRemaining);
                  
                    outBuf = pHl7Exp->pHl7BufNext;
                    outBufRemaining = pHl7Exp->outputHl7SizeLeft;

        }
                else if (strcmp(id, "OBR") == 0) {
                    char* placer = btlHl7GetNodeText(node, "PlacerOrderNumber");
                    char* filler = btlHl7GetNodeText(node, "FillerOrderNumber");
                    char* usid = btlHl7GetNodeText(node, "UniversalServiceId");
                    char* reqTime = btlHl7GetNodeText(node, "RequestedDateTime");
                  //  char* obsTime = btlHl7GetNodeText(node, "ObservationDateTime");// this should come from btlxmlNg not PEd
                    pHl7Exp->placerOrderNumber[0] = '\0';
                    /* Save to struct for reuse later (if ORC didn’t have it) */
                    if (placer && placer[0]) {
                       strncpy_s(pHl7Exp->placerOrderNumber, sizeof(pHl7Exp->placerOrderNumber), placer, _TRUNCATE);
                    }
                      pHl7Exp->universalServiceId[0] = '\0';
                    if (usid && usid[0])
                        strncpy_s(pHl7Exp->universalServiceId, sizeof(pHl7Exp->universalServiceId), usid, _TRUNCATE);
                   // if (reqTime && reqTime[0])
                   //     strncpy_s(pHl7Exp->requestedDateTime, sizeof(pHl7Exp->requestedDateTime), reqTime, _TRUNCATE);
                    //if (obsTime && obsTime[0])
                      //  strncpy_s(pHl7Exp->observationDateTime, sizeof(pHl7Exp->observationDateTime), obsTime, _TRUNCATE);
                    pHl7Exp->orderRequestedTimeStr[0] = 0;
                    if (reqTime && reqTime[0])
                        strncpy_s(pHl7Exp->orderRequestedTimeStr, sizeof(pHl7Exp->orderRequestedTimeStr), reqTime, _TRUNCATE);
                   
                       


                   const char* placerField = (pHl7Exp->placerOrderNumber[0]) ? pHl7Exp->placerOrderNumber : "";
                    const char* fillerField = (pHl7Exp->fillerOrderNumber[0]) ? pHl7Exp->fillerOrderNumber : "";
                 // const char* serviceIdField = (pHl7Exp->universalServiceId[0])? pHl7Exp->universalServiceId  : (pHl7Exp->orderType[0] ? pHl7Exp->orderType : "");
                    char serviceIdField[128] = "";
                    
                    if (pHl7Exp->universalServiceId[0]) {
                        // from protocolextraData
                        // If UniversalServiceId exists, use it directly (e.g., from XML)
                        strncpy_s(serviceIdField, sizeof(serviceIdField), pHl7Exp->universalServiceId, _TRUNCATE);
                    }
                    else if (pHl7Exp->orderType[0]) {
                        // from configuration xml
                        // If not present, use OrderType with caret prefix (e.g., ^ECG)
                        snprintf(serviceIdField, sizeof(serviceIdField), "^%s", pHl7Exp->orderType);
                    }

               
                    if (pHl7Exp->orderRequestedTimeStr[0]) {
                        nCharWritten = snprintf(outBuf, outBufRemaining,
                            "OBR|1|%s|%s|%s||%s|%s\r",
                            placerField,
                            fillerField,
                            serviceIdField,
                            pHl7Exp->orderRequestedTimeStr,
                            pHl7Exp->examPerformedTimeStr[0] ? pHl7Exp->examPerformedTimeStr : pHl7Exp->msgTimestampStrBuf);
                    }
                    else {
                        nCharWritten = snprintf(outBuf, outBufRemaining,
                            "OBR|1|%s|%s|%s|||%s\r",
                            placerField,
                            fillerField,
                            serviceIdField,
                            pHl7Exp->examPerformedTimeStr[0] ? pHl7Exp->examPerformedTimeStr : pHl7Exp->msgTimestampStrBuf); //observationDateTimeStr
                    }

                    if (nCharWritten < 0) {
                        printf("BTLHL7EXP: Error: Failed HL7 buffer write-OBR!\n");
                        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_PROCESSING_ERR;
                        return -1;
                    }
                    outBuf += nCharWritten;
                    outBufRemaining -= nCharWritten;
                    
        }

    }
    // If no <Segment> nodes found → fall back to XML NG minimal builder
    if (outBuf == pHl7Exp->pHl7BufNext) {
        BTLHL7EXP_DBUG1("BTLHL7EXP: No <Segment> nodes found in ProtocolExtraData. Falling back to BuildMinimalFromXmlNg().\n");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return btlHl7BuildMinimalFromXmlNg(pHl7Exp);
    }
    else
    {
        //update control struct as all processing succeeded
        _btlHl7ExpUpdateBufCtl(pHl7Exp, outBufRemaining);
    }
       
  //  printf("HL7 output (%d bytes):\n%s\n", pHl7Exp->outputHl7Len, pHl7Exp->outputHl7Buf);

    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 0;
}

