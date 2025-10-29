#include "btlHl7ExpParseHl7.h"
#include <stdarg.h>  // for va_list, va_start, etc
#include "btlHl7ExpDebug.h"

//typedef void (*BtlHl7LogCallback_t)(const char* msg);  // log callback type

// Global callback pointer (default is NULL)
 BtlHl7LogCallback_t gBtlHl7LogCallbackFn = NULL;

// Default verbosity level (0 = minimal, 1 = normal, 2 = verbose)
int gHl7ExpCurrentDebugLevel = 0;


// Function to register a log callback (used by customer or log system)
void btlHl7ExpRegisterLogCallback(BtlHl7LogCallback_t cb) {
    gBtlHl7LogCallbackFn = cb;
}



//IMPORTANT: The order below must match BTLHL7EXP_SEG_xxxx defines in btlHl7ParseHl7.h
//Do NOT change the first and last members in the array init below
//For export, we need to process only the ACK segment (MSA)
static char gBtlHl7ExpKnownSegments[][BTL_HL7_SEGMENT_NAME_BUF_LEN] = {
    "___"  //Invalid segment type, Id of 0 is reserved for invalid segment ID
    ,"MSH"  //1
    ,"MSA"  //2

    ,""  //empty string to mark end
};//gBtlHl7ExpKnownSegments[][]



static int gBtlHl7Exp_nKnownSegments = (sizeof(gBtlHl7ExpKnownSegments) / BTL_HL7_SEGMENT_NAME_BUF_LEN) - 1;

BtlHL7ExpSegmentDef_t gBtlHl7ExpMshSegmentDef[BTLHL7EXP_MSH_N_ATTR + 1] = {
     {
        .attrSystemId = BTLHL7EXP_MSH_CONTROL_CHARS
        ,.fieldNo = 1
        ,.componentNo = 0  //whole field
        ,.subComponentNo = 0 //not applicable
        ,.required = 1
     }
    ,{
        .attrSystemId = BTLHL7EXP_MSH_MSG_TYPE
        ,.fieldNo = 8
        ,.componentNo = 1  //
        ,.subComponentNo = 0 //not applicable
        ,.required = 1
     }
    ,{
        .attrSystemId = BTLHL7EXP_MSH_MSG_TYPE_ID
        ,.fieldNo = 8
        ,.componentNo = 2  //
        ,.subComponentNo = 0 //not applicable
        ,.required = 1
     }
        ,{
        .attrSystemId = BTLHL7EXP_MSH_SENDING_APP_1,
        .fieldNo = 3,
        .componentNo = 1,
        .subComponentNo = 0,
        .required = 0
     }
    ,{
        .attrSystemId = BTLHL7EXP_MSH_SENDING_APP_2,
        .fieldNo = 3,
        .componentNo = 2,
        .subComponentNo = 0,
        .required = 0
     }
    ,{
        .attrSystemId = BTLHL7EXP_MSH_SENDING_FAC_1,
        .fieldNo = 4,
        .componentNo = 1,
        .subComponentNo = 0,
        .required = 0
     }
    ,{
        .attrSystemId = BTLHL7EXP_MSH_SENDING_FAC_2,
        .fieldNo = 4,
        .componentNo = 2,
        .subComponentNo = 0,
        .required = 0
     }


    ,{
        .attrSystemId = BTLHL7_SEG_LAST  //placeholder to detect end of array
        ,.fieldNo = 0			//not applicable
        ,.componentNo = 0		//not applicable
        ,.subComponentNo = 0	//not applicable
        ,.required = 1
     }


};  //gBtlHl7ExpMshSegmentDef[]


