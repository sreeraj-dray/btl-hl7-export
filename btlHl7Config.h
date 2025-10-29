#ifndef BTL_HL7_CONFIG_H
#define BTL_HL7_CONFIG_H

#include "btlHl7EcgExport.h"

/// Worklist Config IDs
typedef enum {
    WL_CFG_SERVER_IP = 1,
    WL_CFG_SERVER_PORT,
    WL_CFG_SENDING_APP,
    WL_CFG_SENDING_FACILITY,
    WL_CFG_RECEIVING_APP,
    WL_CFG_RECEIVING_FACILITY,
    WL_CFG_LAST_ID
} Hl7WlConfigId_t;

// Export Config IDs (WARNING: do not change the IDS as the values are directly using in Config Xml)
typedef enum {
    EXP_CFG_SERVER_IP = 1,
    EXP_CFG_SERVER_PORT = 2,
    EXP_CFG_SERVER_TIMEOUT = 3,
    EXP_CFG_PDF_CHUNK_SIZE = 4,
    EXP_CFG_ACK_TIMEOUT= 5,
    EXP_CFG_DEBUG_LEVEL = 6,
    EXP_CFG_SENDING_APP = 7,
    EXP_CFG_SENDING_FACILITY = 8,
    EXP_CFG_RECEIVING_APP = 9,
    EXP_CFG_RECEIVING_FACILITY = 10,
    EXP_CFG_USE_SEP_OBX = 11,
    EXP_CFG_OBX_RESULT_STATUS = 12,

    EXP_CFG_LAST_ID
} Hl7PdfExportConfigId_t;


// Function declaration
int btlHl7ExpLoadXmlConfig(BtlHl7Export_t* pHl7Exp, char* xmlFilePath);

#endif // BTL_HL7_CONFIG_H
