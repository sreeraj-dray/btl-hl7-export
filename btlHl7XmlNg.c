#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "btlHl7XmlExtraData.h"
#include "btlHl7EcgExport.h"
#include "btlHl7XmlNg.h"

HL7ExpAttributeParsed_t g_BtlHl7ExpXmlNgAttributesParsed[BTLHL7EXP_XML_NG_ATTR_COUNT] = { 0 };

int btlHl7BuildMinimalFromXmlNg(BtlHl7Export_t* pHl7Exp) {
    int nCharWritten;
    char* outBuf = pHl7Exp->pHl7BufNext;
    int outBufRemaining = pHl7Exp->outputHl7SizeLeft;
   

    // Use version override if set
    char* pVersion = btlHl7GetEffectiveHl7Version(pHl7Exp, pHl7Exp->hl7VersionDefaultStr);

    // ------------------ MSH ------------------

    char* pSendingApp = pHl7Exp->sendingApplication;
    char* pSendingFac = pHl7Exp->sendingFacility;
    char* pReceivingApp = pHl7Exp->receivingApplication;
    char* pReceivingFac = pHl7Exp->receivingFacility;
    // Default message type for export
     
    if (pHl7Exp->protocolExtraDataLen == 0) {
         char* msgType = "ORU^R01";
        // Append message structure for version >= 2.3
        if (pVersion && pVersion[2] >= '3') {
            msgType = "ORU^R01^ORU_R01";
        }

        nCharWritten = snprintf(outBuf, outBufRemaining,
            "MSH|^~\\&|%s|%s|%s|%s|%s||%s|%s|P|%s||||||UNICODE UTF-8\r",
            pSendingApp, pSendingFac, pReceivingApp, pReceivingFac,
            pHl7Exp->msgTimestampStrBuf, msgType, pHl7Exp->msgControlIdStrBuf, pVersion);
        if (nCharWritten < 0) {
            return -1;
        }
        outBuf += nCharWritten;
        outBufRemaining -= nCharWritten;
    }

    // ---------------- Parse XML ----------------
    xmlDoc* doc = xmlReadFile(pHl7Exp->diagnosticsFilePath, NULL, 0);
    if (!doc) {
        BTLHL7EXP_ERR("BTLHL7EXP: Error: failed to read/ process BTL XMLng file!\n");
        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_XMLNG_ERR1;
        return -1;  //Abort
    }
    xmlNode* root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        BTLHL7EXP_ERR("BTLHL7EXP: Error: failed to parse BTL XMLng!\n");
        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_XMLNG_ERR2;

         // Check if PED (ProtocolExtraData) is available
    if (pHl7Exp->protocolExtraDataLen > 0) {
        BTLHL7EXP_DBUG1("BTLHL7EXP: Proceeding with PDF OBX only (PED available, XML missing)\n");

        // OBR-6 should still come from PED
        // OBR-7 (examPerformedTimeStr) must be blank
        pHl7Exp->examPerformedTimeStr[0] = '\0';

        // Continue with OBX generation (PDF report only)
        return btlHl7ParseProtocolExtraData(pHl7Exp);
    }

        return -1;  //Abort
    }

    // ---------------- PID Segment ----------------
    {
        xmlNode* patientNode = NULL;
        for (xmlNode* node = root->children; node; node = node->next) {
            if (node->type == XML_ELEMENT_NODE && strcmp((char*)node->name, "patients") == 0) {
                for (xmlNode* pat = node->children; pat; pat = pat->next) {
                    if (pat->type == XML_ELEMENT_NODE && strcmp((char*)pat->name, "patient") == 0) {
                        patientNode = pat;
                        break;
                    }
                }
            }
        }

        if (!patientNode) {
            xmlFreeDoc(doc);
            BTLHL7EXP_ERR("BTLHL7EXP: Error: failed to parse BTL XMLng!\n");
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_XMLNG_NO_PAT_INFO;
            return -1;
        }


        btlHl7ExpPidData_t pidData;

        pidData.pid = btlHl7GetNodeText(patientNode, "guid");
        pidData.fn = btlHl7GetNodeText(patientNode, "firstName");
        pidData.mn = btlHl7GetNodeText(patientNode, "middleName");
        pidData.ln = btlHl7GetNodeText(patientNode, "surname");
        pidData.title = 0;
        pidData.dob = 0;
        pidData.cls = 0;
        pidData.sex = btlHl7GetNodeText(patientNode, "sex");

        int errStat;
        outBuf = btlHl7ExpPutPidToMsg(outBuf, &outBufRemaining, &pidData, &errStat);
#ifdef UNDEF
        nCharWritten = snprintf(outBuf, outBufRemaining,
            "PID|1||%s||%s^%s^%s^^||%s||||||||||||||||\r",
            pid ? pid : "",
            ln ? ln : "", fn ? fn : "", mn ? mn : "",
            sex ? sex : "");
        if (nCharWritten < 0) {
            xmlFreeDoc(doc);
            return -1;
    }

        outBuf += nCharWritten;
        outBufRemaining -= nCharWritten;
#endif
        pHl7Exp->pHl7BufNext = outBuf;
        pHl7Exp->outputHl7SizeLeft = outBufRemaining;

       

        if (errStat < 0) {
            xmlFreeDoc(doc);
            BTLHL7EXP_ERR("BTLHL7EXP: Error: failed to get patient info from BTL XMLng!\n");
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_XMLNG_NO_PAT_INFO2;
            return -1;
        }
    }

   
   
   //---------------- ORC Segment ---------------- 
    {

            // No PED → ORC-9 fallback from <createdDatetime>
            nCharWritten = snprintf(outBuf, outBufRemaining,
                "ORC|%s|%s|%s||||||||%s\r",
                pHl7Exp->orderControl[0] ? pHl7Exp->orderControl : "RE",
                 "",
                pHl7Exp->fillerOrderNumber[0] ? pHl7Exp->fillerOrderNumber : "",
                pHl7Exp->examPerformedTimeStr[0] ? pHl7Exp->examPerformedTimeStr : "");
        
                if (nCharWritten < 0) {
                    xmlFreeDoc(doc);
                    return -1;
                }

            outBuf += nCharWritten;
            outBufRemaining -= nCharWritten;
    }


        /* ---------------- OBR Segment ---------------- */
        {

            nCharWritten = snprintf(outBuf, outBufRemaining,
                "OBR|1|%s|%s|^%s||%s|%s\r",
                pHl7Exp->placerOrderNumber[0] ? pHl7Exp->placerOrderNumber : "",
                pHl7Exp->fillerOrderNumber[0] ? pHl7Exp->fillerOrderNumber : "",
                pHl7Exp->universalServiceId[0] ? pHl7Exp->universalServiceId : pHl7Exp->orderType,
                pHl7Exp->orderRequestedTimeStr[0] ? pHl7Exp->orderRequestedTimeStr : "",
                pHl7Exp->examPerformedTimeStr[0] ? pHl7Exp->examPerformedTimeStr : "");


            if (nCharWritten < 0) {
                xmlFreeDoc(doc);
                return -1;
            }
            outBuf += nCharWritten;
            outBufRemaining -= nCharWritten;
        }
    
   

    /* ---------------- Save pointers & cleanup ---------------- */
    _btlHl7ExpUpdateBufCtl(pHl7Exp, outBufRemaining);
    xmlFreeDoc(doc);
   
    return 0;
}
/*

typedef struct {
    char* code;
    char* label;
    int xmlNgAttrId;
} BtlXmlNgAttrMap_t;
*/

