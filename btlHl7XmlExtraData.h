#ifndef BTL_HL7_XML_EXTRA_DATA_H
#define BTL_HL7_XML_EXTRA_DATA_H 1

#include <libxml/parser.h>
#include <libxml/tree.h>
#include "btlHl7EcgExport.h"

#include <time.h>
#ifdef BTL_HL7_FOR_WINDOWS
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

char* btlHl7GetNodeText(xmlNode* root, char* tag);
int btlHl7ParseProtocolExtraData(BtlHl7Export_t* pHl7Exp);
int btlHl7ParseXmlNg(BtlHl7Export_t* pHl7Exp);
#endif // BTL_HL7_XML_EXTRA_DATA_H
