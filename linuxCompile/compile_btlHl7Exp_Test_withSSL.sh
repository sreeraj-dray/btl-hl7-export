
     
SRC_FILES="btlHl7EcgExport.c   btlHl7ExpParseHl7.c  btlHl7XmlNg.c btlHl7XmlExtraData.c btlHl7Config.c btlHl7ExpSslCommon.c"

MAIN_FILE=btlHl7EcgExportTestmain.c 
OUTPUT_EXE=btlHl7EcgExportTestmain
LIBXML2_INC_DIR=/usr/include/libxml2

LIBS="-lxml2 -lssl -lcrypto"

#echo SRC FILES: $SRC_FILES
#echo Main File: $MAIN_FILE
#echo Running gcc -o $OUTPUT_EXE  $MAIN_FILE $SRC_FILES

rm -f $OUTPUT_EXE
gcc  -o $OUTPUT_EXE -DWSL_LINUX -I$LIBXML2_INC_DIR $MAIN_FILE $SRC_FILES $LIBS
ls -l $OUTPUT_EXE