#ifdef UNDEF
static BtlXmlNgAttrMap_t btlXmlNgAttrMap[] = {
    {"RR_AvgDistance_05ms", "RR"},
    {"P_Duration_05ms", "P"},
    {"PQ_Duration_05ms", "PQ (PR)"},
    {"QRS_Duration_05ms", "QRS"},
    {"QT_Duration_05ms", "QT"},
    {"P_Axis_deg", "P axis"},
    {"QRS_Axis_deg", "QRS axis"},
    {"T_Axis_deg", "T axis"},
    {"QTc_Duration_05ms", "QTc(Baz)"},
    {"LVH_Sokolow_mm", "Sokolow"},
    {"LVH_Cornell_mm", "Cornell"},
    {"LVH_Romhilt_points", "Romhilt"},
};

#endif

    char gBtlHl7ExpUnitStr_ms[] = "ms";
    char gBtlHl7ExpUnitStr_mm[] = "mm";
    char gBtlHl7ExpUnitStr_deg[] = { 0xC2, 0xB0, 0x00 }; // The UTF-8 code 0xC2B0 represents the degree sign,0x00 for null terminating 
    char gBtlHl7ExpUnitStr_empty[] = "";

      BtlXmlNgAttrMap_t btlXmlNgAttrMap[BTLHL7EXP_XML_NG_ATTR_COUNT] = {
        {
            .code = "RR_AvgDistance_05ms"
            ,.label = "RR"
            ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_RR_AVG_DIST
            ,.unit = gBtlHl7ExpUnitStr_ms
        },

       {
        .code = "P_Duration_05ms"
       ,.label = "P"
       ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_P_DURATION
        ,.unit = gBtlHl7ExpUnitStr_ms
        },
        {
        .code = "PQ_Duration_05ms"
       ,.label = "PQ (PR)"
       ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_PQ_DURATION
        ,.unit = gBtlHl7ExpUnitStr_ms
        },
        {
        .code = "QRS_Duration_05ms"
       ,.label = "QRS"
       ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_QRS_DURATION
        ,.unit = gBtlHl7ExpUnitStr_ms
        },
        {
        .code = "QT_Duration_05ms"
       ,.label = "QT"
       ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_QT_DURATION
       ,.unit = gBtlHl7ExpUnitStr_ms
        },

    {
        .code = "P_Axis_deg"
        ,.label = "P axis"
        ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_P_AXIS
       ,.unit = gBtlHl7ExpUnitStr_deg
    },

    {
        .code = "QRS_Axis_deg"
        , .label = "QRS axis"
         ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_QRS_AXIS
       ,.unit = gBtlHl7ExpUnitStr_deg
    },

    {
        .code = "T_Axis_deg"
        ,.label = "T axis"
        ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_T_AXIS
       ,.unit = gBtlHl7ExpUnitStr_deg
    },


    {
        .code = "QTc_Duration_05ms"
        ,.label = "QTc(Baz)"
         ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_QTC_DURATION
       ,.unit = gBtlHl7ExpUnitStr_ms
    },

    {
        .code = "LVH_Sokolow_mm"
        ,.label = "Sokolow"
         ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_LHV_SOKOLOV
       ,.unit = gBtlHl7ExpUnitStr_mm
    },

    {
        .code = "LVH_Cornell_mm"
        ,.label = "Cornell"
   ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_LHV_CORNELL
       ,.unit = gBtlHl7ExpUnitStr_mm
    },

    {
        .code = "LVH_Romhilt_points"
        ,.label = "Romhilt"
        ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_LHV_ROMHILT
       ,.unit = gBtlHl7ExpUnitStr_empty
    },

          {
        .code = "conclusion"
        ,.label = "MedicalFindings"
        ,.xmlNgAttrId =  BTLHL7EXP_XMLNG_ATTR_ID_CONCLUSION
        ,.unit = gBtlHl7ExpUnitStr_empty
    },


    //Block to mark end of array with code = 0
     {
        .code = 0
        ,.label = 0
        ,.xmlNgAttrId = -1
        ,.unit = gBtlHl7ExpUnitStr_empty
    } 

};