//MSA segment known attributes
BtlHL7ExpSegmentDef_t gBtlHl7ExpMsaSegmentDef[BTLHL7EXP_MSA_N_ATTR + 1] = {
     {
        .attrSystemId = BTLHL7EXP_MSA_ACK_STAT
        ,.fieldNo = 1
        ,.componentNo = 0  //whole field
        ,.subComponentNo = 0 //not applicable
        ,.required = 1
     }
    ,{
        .attrSystemId = BTLHL7_SEG_LAST  //placeholder to detect end of array
        ,.fieldNo = 0			//not applicable
        ,.componentNo = 0		//not applicable
        ,.subComponentNo = 0	//not applicable
        ,.required = 1
     }

};  //gBtlHl7ExpMsaSegmentDef[]

//###############################################################################
//Function to get known segment id


static int btlHl7GetSegmentTypeId( char* pBuf, int size) {
    int segId = 0;
    int idx = 0;

    int nKnownSegments;

    if (size > BTL_HL7_MAX_SEGMENT_NAME_LEN) {
        return -1;
    }
    nKnownSegments = gBtlHl7Exp_nKnownSegments;

    while (nKnownSegments) {
        nKnownSegments--;
        if (gBtlHl7ExpKnownSegments[segId][0] != pBuf[0]) {
            //first char mismatch - skip segment type
            segId++;
            continue;
        }
        if (strncmp(gBtlHl7ExpKnownSegments[segId], pBuf, size) != 0) {
            //segment type mismatch, so skip
            segId++;
            continue;
        }
        //found the segment type
        return segId;
    }

    return -1;

}  //btlHl7GetSegmentTypeId()

//###############################################################################
// Function to parse HL7 message (First Level)
//Find segment and field boundaries, field lengths
int btlHl7ExpParseHL7Message_L1(BtlHl7ExpParser_t* pParser, char* pHl7Msg) {

     char* pMsgLast = pHl7Msg;
     char* pMsgCur = pHl7Msg;
     char* pMsgSegmentStart = pHl7Msg;

     BtlHL7ExpMsgParsedL1_t* pParsedL1Out = pParser->parsedL1OutSegments;

    char segmentEnd = BTL_HL7_SEGMENT_END_CHAR;
    char fieldSep = BTL_HL7_FIELD_SEPERATOR_CHAR;

    int nKnownSegmentsFound = 0;
    pParser->nKnownSegmentsFound = 0;

    int fieldNo = 0;
    int fieldSize;
    int segmentTypeId = -1;
    int skipSegment = 0;
    int skipTillSegmentStart = 1;

    int sizeofParsedL1Out = sizeof(*pParsedL1Out) * BTLHL7EXP_MAX_KNOWN_SEGMENTS;
    memset(pParsedL1Out, 0, sizeofParsedL1Out);

    //Find known segments and populate parse output
    while (1) {
        if (*pMsgCur == 0) {
            //end of HL7 message
            break;
        }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //skipping multiple segment end characters if present
        if (skipTillSegmentStart != 0) {
            pMsgLast = pMsgCur;
            if ((*pMsgCur == segmentEnd) || (*pMsgCur == BTLHL7EXP_MLLP_MSG_START_CHAR)) {
                pMsgCur++;
                continue;  //skip till field end
            }
            skipTillSegmentStart = 0;  //new segment start detected
            fieldNo = 0;
            pMsgSegmentStart = pMsgCur;
        }
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          //skip till next field boundary
        if (!((*pMsgCur == segmentEnd) || (*pMsgCur == fieldSep)))
        {
            //not field or segment end
            pMsgCur++;
            continue;  //skip till field end
        }  //if NOT field/segment end
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

            //We are at field boundary
             //get field data and reinit for next segment/ field
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            //skip the segment bytes if unknown segment
        if (skipSegment != 0) {
            //parsing unknown segment - skip

            if (*pMsgCur != segmentEnd) {
                pMsgCur++;
                continue;
            }
            //segment end
            skipSegment = 0;
            fieldNo = 0;
            skipTillSegmentStart = 1;
            pMsgCur++;
            pMsgLast = pMsgCur;
            pMsgSegmentStart = pMsgCur;
            continue;  //skip till field end
        }
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        //We reach here only for known segments unless we are parsing the
            // segment type field itself
        //we are at a field boundary
        fieldSize = (int)(pMsgCur - pMsgLast);

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if (fieldNo == 0) {
            //segment type field
#ifdef BTLHL7EXP_L1PARSE_EN_PRT_FIELDS
            btlHl7ExpDbugCopyPrintStr("\n~~~~~~~~~~~~~~~ Found segment: ", pMsgLast, fieldSize);
#endif
            segmentTypeId = btlHl7GetSegmentTypeId(pMsgLast, fieldSize);
            if (segmentTypeId < 0) {
                //unknown segment type - so skip
                skipSegment = 1;
                pMsgCur++;
                continue;
            }
            //found a known segment type
            pParsedL1Out->segmentTypeId = segmentTypeId;
            pParsedL1Out->segmentStartIndex = (int)(pMsgSegmentStart - pHl7Msg);
            nKnownSegmentsFound++;
        }
        else {
#ifdef BTLHL7EXP_L1PARSE_EN_PRT_FIELDS
            btlHl7ExpDbugCopyPrintStr("   Found field value: ", pMsgLast, fieldSize);
#endif
        }
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        pParsedL1Out->fieldBeginIndices[fieldNo] = (int)(pMsgLast - pHl7Msg);
        pParsedL1Out->fieldSizes[fieldNo] = fieldSize;
        pParsedL1Out->nFieldsFound = fieldNo + 1;

        if ((*pMsgCur == segmentEnd) || (*pMsgCur == 0)) {
            //this is the last field in the segment
            //save the segment size
            pParsedL1Out->segmentSize = (int)(pMsgCur - pMsgSegmentStart);

            if (*pMsgCur == 0) {
                //end of message
                break;
            }
            pParsedL1Out++;  //ready to start parsing next known segment
            //memset(pParsedL1Out, 0, sizeof(*pParsedL1Out));
            fieldNo = 0;
            skipTillSegmentStart = 1;
            skipSegment = 0;
            pMsgCur++;
            pMsgLast = pMsgCur;  //point to next field start
            continue;
        }

        skipSegment = 0;
        skipTillSegmentStart = 0;

        fieldNo++;  //parse next field
        pMsgCur++;
        pMsgLast = pMsgCur;  //point to next field start

    } //main parser while
    pParser->nKnownSegmentsFound = nKnownSegmentsFound;
    return nKnownSegmentsFound;
}  //btlHl7ExpParseHL7Message_L1()

