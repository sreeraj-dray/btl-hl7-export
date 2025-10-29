#ifndef BTLHL7EXP_DEBUG_H
#define BTLHL7EXP_DEBUG_H 1

//##################################### Main Debug Print Control ###########
// Uncomment below line for getting debug prints. Finer control can
// be obtained by defining BTLHL7EXP_DBUG0, BTLHL7EXP_DBUG1 and BTLHL7EXP_ERR further 
// below individually

#define BTLHL7_SRV_DBUGEN 1

#define BTLHL7EXP_DEFAULT_LOG_LEVEL (BTL_HL7EXP_DBG_LVL_ERR | BTL_HL7EXP_DBG_LVL_DBG0 | BTL_HL7EXP_DBG_LVL_DBG1) //for testing

//#define BTLHL7EXP_DEFAULT_LOG_LEVEL (0)

	//	Uncomment below to debug the parser - normally not required (lot of print lines)
#define BTLHL7_SRV_PARSER_DBUGEN 1
// 
//###########################################################################
//comment out below to test the TCP server
//#define PARSE_XML_TEST 1  //only for test main app
#define BTLHL7EXP_DBUG_HL7_MSG_PRT_EN 1

#include <stdio.h>
#include <stdarg.h>  // for va_list



// #########################################################################




// Declare log callback type
typedef void (*BtlHl7LogCallback_t)(int logLevel, char* msg);


// Function declaration
//void btlHl7RegisterLogCallback(BtlHl7LogCallback_t cb);
//void btlHl7ExportPrintf(const char* fmt, ...);

// Client log levels (bit flags)
#ifdef UNDEF
enum BtlHl7ExpLogLevel {
    NONE = 0,
    AUDIT = 1,      //Not used by HL7 library
    HIS_ERROR = 2,  //ERR logs in HL7 Library
    WARNING = 4,    //Not used by HL7 library
    INFO = 8,       //Not used by HL7 library
    DEBUG = 16,     //DEBUG1 level in HL& Library, brief logs
    DEBUG2 = 32     //DEBUG0 level in HL& Library, more verbose
};
#endif

#define BTL_HL7EXP_DBG_LVL_ERR (2)
#define BTL_HL7EXP_DBG_LVL_DBG0 (32)
#define BTL_HL7EXP_DBG_LVL_DBG1 (16)


#ifdef BTLHL7_SRV_DBUGEN 
#else
#define BTLHL7EXP_DEFAULT_LOG_LEVEL (0)
#endif

#ifdef BTLHL7_SRV_DBUGEN
//DISABLED BELOW PRT
#define BTLHL7EXP_DBG_PRT_ACK_MSG_RCVD 1
#define BTLHL7EXP_DBUG_HL7_MSG_PRT_EN 1

// Base macros: these call your logging function
#define BTLHL7EXP_ERR(fmt, ...)    btlHl7ExportPrintf(BTL_HL7EXP_DBG_LVL_ERR, fmt, ##__VA_ARGS__)
#define BTLHL7EXP_DBUG0(fmt, ...)  btlHl7ExportPrintf(BTL_HL7EXP_DBG_LVL_DBG0, fmt, ##__VA_ARGS__)
#define BTLHL7EXP_DBUG1(fmt, ...)  btlHl7ExportPrintf(BTL_HL7EXP_DBG_LVL_DBG1, fmt, ##__VA_ARGS__)


#else
// Disabled: compile to nothing
#define BTLHL7EXP_ERR(...)
#define BTLHL7EXP_DBUG0(...)
#define BTLHL7EXP_DBUG1(...)
#define BTLHL7EXP_L1PARSE_ERR(...)
#define BTLHL7EXP_L1PARSE_DBUG0(...)
#define BTLHL7EXP_L2PARSE_ERR(...)
#define BTLHL7EXP_L2PARSE_DBUG0(...)
#endif

#ifdef BTLHL7_SRV_PARSER_DBUGEN
//DISABLED BELOW DEBUG PRINTS
// Aliases for parser-specific logging
#define BTLHL7EXP_L1PARSE_ERR(...)    BTLHL7EXP_ERR(__VA_ARGS__)
#define BTLHL7EXP_L1PARSE_DBUG0(...)  BTLHL7EXP_DBUG0(__VA_ARGS__)
#define BTLHL7EXP_L2PARSE_ERR(...)    BTLHL7EXP_ERR(__VA_ARGS__)
#define BTLHL7EXP_L2PARSE_DBUG0(...)  BTLHL7EXP_DBUG0(__VA_ARGS__)

#else
#define BTLHL7EXP_L1PARSE_ERR(...)   
#define BTLHL7EXP_L1PARSE_DBUG0(...)  
#define BTLHL7EXP_L2PARSE_DBUG0(...)
#define BTLHL7EXP_L2PARSE_ERR(...)
#endif

#define btlHl7ExpDbugCopyPrintStr _btlHl7ExpDbugCopyPrintStr
//#define BTLHL7EXP_L1PARSE_EN_PRT_FIELDS 1  //btlHl7ExpDbugCopyPrintStr must also be defined above
//#define BTLHL7EXP_L2PARSE_EN_PRT_FIELDS 1  //btlHl7ExpDbugCopyPrintStr must also be defined above



#endif //BTLHL7EXP_DEBUG_H