static BtlXmlNgAttrMap_t*  getXmlNgAttrMapForCode(BtlXmlNgAttrMap_t* pXmlNgAttr, char* code) {
    for (int i = 0; i < BTLHL7EXP_XML_NG_ATTR_COUNT; i++)
    { 
        //check if we have reached end of map the array
        if (pXmlNgAttr->code == NULL) {
            break;
        }
        if (strcmp(code, pXmlNgAttr->code) == 0) {
            return pXmlNgAttr ;
        }
        pXmlNgAttr++;
    }
    return NULL; // Fallback if not found
}


char* getLabelForCode( char* code) {
    for (int i = 0; i < BTLHL7EXP_XML_NG_ATTR_COUNT; i++) {
        if (strcmp(code, btlXmlNgAttrMap[i].code) == 0) {
            return btlXmlNgAttrMap[i].label;
        }
    }
    return NULL; // Fallback if not found
}




xmlNode* btlhl7ExpGetXmlNode(xmlNode* parent,  xmlChar* nodeName, xmlChar* attrName, xmlChar* attrValMatch) {
    if (parent == NULL || nodeName == NULL) {
        return NULL;
    }

    xmlNode* cur_node = parent->children;
    while (cur_node != NULL) {
        if (cur_node->type == XML_ELEMENT_NODE && xmlStrcmp(cur_node->name, nodeName) == 0) {
            if (attrName == NULL || attrValMatch == NULL) {
                return cur_node; // Found the child node
            }
             xmlChar* pAttrVal = xmlGetProp(cur_node, attrName);
             if (pAttrVal != NULL) {
                 if (xmlStrcmp(pAttrVal, attrValMatch) == 0) {
                     //found the child node matching the attribute value
                     return cur_node;
                 }               
             }
        }
        cur_node = cur_node->next;
    }
    return NULL; // Child node not found
}

int btlhl7ExpParseXmlNgGlobalResults(xmlNode* pDiagnosticStepNode) {
   //we have diagnosticStep node with name = globalResults  

    xmlNode* pMeasurementsNode = btlhl7ExpGetXmlNode(pDiagnosticStepNode, "measurements", NULL, NULL);
    if (pMeasurementsNode == NULL) {
        return -2;
    }

    // loop through measurement children
    for (xmlNode* meas = pMeasurementsNode->children; meas != NULL; meas = meas->next) {
        if (meas->type != XML_ELEMENT_NODE || xmlStrcmp(meas->name, (xmlChar*)"measurement") != 0) {
            continue;
        }

        xmlChar* pCode = xmlGetProp(meas, (xmlChar*)"code");
        if (pCode == NULL) {
            continue;
        }
        char* pVal = (meas->children) ? (char*)meas->children->content : NULL;
        if (pVal == NULL) {
            xmlFree(pCode);
            continue;
        }
        BtlXmlNgAttrMap_t* pXmlNgAttrMap = getXmlNgAttrMapForCode(btlXmlNgAttrMap, ( char*)pCode);
        
        if (pXmlNgAttrMap == NULL) {
            xmlFree(pCode);
            continue;
        }
       //we got a known code so write its value to the output array
        HL7ExpAttributeParsed_t* pXmlNgAttr = &g_BtlHl7ExpXmlNgAttributesParsed[pXmlNgAttrMap->xmlNgAttrId];
        pXmlNgAttr->found = 1;
        pXmlNgAttr->size = (int)strlen(pVal);
        pXmlNgAttr->pRelated1 = pXmlNgAttrMap;
        strncpy(pXmlNgAttr->value, ( char*)pVal, sizeof(pXmlNgAttr->value));

         xmlFree(pCode);
    }
    return 0;
} //btlhl7ExpParseXmlNgGlobalResults