//##################################################################################



// ###########################################################################
//Level-2 Parsing 

static int btlHl7ExpSetMsgControlCharacters(BtlHl7ExpParser_t* pParser, char* pHl7Msg) {

    int fieldSize;
    BtlHL7ExpMsgParsedL1_t* pParsedL1Out = pParser->parsedL1OutSegments;

     char* pControlChars = &(pHl7Msg[pParsedL1Out->fieldBeginIndices[1]]);
    fieldSize = pParsedL1Out->fieldSizes[1];
    if (fieldSize != 4) {
        //Error - control characters field size must be 4
        return -1;
    }
#ifdef BTLHL7EXP_L2PARSE_EN_PRT_FIELDS
    _btlHl7ExpDbugCopyPrintStr("BTLHL7EXP: INFO0: L2 HL7: found control chars: ", pControlChars, fieldSize);
#endif
    pParser->hl7CompSepChar = pControlChars[0];
    pParser->hl7SubCompSepChar = pControlChars[3];
    pParser->hl7RepeatChar = pControlChars[1];
    pParser->hl7EscapeChar = pControlChars[2];

#ifdef BTLHL7EXP_L2PARSE_EN_PRT_FIELDS
    BTLHL7EXP_DBUG0("BTLHL7EXP: INFO: L2 HL7: component seperator: %c\n", pParser->hl7CompSepChar);
    BTLHL7EXP_DBUG0("BTLHL7EXP: INFO0: L2 HL7: sub-component seperator: %c\n", pParser->hl7SubCompSepChar);
    BTLHL7EXP_DBUG0("BTLHL7EXP: INFO0: L2 HL7: repeat specifier: %c\n", pParser->hl7RepeatChar);
    BTLHL7EXP_DBUG0("BTLHL7EXP: INFO0: L2 HL7: escape character: %c\n", pParser->hl7EscapeChar);
#endif


    return 0;
}  //btlHl7ExpSetMsgControlCharacters

