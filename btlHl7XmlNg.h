#ifndef BTL_XML_NG_ATTR_MAP_H
#define BTL_XML_NG_ATTR_MAP_H 1


#include "btlHl7EcgExport.h"

#define BTLHL7EXP_XML_NG_ATTR_COUNT (32)//Should me more then the max xmlng attribute Id

#define BTLHL7EXP_XMLNG_ATTR_ID_EXAM_TYPE (0)
#define BTLHL7EXP_XMLNG_ATTR_ID_RR_AVG_DIST (1)
#define BTLHL7EXP_XMLNG_ATTR_ID_P_DURATION (2)
#define BTLHL7EXP_XMLNG_ATTR_ID_PQ_DURATION (3)
#define BTLHL7EXP_XMLNG_ATTR_ID_QRS_DURATION (4)
#define BTLHL7EXP_XMLNG_ATTR_ID_QT_DURATION (5)
#define BTLHL7EXP_XMLNG_ATTR_ID_P_AXIS (6)
#define BTLHL7EXP_XMLNG_ATTR_ID_QRS_AXIS (7)
#define BTLHL7EXP_XMLNG_ATTR_ID_T_AXIS (8)
#define BTLHL7EXP_XMLNG_ATTR_ID_QTC_DURATION (9)
#define BTLHL7EXP_XMLNG_ATTR_ID_LHV_SOKOLOV (10)
#define BTLHL7EXP_XMLNG_ATTR_ID_LHV_CORNELL (11)
#define BTLHL7EXP_XMLNG_ATTR_ID_LHV_ROMHILT (12)
#define BTLHL7EXP_XMLNG_ATTR_ID_CONCLUSION (13)
#define BTLHL7EXP_XMLNG_ATTR_ID_MED_FINDING (14)


typedef struct {
    char* code;
    char* label;
    char* unit;
    int xmlNgAttrId;
} BtlXmlNgAttrMap_t;


typedef struct {
    int xmlNgAttrId;  // XML attribute ID (e.g., BTLHL7EXP_XMLNG_ATTR_ID_RR_AVG_DIST)
    const char* ecgMeasurement;  // ECG measurement code (e.g., "RR")
} BtlXmlNgAttrToEcgMeasMap_t;



extern int gMeasurementAttrIds[];
extern HL7ExpAttributeParsed_t g_BtlHl7ExpXmlNgAttributesParsed[BTLHL7EXP_XML_NG_ATTR_COUNT];
extern  BtlXmlNgAttrMap_t btlXmlNgAttrMap[BTLHL7EXP_XML_NG_ATTR_COUNT];


#ifdef UNDEF
static  BtlXmlNgAttrMap_t btlXmlNgAttrMap[] = {
    {
        .code = "RR_AvgDistance_05ms"
        ,.label = "RR"
        ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_RR_AVG_DIST
    },

   {
    .code = "P_Duration_05ms"
   ,.label = "P"
   ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_P_DURATION
},
{
    .code = "PQ_Duration_05ms"
   ,.label = "PQ (PR)"
   ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_PQ_DURATION
},
{
    .code = "QRS_Duration_05ms"
   ,.label = "QRS"
   ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_QRS_DURATION
},
{
    .code = "QT_Duration_05ms"
   ,.label = "QT"
   ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_QT_DURATION
},

{
        .code = "P_Axis_deg"
        ,.label = "P axis"
        ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_P_AXIS
    },

    {
        .code = "QRS_Axis_deg"
        , .label = "QRS axis"
         ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_QRS_AXIS
    },

    {
        .code = "T_Axis_deg"
        ,.label = "T axis"
        ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_T_AXIS
    },


    {
        .code = "QTc_Duration_05ms"
        ,.label = "QTc(Baz)"
         ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_QTC_DURATION
    },

    {
        .code = "LVH_Sokolow_mm"
        ,.label =  "Sokolow"
         ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_LHV_SOKOLOV
    },

    {
        .code = "LVH_Cornell_mm"
        ,.label =  "Cornell"
   ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_LHV_CORNELL
    },

    {
        .code = "LVH_Romhilt_points"
        ,.label =  "Romhilt"
    ,.xmlNgAttrId = BTLHL7EXP_XMLNG_ATTR_ID_LHV_ROHMILT
};

#define BTL_XML_NG_ATTR_COUNT (sizeof(btlXmlNgAttrMap) / sizeof(BtlXmlNgAttrMap_t))
#endif

 extern int gBtlHl7ExpMeasAttrIds[];
//int btlHl7ParseDiagnosticsXml(BtlHl7Export_t* pHl7Exp);
int btlHl7ParseXmlNg(BtlHl7Export_t* pHl7Exp);

int btlHl7ExportFallback(BtlHl7Export_t* p);
int btlHl7BuildMinimalFromXmlNg(BtlHl7Export_t* p);

void btlHl7ExpAddEcgMeasToHl7_1(BtlHl7Export_t* pHl7Exp, int* pMeasAttrIds);
void btlHl7ExpAddEcgMeasToHl7_0(BtlHl7Export_t* pHl7Exp, int* pMeasAttrIds);
void btlHl7ExpAddDoctorStatement(BtlHl7Export_t* pHl7Exp);
void btlHl7InitXmlNgParser(BtlHl7Export_t* pHl7Exp);


#endif // BTL_XML_NG_ATTR_MAP_H