int btlhl7ExpParseXmlNgAverageBeat(xmlNode* pDiagnosticStepNode)
{
    // We have diagnosticStep node with name = averageBeat  
    xmlNode* pEventTableNode = btlhl7ExpGetXmlNode(pDiagnosticStepNode, "eventTable", NULL, NULL);
    if (pEventTableNode == NULL) {
        return -2;
    }

    // Find first event
    xmlNode* pEventNode = btlhl7ExpGetXmlNode(pEventTableNode, "event", NULL, NULL);
    if (pEventNode == NULL) {
        return -2;
    }

    // Find its measurements
    xmlNode* pMeasurementsNode = btlhl7ExpGetXmlNode(pEventNode, "measurements", NULL, NULL);
    if (pMeasurementsNode == NULL) {
        return -2;
    }

    // Loop through <measurement> elements
    for (xmlNode* meas = pMeasurementsNode->children; meas != NULL; meas = meas->next) {
        if (meas->type != XML_ELEMENT_NODE || xmlStrcmp(meas->name, (xmlChar*)"measurement") != 0)
            continue;

        xmlChar* pCode = xmlGetProp(meas, (xmlChar*)"code");
        if (pCode == NULL)
            continue;

        char* pVal = (meas->children) ? (char*)meas->children->content : NULL;
        if (pVal == NULL) {
            xmlFree(pCode);
            continue;
        }

        // Map code to known label/unit
        BtlXmlNgAttrMap_t* pXmlNgAttrMap = getXmlNgAttrMapForCode(btlXmlNgAttrMap, (char*)pCode);
        if (pXmlNgAttrMap == NULL) {
            xmlFree(pCode);
            continue;
        }

        // Store in parsed array
        HL7ExpAttributeParsed_t* pXmlNgAttr =
            &g_BtlHl7ExpXmlNgAttributesParsed[pXmlNgAttrMap->xmlNgAttrId];
        pXmlNgAttr->found = 1;
        pXmlNgAttr->size = (int)strlen(pVal);
        pXmlNgAttr->pRelated1 = pXmlNgAttrMap;
        strncpy(pXmlNgAttr->value, (char*)pVal, sizeof(pXmlNgAttr->value));

        xmlFree(pCode);
    }

    return 0;
}

//#################################################################################################