//###########################################################################################
//returns size of component found or -1 if not found
//pAttrParsedCur is loaded with the value found or else cleared to 0
static int btlHl7ExpExtractComponent( char* pField, int fieldSize, HL7MsgAttributeParsed_t* pAttrParsedCur,  BtlHL7ExpSegmentDef_t* pSegDef, char compSeperator) {
     char* pCur = pField;
     char* pLast = pField;
    //char segEndChar = BTL_HL7_SEGMENT_END_CHAR;
    //char fieldSepChar = BTL_HL7_FIELD_SEPERATOR_CHAR;
    int componentNo = pSegDef->componentNo - 1;

    //first componentNo is 0, fieldSize will be non-zero
    int compNo = 0;
    int compSize = 0;
    int forceZeroSize = 0;

    //clear output data - just in case component not found
    pAttrParsedCur->found = 0;
    pAttrParsedCur->size = 0;
    pAttrParsedCur->value[0] = 0;  //null string

    while (fieldSize) {
        fieldSize--;
        if (!((*pCur == compSeperator) ||
            (fieldSize == 0)
            ))
        {
            //character not component seperator/ not end of field - so skip
            pCur++;
            continue;
        }
        //char is component seperator or we reached end of field
        if (compNo != componentNo) {
            //this is not the component asked for - so continue to parse
            compNo++;
            if ((fieldSize == 0) && (*pCur == compSeperator)) {
                //special case: last component may have zero size - check
                if (compNo != componentNo) {
                    return -1;  //component not found
                }
                else {
                    //component found, but has zero size
                    //output will be loaded further below (common code)
                    pLast = pCur;
                    forceZeroSize = 1;
                }
            }
            else {
                //continue to parse
                pCur++;
                pLast = pCur;
                continue;
            }

        }
        //We have found the component - save to L2 parser output data
        compSize = (int)(pCur - pLast);
        if ((forceZeroSize == 0) && (*pCur != compSeperator)) {
            compSize++; //adjust size since char at end is part of the component
        }
        pAttrParsedCur->size = compSize;
        pAttrParsedCur->found = 0;
        if (compSize > 0) {
            pAttrParsedCur->found = 1;
            memcpy(pAttrParsedCur->value, pLast, compSize);
        }
        pAttrParsedCur->value[compSize] = 0;  //null terminate
#ifdef BTLHL7EXP_L2PARSE_EN_PRT_FIELDS
        BTLHL7EXP_DBUG0(" L2 HL7: Attr parsed (Id=%d): %s\n",\
            pSegDef->attrSystemId, pAttrParsedCur->value);
#endif
        break;

    }  //while fieldSize

    return compSize;
}  //btlHl7ExpExtractComponent


