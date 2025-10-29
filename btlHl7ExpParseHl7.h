#ifndef BTLHL7EXP_PARSE_HL7_H
#define BTLHL7EXP_PARSE_HL7_H 1

#include "btlHl7ExpDebug.h"

#include <string.h>

#define BTLHL7EXP_MLLP_MSG_START_CHAR (0x0B)
#define BTLHL7EXP_MLLP_MSG_END_CHAR (0x1c)

//ACK message parsing
#define BTLHL7EXP_MAX_KNOWN_SEGMENTS (4)

#ifndef BTL_HL7_SEGMENT_END_CHAR
//HL7 Standard definitions

#define BTL_HL7_SEGMENT_END_CHAR ((char) '\r')
#define BTL_HL7_FIELD_SEPERATOR_CHAR ((char) '|')

#define MAX_FIELDS_IN_SEGMENT 64

//L1 Parser
#define MAX_SEGMENTS_IN_MESSAGE (256)  //including repeated segments

//System Id used for all segments to mark end of array
#define BTLHL7_SEG_LAST (-1)

#define BTL_HL7_MAX_SEGMENT_NAME_LEN (7)
#define BTL_HL7_SEGMENT_NAME_BUF_LEN (BTL_HL7_MAX_SEGMENT_NAME_LEN + 1)

//L2 parser
#define MAX_SEGMENT_LEN 2048 // Large buffer for segments
#define MAX_FIELD_LEN 256    // Small buffer for field

//Data structure to keep the parsed values of attribute (string format)
//Used in Level-2 parsing
//An array having size equal to the number of known attributes will be used for each segment
//The attribute's index in the array will be the systemId of the attribute

typedef struct HL7MsgAttributeParsed {
    int size;
    int found;
    char value[MAX_FIELD_LEN];
} HL7MsgAttributeParsed_t;

#endif  //ifndef BTL_HL7_SEGMENT_END_CHAR

//################################################################################################
//################################################################################################
//maximum known atttribute ids
#define BTLHL7EXP_N_ATTR_IDS (16) //This must be more then the max attr id defined below

//Segment Type IDs - Do not use 0 or negative numbers
#define BTLHL7_SEG_MSH (1)
#define BTLHL7_SEG_MSA (2)  //ack segment


//system Ids for MSH known attributes
#define BTLHL7EXP_MSH_N_ATTR           (7)  //always make this number of attr defined below
#define	BTLHL7EXP_MSH_CONTROL_CHARS	    (0)
#define BTLHL7EXP_MSH_MSG_TYPE			(1)
#define BTLHL7EXP_MSH_MSG_TYPE_ID		(2)
#define BTLHL7EXP_MSH_SENDING_APP_1   (3)
#define BTLHL7EXP_MSH_SENDING_APP_2   (4)
#define BTLHL7EXP_MSH_SENDING_FAC_1   (5)
#define BTLHL7EXP_MSH_SENDING_FAC_2   (6)


//system Ids for MSA known attributes
#define BTLHL7EXP_MSA_N_ATTR           (1)  //always make this number of attr defined below
#define	BTLHL7EXP_MSA_ACK_STAT	(10)

//################################################################################

typedef struct BtlHL7ExpMsgParsedL1 {
    int segmentStartIndex;
    int segmentSize;
    int segmentTypeId;
    int nFieldsFound;
    int fieldBeginIndices[MAX_FIELDS_IN_SEGMENT];
    int fieldSizes[MAX_FIELDS_IN_SEGMENT];
} BtlHL7ExpMsgParsedL1_t;

//struct to define the known attributes in a segment
// Used in Level-2 parsing
//An array of this struct will be used for each segment type to
    //define the known attributes of the segment to be parsed
typedef struct BtlHL7ExpSegmentDef {
    int attrSystemId;  //BTL defined system ID for the attribute
    int fieldNo;  //attribute's field number in the segment
    int componentNo; //attribute's component number in the field (0 if entire field)
    int subComponentNo; //attribute's sub-component number in the component (0 if entire component)
    int required;
} BtlHL7ExpSegmentDef_t;

typedef struct BtlHl7ExpParser {
    BtlHL7ExpMsgParsedL1_t parsedL1OutSegments[BTLHL7EXP_MAX_KNOWN_SEGMENTS];
    BtlHL7ExpMsgParsedL1_t* pCurL1SegmentForL2;
    int nKnownSegmentsFound;
    int nKnownSegmentsLeft;

    //Following must be loaded from MSH segment at the begining of
    //level-2 parsing for every message
    char hl7CompSepChar;
    char hl7SubCompSepChar;
    char hl7RepeatChar;
    char hl7EscapeChar;

    //Buffer for parsed out attributes from all Segments
    //The data for an attribute will be at index attribute id inside the array
    HL7MsgAttributeParsed_t parsedL2OutAttributes[BTLHL7EXP_N_ATTR_IDS];

} BtlHl7ExpParser_t;
//################################################################################
//Functions:
//API:
void btlHl7ExpInitHl7Parser(BtlHl7ExpParser_t* pParser);
int btlHl7ExpParseHL7Message_L1(BtlHl7ExpParser_t* pParser, char* pHl7Msg);
int btlHl7ExpParseHL7Message_L2(BtlHl7ExpParser_t* pParser, char* pHl7Msg, int nKnownSegmentsFound);
HL7MsgAttributeParsed_t* btlHl7ExpGetAttrById(BtlHl7ExpParser_t* pParser, int attrId);

//Internal
void _btlHl7ExpDbugCopyPrintStr(char* msg, char* val, int size);
void btlHl7ExpMsgDump(unsigned char* pBuf, int size);
void btlHl7ExpSetLogLevel(int logLevel);
void btlHl7ExportPrintf(int logLevel, char* format, ...);
#endif //ifndef BTLHL7EXP_PARSE_HL7_H