int btlHl7ParseXmlNg(BtlHl7Export_t* pHl7Exp) {
    int retVal;

    if (pHl7Exp->diagnosticsFilePath[0]== 0) {
        BTLHL7EXP_DBUG0("BTLHL7EXP:No diagnostics btlxmlng given!\n");
        return -1;
    }

    xmlDoc* doc = xmlReadFile(pHl7Exp->diagnosticsFilePath, NULL, 0);
    if (!doc) {
        BTLHL7EXP_ERR("BTLHL7EXP:  Error: Failed to open diagnostics XML!\n");
        return -1;
    }

    xmlNode* root = xmlDocGetRootElement(doc);
    if (!root) {
        BTLHL7EXP_ERR("BTLHL7EXP: Error: Empty diagnostics XML!\n");
        xmlFreeDoc(doc);
        return -2; // XmlNg parse error
    }
    memset(g_BtlHl7ExpXmlNgAttributesParsed, 0, sizeof(g_BtlHl7ExpXmlNgAttributesParsed));
    // ---------------- Extract createdDatetime for OBR-7 (exam performed time) ----------------
do {
    xmlNode* pHeaderNode = btlhl7ExpGetXmlNode(root, (xmlChar*)"header", NULL, NULL);
    if (pHeaderNode != NULL) {
        xmlNode* pCreatedNode = btlhl7ExpGetXmlNode(pHeaderNode, (xmlChar*)"createdDatetime", NULL, NULL);
        if (pCreatedNode != NULL && pCreatedNode->children && pCreatedNode->children->content) {
            // btlxmlng has date and time in Iso formate with no utc offset so convert it into HL7 format
             
            char hl7_timestamp_out[24];
            retVal = btlHl7ExpConvertIsoToHl7Ts(pCreatedNode->children->content, hl7_timestamp_out, sizeof(hl7_timestamp_out));
            if (retVal == 0) {
                // copy safely into pHl7Exp->examPerformedTimeStr
                strncpy(pHl7Exp->examPerformedTimeStr,
                    hl7_timestamp_out,
                    sizeof(hl7_timestamp_out) - 1);
                pHl7Exp->examPerformedTimeStr[sizeof(pHl7Exp->examPerformedTimeStr) - 1] = '\0';

                BTLHL7EXP_DBUG1("BTLHL7EXP: Parsed header createdDatetime -> examPerformedTimeStr = [%s]\n",
                    pHl7Exp->examPerformedTimeStr);
            }
            else {
                pHl7Exp->examPerformedTimeStr[0] = 0;
                BTLHL7EXP_ERR("BTLHL7EXP:ERROR:BTLXMLNG:could not parse createdDatetime\n");
            }
        }
    }
} while (0);

   
    // parse out examinations/examination :type=
    xmlNode* pExaminationsNode = btlhl7ExpGetXmlNode( root, "examinations", NULL, NULL);
    if (pExaminationsNode == NULL) {
        return -2; 
    }
    xmlNode* pExaminationNode = btlhl7ExpGetXmlNode(pExaminationsNode, "examination", NULL, NULL);
    if (pExaminationNode == NULL) {
        return -2;
    }
    //############################################################################################
    {
    xmlChar* pExamGuid = xmlGetProp(pExaminationNode, (xmlChar*)"guid");
    if (pExamGuid != NULL) {
        // Copy into export structure
        char cleanedGuid[128] = {0};
        int j = 0;
        for (int i = 0; pExamGuid[i] != '\0' && j < sizeof(cleanedGuid) - 1; i++) {
            if (pExamGuid[i] != '{' && pExamGuid[i] != '}')
                cleanedGuid[j++] = pExamGuid[i];
        }

        strncpy(pHl7Exp->fillerOrderNumber, cleanedGuid, sizeof(pHl7Exp->fillerOrderNumber) - 1);
        pHl7Exp->fillerOrderNumber[sizeof(pHl7Exp->fillerOrderNumber) - 1] = '\0';

        BTLHL7EXP_DBUG1("BTLHL7EXP: Parsed examination guid -> fillerOrderNumber = [%s]\n",
                         pHl7Exp->fillerOrderNumber);

        xmlFree(pExamGuid);
    } else {
        pHl7Exp->fillerOrderNumber[0] = 0;
        BTLHL7EXP_DBUG0("BTLHL7EXP: No examination guid found in XML\n");
    }
    }


    //we got examination node, now check the type
    xmlChar* pExamTypeVal = xmlGetProp(pExaminationNode, "type");
    if (pExamTypeVal == NULL) {
            return -2;
    }

    HL7ExpAttributeParsed_t* pXmlNgAttr = &(g_BtlHl7ExpXmlNgAttributesParsed[BTLHL7EXP_XMLNG_ATTR_ID_EXAM_TYPE]);
    pXmlNgAttr->found = 1;
    strncpy(pXmlNgAttr->value, ( char*)pExamTypeVal, sizeof(pXmlNgAttr->value));

    pXmlNgAttr->size = (int)strlen(pXmlNgAttr->value);

    do {
        // examinations/examination/analysis/diagnosticStep :name = globalResults 

        xmlNode* pAnalysisNode = btlhl7ExpGetXmlNode(pExaminationNode, "analysis", NULL, NULL);
        if (pAnalysisNode == NULL) {
            break;
        }
        xmlNode* pDiagnosticStepNode = btlhl7ExpGetXmlNode(pAnalysisNode, "diagnosticStep", "name", "globalResults");
        if (pDiagnosticStepNode != NULL) {
            //we have diagnosticStep node with name = globalResults  
            btlhl7ExpParseXmlNgGlobalResults(pDiagnosticStepNode);
        }

        // --- Parse diagnosticStep name = "averageBeat" ---
        xmlNode* pDiagnosticStepAvgNode = btlhl7ExpGetXmlNode(pAnalysisNode, "diagnosticStep", "name", "averageBeat");
        if (pDiagnosticStepAvgNode != NULL) {
            btlhl7ExpParseXmlNgAverageBeat(pDiagnosticStepAvgNode);
        }
    }while (0);
      
    // --- handle <results>/<conclusion> only ---
    do {
        xmlNode* pResultsNode = btlhl7ExpGetXmlNode(pExaminationNode, "results", NULL, NULL);
        if (pResultsNode == NULL) {
            break;
        }
        xmlNode* pConclusionNode = btlhl7ExpGetXmlNode(pResultsNode, "conclusion", NULL, NULL);
        if (pConclusionNode == NULL) {
            break;
        }
        xmlChar* pValidAttr = xmlGetProp(pConclusionNode, "valid");
        if (!pValidAttr || xmlStrcasecmp(pValidAttr, ( xmlChar*)"true") != 0) {
            if (!pValidAttr) {
                xmlFree(pValidAttr);
            }
            break;
        }

    xmlChar* pConclusionText = xmlNodeGetContent(pConclusionNode);
    if (pConclusionText && xmlStrlen(pConclusionText) > 0) {
        BtlXmlNgAttrMap_t* pXmlNgAttrMap = getXmlNgAttrMapForCode(btlXmlNgAttrMap,"conclusion");
        if (pXmlNgAttrMap != NULL) {
            HL7ExpAttributeParsed_t* pConclusionAttr =
                &g_BtlHl7ExpXmlNgAttributesParsed[pXmlNgAttrMap->xmlNgAttrId];

            pConclusionAttr->found = 1;
            strncpy(pConclusionAttr->value, (char*)pConclusionText, sizeof(pConclusionAttr->value) - 1);
            pConclusionAttr->value[sizeof(pConclusionAttr->value) - 1] = '\0';
            pConclusionAttr->size = (int)strlen(pConclusionAttr->value);

                            
        }
    }
    if (pConclusionText) {
        xmlFree(pConclusionText);
    }
                
    if (pValidAttr){
        xmlFree(pValidAttr);
        }
            
        
    } while (0);//do (while) block processing result node 

    xmlFreeDoc(doc);
    return 0;
    }

        // ---------------------------------------------------------------------
        // Array of attribute IDs to include in HL7 OBX measurement output
        // ---------------------------------------------------------------------
        int gBtlHl7ExpMeasAttrIds[] = {
            BTLHL7EXP_XMLNG_ATTR_ID_RR_AVG_DIST,
            BTLHL7EXP_XMLNG_ATTR_ID_P_DURATION,
            BTLHL7EXP_XMLNG_ATTR_ID_PQ_DURATION,
            BTLHL7EXP_XMLNG_ATTR_ID_QRS_DURATION, 
            BTLHL7EXP_XMLNG_ATTR_ID_QT_DURATION,
            BTLHL7EXP_XMLNG_ATTR_ID_P_AXIS,
            BTLHL7EXP_XMLNG_ATTR_ID_QRS_AXIS ,
            BTLHL7EXP_XMLNG_ATTR_ID_T_AXIS ,
            BTLHL7EXP_XMLNG_ATTR_ID_QTC_DURATION,
            BTLHL7EXP_XMLNG_ATTR_ID_LHV_SOKOLOV,
            BTLHL7EXP_XMLNG_ATTR_ID_LHV_CORNELL,
            BTLHL7EXP_XMLNG_ATTR_ID_LHV_ROMHILT,
            BTLHL7EXP_XMLNG_ATTR_ID_CONCLUSION ,
            BTLHL7EXP_XMLNG_ATTR_ID_MED_FINDING,

            -1 // terminator
        };

        // List of attribute IDs for doctor statement / conclusion / medical finding
   // static int docAttrIds[] = {
   // BTLHL7EXP_XML_ATTR_CONCLUSION,
  //  BTLHL7EXP_XML_ATTR_DOCTOR_STAT,
  //  BTLHL7EXP_XML_ATTR_MED_FINDING,
   // -1  // terminator
   // };