//###########################################################################################
static void btlHl7ExtractAttributes(BtlHl7ExpParser_t* pParser, char* pHl7Msg, BtlHL7ExpSegmentDef_t* pSegDef)
{

     char* pField;
    int fieldSize;
    char segName[16];
    HL7MsgAttributeParsed_t* pAttrParsedCur;
    BtlHL7ExpMsgParsedL1_t* pParsedL1Out = pParser->pCurL1SegmentForL2;
    HL7MsgAttributeParsed_t* pAttrParsed = pParser->parsedL2OutAttributes;
    //get segment name from message at index 0
    pField = &(pHl7Msg[pParsedL1Out->fieldBeginIndices[0]]);
    fieldSize = pParsedL1Out->fieldSizes[0];
    segName[0] = 0;
    if (fieldSize < 15) {
        memcpy(segName, pField, fieldSize);
        segName[fieldSize] = 0;
    }
#ifdef BTLHL7EXP_L2PARSE_EN_PRT_FIELDS
    BTLHL7EXP_DBUG0("BTLHL7EXP: INFO0: INFO0: L2 HL7: Parsing Segment: %s\n", segName);
#endif
    while (pSegDef->attrSystemId != BTLHL7_SEG_LAST) {
        //copy the value of this attribute and other details to output

        pAttrParsedCur = &(pAttrParsed[pSegDef->attrSystemId]);
        //first get the field definition from L1 parse output

        pField = &(pHl7Msg[pParsedL1Out->fieldBeginIndices[pSegDef->fieldNo]]);
        fieldSize = pParsedL1Out->fieldSizes[pSegDef->fieldNo];
        if (fieldSize == 0) {
            //attribute value not present in message
#ifdef BTLHL7EXP_L2PARSE_EN_PRT_FIELDS
            BTLHL7EXP_DBUG0("BTLHL7EXP: INFO0: L2 HL7: Attr with ID %d not present in message\n", pSegDef->attrSystemId);
#endif
            pAttrParsedCur->found = 0;
            pAttrParsedCur->size = 0;
            pAttrParsedCur->value[0] = 0;  //empty string
            pSegDef++; //get next attribute
            continue;
        }
        //attribute found in message
        if (pSegDef->componentNo == 0) {
            //no subcomponents - the entire field is one attribute
            //so copy the entire field to output
            memcpy(pAttrParsedCur->value, pField, fieldSize);
            pAttrParsedCur->value[fieldSize] = 0;  //null terminate
            pAttrParsedCur->size = fieldSize;
            pAttrParsedCur->found = 1;


#ifdef BTLHL7EXP_L2PARSE_EN_PRT_FIELDS
            BTLHL7EXP_DBUG0("BTLHL7EXP: INFO0: L2 HL7: Attr parsed (Id=%d): %s\n",\
            pSegDef->attrSystemId, pAttrParsedCur->value);
#endif    
            pSegDef++; //get next attribute
            continue;
        }

        //Handle attributes that are components and sub-components
        btlHl7ExpExtractComponent(pField, fieldSize, pAttrParsedCur, pSegDef, pParser->hl7CompSepChar);
        //reinit to get next attribute required
        pSegDef++; //get next attribute

    } //while
    return;
}  //btlHl7ExtractAttributes()

//Level-2 parsing - extract string values of all known attributes

int btlHl7ExpParseHL7Message_L2(BtlHl7ExpParser_t* pParser,  char* pHl7Msg, int nKnownSegmentsFound) {
    int i;
    int retVal;
    BtlHL7ExpMsgParsedL1_t* pParsedL1Out = pParser->parsedL1OutSegments;
    HL7MsgAttributeParsed_t* pParsedL2OutAttributes = pParser->parsedL2OutAttributes;

    for (i = 0; i < nKnownSegmentsFound; i++) {
        pParser->pCurL1SegmentForL2 = pParsedL1Out;
        BTLHL7EXP_L2PARSE_DBUG0("BTLHL7EXP: INFO1: L2 HL7 parser: Parsing segment type ID: %d\n", pParsedL1Out->segmentTypeId);
        switch (pParsedL1Out->segmentTypeId) {
        case BTLHL7_SEG_MSH:
            retVal = btlHl7ExpSetMsgControlCharacters(pParser, pHl7Msg);
            if (retVal != 0) {
                BTLHL7EXP_L2PARSE_ERR("BTLHL7EXP: ERROR: L2 HL7 parser: FATAL: Invalid control character definition in MSH\n");
                return -1;

            }
            btlHl7ExtractAttributes(pParser, pHl7Msg, gBtlHl7ExpMshSegmentDef);
            break;
        case BTLHL7_SEG_MSA:
            // Handle MSA segment (Acknowledgement)
            btlHl7ExtractAttributes(pParser, pHl7Msg, gBtlHl7ExpMsaSegmentDef);

            break;


        default: BTLHL7EXP_L2PARSE_DBUG0("BTLHL7EXP: INFO: L2 HL7 parser: Skipping unknown segment ID %d! MUST NOT HAPPEN!\n", pParsedL1Out->segmentTypeId);
            break;
        }
        pParsedL1Out++;
        if (pParsedL1Out->segmentTypeId == 0) {
            break;
        }
    } //for

    return 0;
}
//##################################################################################
void btlHl7ExpInitHl7Parser(BtlHl7ExpParser_t* pParser) {

    memset(pParser, 0, sizeof(BtlHl7ExpParser_t));
    return;
}