#ifdef UNDEF

        static const BtlXmlNgAttrToEcgMeasMap_t btlXmlNgAttrToEcgMeasMap[] = {
        {BTLHL7EXP_XMLNG_ATTR_ID_RR_AVG_DIST, "RR"},
        {BTLHL7EXP_XMLNG_ATTR_ID_P_DURATION, "P"},
        {BTLHL7EXP_XMLNG_ATTR_ID_PQ_DURATION, "PQ (PR)"},
        {BTLHL7EXP_XMLNG_ATTR_ID_QRS_DURATION, "QRS"},
        {BTLHL7EXP_XMLNG_ATTR_ID_QT_DURATION, "QT"},
        {BTLHL7EXP_XMLNG_ATTR_ID_QTC_DURATION, "QTc(Baz)"},
        {BTLHL7EXP_XMLNG_ATTR_ID_LHV_SOKOLOV, "Sokolow"},
        {BTLHL7EXP_XMLNG_ATTR_ID_LHV_CORNELL, "Cornell"},
        {BTLHL7EXP_XMLNG_ATTR_ID_LHV_ROMHILT, "Romhilt"},
        {BTLHL7EXP_XMLNG_ATTR_ID_CONCLUSION, "Conclusion"},
        {BTLHL7EXP_XMLNG_ATTR_ID_MEDICAL_FINDINGS, "MedicalFindings"},
        {-1, NULL}  // Terminator for the array
            };

        // Function to get xmlNgAttrId from the code (label)
        int getXmlNgAttrIdFromCode(const char* code) {
            for (int i = 0; btlXmlNgAttrToEcgMeasMap[i].xmlNgAttrId != -1; i++) {
                if (strcmp(code, btlXmlNgAttrToEcgMeasMap[i].ecgMeasurement) == 0) {
                    return btlXmlNgAttrToEcgMeasMap[i].xmlNgAttrId;
                }
            }
            return -1;  // Return -1 if not found
        }
#endif

    // ---------------------------------------------------------------------
//  Original Method — Fixed Measurement Set
// ---------------------------------------------------------------------
    void btlHl7ExpAddEcgMeasToHl7_1(BtlHl7Export_t * pHl7Exp, int* pMeasAttrIds)
    {
        char* outBuf = pHl7Exp->pHl7BufNext;
        int outBufRemaining = pHl7Exp->outputHl7SizeLeft;

       for (int i = 0; pMeasAttrIds[i] != -1; i++) {
       //for (int i = 0; i < BTLHL7EXP_XML_NG_ATTR_COUNT; i++) {
            int attrId = pMeasAttrIds[i];

            // Skip invalid attribute IDs
            if (attrId < 0 || attrId >= BTLHL7EXP_XML_NG_ATTR_COUNT) {
                continue;
            }


            HL7ExpAttributeParsed_t* attr = &g_BtlHl7ExpXmlNgAttributesParsed[attrId];
            if (!attr->found) {
                continue;
            }

            BtlXmlNgAttrMap_t* pXmlNgAttrMap = attr->pRelated1;

           if (pXmlNgAttrMap == NULL) {
               continue;  // check for NULL
            }
        

             char* label = pXmlNgAttrMap->label;
             char* value = attr->value;
             char* unit = pXmlNgAttrMap->unit;
             pHl7Exp->obxIndex++;
            
            int n = snprintf(outBuf, outBufRemaining,
                "OBX|%d|TX|^%s||%s|%s|||||F\r",
                pHl7Exp->obxIndex, label, value, unit);

            if (n < 0 || n >= outBufRemaining) {
                break;
            }
            outBuf += n;
            outBufRemaining -= n;
        }
       _btlHl7ExpUpdateBufCtl(pHl7Exp, outBufRemaining);
       return;
        
    }// btlHl7ExpAddEcgMeasToHl7_1()

    /*
    void addMeasurementToXmlNg2(BtlHl7Export_t * pHl7Exp,  int* attrList, int count)
    {
        char* outBuf = pHl7Exp->pHl7BufNext;
        int outBufRemaining = pHl7Exp->outputHl7SizeLeft;
        int obxIndex = 1;

        for (int i = 0; i < count; i++) {
            int attrId = attrList[i];
            if (attrId < 0 || attrId >= BTLHL7EXP_XML_NG_ATTR_COUNT) {
                continue;
            }
               

            HL7ExpAttributeParsed_t* attr = &g_BtlHl7ExpXmlNgAttributesParsed[attrId];
            if (!attr->found) {
                continue;
            }

           BtlXmlNgAttrMap_t* pXmlNgAttrMap = attr->pRelated1;
          //  if (pXmlNgAttrMap == NULL) {
            //    continue;  // check for NULL                                                                                                          
          //  }


            // Get the code from the attribute map
             char* code = btlXmlNgAttrMap[attrId].label;

            // Use the code to get the corresponding xmlNgAttrId
            int xmlNgAttrId = getXmlNgAttrIdFromCode(code);
            if (xmlNgAttrId == -1) {
                continue;  // Invalid attribute, skip
            }
             char* label = btlXmlNgAttrMap[attrId].label;
             char* value = attr->value;
             char* unit = pXmlNgAttrMap->unit;
             // char* unit = getUnitForCode(btlXmlNgAttrMap[attrId].code);

            int n = snprintf(outBuf, outBufRemaining,
                "OBX|%d|ST|%s||%s|%s||N|||F\r",
                obxIndex++, label, value, unit);

            if (n < 0 || n >= outBufRemaining) {
                break;
            }
               

            outBuf += n;
            outBufRemaining -= n;
        }

        pHl7Exp->pHl7BufNext = outBuf;
        pHl7Exp->outputHl7SizeLeft = outBufRemaining;
    }
    */
    
    //  Build Continuous String (RR=800|PR=120|QT=400|QTc=420|)
    // ---------------------------------------------------------------------
   
    // function to generate ecg measurments as a continous string 
    //reference from ConnectIN: 
    // OBX|2|TX|^Conclusion||RR: 1071ms\X0A\P: 98ms\X0A\PQ (PR): 186ms\X0A\QRS: 100ms\X0A\QT: 515ms\X0A\P axis: 41..\X0A\
    // QRS axis: 25..\X0A\T axis: 131..\X0A\QTc(Baz): 498ms\X0A\Sokolow: 19mm\X0A\Cornell: 13mm\X0A\Romhilt: 2\X0A\||||||P
   
    void btlHl7ExpAddEcgMeasToHl7_0(BtlHl7Export_t* pHl7Exp, int* pMeasAttrIds)
    {
        BTLHL7EXP_DBUG0("[BTLHL7EXP: adding ecg measurments in a single OBX segment\n");
        char* outBuf = pHl7Exp->pHl7BufNext;
        int outBufRemaining = pHl7Exp->outputHl7SizeLeft;
        pHl7Exp->obxIndex++;
        int n = snprintf(outBuf, outBufRemaining,
            "OBX|%d|TX|^Conclusion||", pHl7Exp->obxIndex);

        outBuf += n;
        outBufRemaining -= n;
        int attrFoundCount = 0;
        for (int i = 0; pMeasAttrIds[i] != -1; i++) {
            //for (int i = 0; i < BTLHL7EXP_XML_NG_ATTR_COUNT; i++) {
            int attrId = pMeasAttrIds[i];

            // Skip invalid attribute IDs
            if (attrId < 0 || attrId >= BTLHL7EXP_XML_NG_ATTR_COUNT) {
                continue;
            }


            HL7ExpAttributeParsed_t* attr = &g_BtlHl7ExpXmlNgAttributesParsed[attrId];
            if (!attr->found) {
                continue;
            }

            BtlXmlNgAttrMap_t* pXmlNgAttrMap = attr->pRelated1;

            if (pXmlNgAttrMap == NULL) {
                continue;  // check for NULL
            }
           
            char* label = pXmlNgAttrMap->label;
            char* value = attr->value;
            char* unit = pXmlNgAttrMap->unit;


            // RR: 1071ms\X0A\ ;
            n = snprintf(outBuf, outBufRemaining, "%s: %s%s\\X0A\\", label, value, unit);


            if (n < 0 || n >= outBufRemaining) {
                break;
            }
            outBuf += n;
            outBufRemaining -= n;
            attrFoundCount++;


        }// for 

         n = snprintf(outBuf, outBufRemaining, "||||||%s\r", pHl7Exp->obxResultStatusStr);
        if (n > 0 ) {
        outBuf += n;
        outBufRemaining -= n;
        }
        if (attrFoundCount > 0) {
            _btlHl7ExpUpdateBufCtl(pHl7Exp, outBufRemaining);

        }
        else {
            pHl7Exp->obxIndex--;
        }
        
        return;
    }// btlHl7ExpAddEcgMeasToHl7_0()