//##################################################################################

//user access API functions

//Function to get an attribute 
HL7MsgAttributeParsed_t* btlHl7ExpGetAttrById(BtlHl7ExpParser_t* pParser, int attrId) {
    if (attrId >= BTLHL7EXP_N_ATTR_IDS) {
        return 0; //invalid attrId
    }
    return(&(pParser->parsedL2OutAttributes[attrId]));
}
//##################################################################################
//##################################################################################

//Debug print

void _btlHl7ExpDbugCopyPrintStr(char* msg, char* val, int size) {
    char buf[512];
    memcpy(buf, val, size);
    buf[size] = '\n';
    buf[size + 1] = 0;
    BTLHL7EXP_DBUG0("%s %s", msg, buf);
}

#ifdef UNDEF
void btlHl7ExpMsgDump(unsigned char* pBuf, int size)
{
    int i, j;
    unsigned char c;
    int padSize = size % 16;
    padSize = 16 - padSize;
    int sizePadded = size + padSize + 1;
    BTLHL7EXP_DBUG0("\nPrinting HL7 Msg, size=%d, padSize=%d, sizePadded=%d\n", size, padSize, sizePadded);
    for (i = 0; i < sizePadded; i++) {
        if (i % 16 == 0) {
            if (i > 0) {
                //print ASCII
                BTLHL7EXP_DBUG0("   ");
                for (j = i - 16; j < i; j++) {
                    c = pBuf[j];
                    if (c < ' ' || c > '~') {
                        //control char/ non-printable
                        BTLHL7EXP_DBUG0(".");
                    }
                    else {
                        BTLHL7EXP_DBUG0("%c", c);

                    }

                }
            }//if i>0
            if (i > size) {
                BTLHL7EXP_DBUG0("\n");
                return;
            }
            BTLHL7EXP_DBUG0("\n%04d: ", i);
        }

        if (i >= size) {
            BTLHL7EXP_DBUG0("%02x ", 0);
        }
        else {
            BTLHL7EXP_DBUG0("%02x ", pBuf[i]);
        }

    }//main for loop
    BTLHL7EXP_DBUG0("\n");
}  //btlHl7ExpMsgDump()
#endif