#ifdef UNDEF



    static char* strtok_r1(char* str, const char* delim, char** saveptr) {
        char *token;
        //char *saveptr1;
        char  *ptr;
        int is_delim;
        int i;
        //if str is not null, we should start from str (new string is being passed by user)
        //else we should start from saveptr 
        // we should go through every member of string and till we find a delimeter from list of delimeters
        //replace the delimeter found with null and save the pointer to remaining string in saveptr for future parsing
        //return the starting address of the token found

        if (str != NULL) {
            *saveptr = str;
        }
        if (*saveptr == NULL) {
            return NULL;
        }
       
        ptr = *saveptr ;
        //loop through every character util we find a delimeter or null character(end of string)
        while (*ptr != '\0') {
            is_delim = 0;
            //loop will check every character in delimiter string
            for (i = 0; delim[i] != '\0'; i++) {
                if (*ptr == delim[i]) {
                    is_delim = 1;// found delimeter
                    break;
                }
            }
            //replace the delimeter found with null
            if (is_delim) {
                *ptr = '/0';
                *saveptr = ptr + 1; // save pointer for next token
                return token; // return to beginning of token
            }
            ptr++;

        }
        *saveptr = NULL;
        return token;



    }
    #endif // UNDEF
    // if <conclusion> is found within the <results> send it out in an hl7 Obx segment , and 
    // The valid attribute in conclusion tab must have value "true".
   void btlHl7ExpAddDoctorStatement(BtlHl7Export_t* pHl7Exp)
   {
       if (!pHl7Exp) {
           return;
       }
        
  

    // Use the Conclusion attribute from XML
    
    HL7ExpAttributeParsed_t* pAttr = &g_BtlHl7ExpXmlNgAttributesParsed[BTLHL7EXP_XMLNG_ATTR_ID_CONCLUSION];
    if (!pAttr->found || (pAttr->size) == 0) {
         return;  
    }
       
    char* outBuf = pHl7Exp->pHl7BufNext;
    int outBufRemaining = pHl7Exp->outputHl7SizeLeft;
    int n;

    // Increment OBX index
    pHl7Exp->obxIndex++;

    // Create OBX header
    n = snprintf(outBuf, outBufRemaining,
                 "OBX|%d|TX|^MedicalFindings||",
                 pHl7Exp->obxIndex);
    if (n < 0 || n >= outBufRemaining) {
        pHl7Exp->obxIndex--; // we are aborting, reinstate orginal value
        return;
    }

        outBuf += n;
        outBufRemaining -= n;


    // Add value lines with \X0A\ for newlines

    char *token;
    char *saveptr = NULL;
    // , will be replaced with null by strtok_r
    token = strtok_r(pAttr->value, ",", &saveptr);
    while (token != NULL) {
    //write token to hl7 message and deliminate with \\X0A\\;
        n = snprintf(outBuf, outBufRemaining, "%s\\X0A\\", token);

        if (n < 0 || n >= outBufRemaining) {
        return;
        }
        outBuf += n;
        outBufRemaining -= n;

        token = strtok_r(NULL, ",", &saveptr); 
    }   
    
    // Close OBX
    n = snprintf(outBuf, outBufRemaining, "||||||%s\r", pHl7Exp->obxResultStatusStr);
    if (n < 0 || n >= outBufRemaining){
        return;
    }
    outBuf += n;
    outBufRemaining -= n;
    

    // Update buffer control
    _btlHl7ExpUpdateBufCtl(pHl7Exp, outBufRemaining);
}
// btlHl7ExpAddDoctorStatement