void btlHl7ExpMsgDump(unsigned char* pBuf, int size)
{
    int i, j;
    unsigned char c;
    int padSize = size % 16;
    padSize = 16 - padSize;
    int sizePadded = size + padSize + 1;
    unsigned char tmpBuf[256];
    int tmpBufSize = 255;
    unsigned char* pTmpBuf = tmpBuf;
    int nCharsWritten;
    BTLHL7EXP_DBUG0("\nPrinting HL7 Msg, size=%d, padSize=%d, sizePadded=%d\n", size, padSize, sizePadded);
    for (i = 0; i < sizePadded; i++) {
        if (i % 16 == 0) {
            if (i > 0) {
                //print ASCII
                nCharsWritten = snprintf(pTmpBuf, tmpBufSize, "   ");
                tmpBufSize = tmpBufSize - nCharsWritten;
                pTmpBuf += nCharsWritten;
                //BTLHL7EXP_DBUG0("   ");
                for (j = i - 16; j < i; j++) {
                    c = pBuf[j];
                    if (c < ' ' || c > '~') {
                        //control char/ non-printable
                        //BTLHL7EXP_DBUG0(".");
                        nCharsWritten = snprintf(pTmpBuf, tmpBufSize, ".");
                        tmpBufSize = tmpBufSize - nCharsWritten;
                        pTmpBuf += nCharsWritten;
                    }
                    else {
                        //BTLHL7EXP_DBUG0("%c", c);
                        nCharsWritten = snprintf(pTmpBuf, tmpBufSize, "%c", c);
                        tmpBufSize = tmpBufSize - nCharsWritten;
                        pTmpBuf += nCharsWritten;

                    }

                }
            }//if i>0
            if (i > size) {
                //BTLHL7EXP_DBUG0("\n");
                nCharsWritten = snprintf(pTmpBuf, tmpBufSize, "\n");
                tmpBufSize = tmpBufSize - nCharsWritten;
                pTmpBuf += nCharsWritten;
                if (tmpBufSize > 1) {
                    tmpBuf[256 - tmpBufSize] = 0;
                    BTLHL7EXP_DBUG0("%s", tmpBuf);
                    tmpBufSize = 255;
                    pTmpBuf = tmpBuf;
                }

                return;
            }
            //BTLHL7EXP_DBUG0("\n%04d: ", i);
            nCharsWritten = snprintf(pTmpBuf, tmpBufSize, "\n");
            tmpBufSize = tmpBufSize - nCharsWritten;
            pTmpBuf += nCharsWritten;
            if (tmpBufSize > 1) {
                tmpBuf[256 - tmpBufSize] = 0;
                BTLHL7EXP_DBUG0("%s", tmpBuf);
 
            }
            tmpBufSize = 255;
            pTmpBuf = tmpBuf;
            nCharsWritten = snprintf(pTmpBuf, tmpBufSize, "%04d: ", i);
            tmpBufSize = tmpBufSize - nCharsWritten;
            pTmpBuf += nCharsWritten;

        }

        if (i >= size) {
            //BTLHL7EXP_DBUG0("%02x ", 0);
            nCharsWritten = snprintf(pTmpBuf, tmpBufSize, "%02x ", 0);
            tmpBufSize = tmpBufSize - nCharsWritten;
            pTmpBuf += nCharsWritten;
        }
        else {
            //BTLHL7EXP_DBUG0("%02x ", pBuf[i]);
            if (tmpBufSize > 4) {
                nCharsWritten = snprintf(pTmpBuf, tmpBufSize, "%02x ", pBuf[i]);
                tmpBufSize = tmpBufSize - nCharsWritten;
                pTmpBuf += nCharsWritten;
            }
        }

    }//main for loop
    //BTLHL7EXP_DBUG0("\n");
    nCharsWritten = snprintf(pTmpBuf, tmpBufSize, "\n");
    tmpBufSize = tmpBufSize - nCharsWritten;
    pTmpBuf += nCharsWritten;
    if (tmpBufSize > 1) {
        tmpBuf[256 - tmpBufSize] = 0;
        BTLHL7EXP_DBUG0("%s", tmpBuf);
    }
}  //btlHl7ExpMsgDump()


void btlHl7ExportPrintf(int logLevel, char* format, ...) {


    if ((logLevel & gHl7ExpCurrentDebugLevel) == 0) { //If no callback is registered, it prints directly to console.
        return; // Skip if above current debug level
    }
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    char finalBuffer[1200];
    // Add function name in front of the log message
    snprintf(finalBuffer, sizeof(finalBuffer), "%s() - %s", __func__, buffer);

    if (gBtlHl7LogCallbackFn) {
        gBtlHl7LogCallbackFn(logLevel, buffer);
    }
    else {
        // Fallback direct to console
        printf("[Level=%d] %s", logLevel, buffer);
    }
}
void btlHl7ExpSetLogLevel(int logLevel) {
    gHl7ExpCurrentDebugLevel = logLevel;   
}