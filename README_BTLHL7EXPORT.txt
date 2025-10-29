
BTL HL7 ECG Export Library
===========================
Date: 07 July 2025, Rel-0.1

Notes: 
========

1. While compiling for Linux with gcc, the libxml2 library must be specified (-lxml2 or whatever is 
appropriate for the target platform). The folder containing the libxml2 include files/folders may 
also have to be specified as a -I gcc command line option (example: -I/usr/include/libxml2).
2. While compiling for Windows, the libxml2 include files provided in libxml2\inc folder must be 
used. Add this folder to the include folders configuration or specify in compiler command line. Also 
specify the dll library to link to and add library directory too to the libraries path. The windows 
libxml2 dlls are present in x64\Debug directory (libxml2.dll as well as dependencies iconv-2.dll and
zlib1.dll). They are also provided in ext_dlls folder. iconv\include folder contains include files 
for iconv used by libxml. This path must also be added to the include folder path.

In Visual Studio project properties, under C/C++ => General :: Additional Include Directories, the 
relative paths $(SolutionDir)\iconv\include;$(SolutionDir)\libxml2\inc; can be added. Also, the 
library folder can be specified in Linker==>General :: Additional Library Directories with relative
path $(SolutionDir)\ext_lib. In addition, the libxml dll and dependent dlls must be copied to the
output directory where the executable is kept (for example x64\Debug folder, same as the 
macro $(OutDir))

3. For testing (only for running the test program), the pdf file to export and the BTL XML-NG file
should be kept in the same directory from where the executable is run (for example, x64\Debug). The
files provided are hl7Test_1.pdf and d60b513e-d780-4ba0-8e57-a66d2ff8d42b.diagnostics.xml. The 
directory from where the program is run is printed on the console when the test program is run to 
aid debugging. If the error message is printed to inform that the input pdf and xml files failed
to be opened, then these files are not in the working directory from where the program is run. For
Visual Studio, the working directory can be set to be the output directory by setting
Properties => Debugging :: Working Directory to the macro $(OutDir).


4. A test program that demonstrates the use of the library is also made available in file named
btlHl7EcgExportTestmain.c. The test has a dependency on the HIS PDF export server being present
at IpAddress:Port = 127.0.0.1:23727 . The server IP and port may be changed by modifying the 
following variables in tlHl7EcgExportTestmain.c.
char gHl7ExpSrvIpAddrStr[16] = "127.0.0.1";
uint16_t gHl7ExpSrvPort = 23727;

There is a dummy HIS server (only for windows PC) to quickly aid running the test program that 
will print the messages it receives and send back acknowledgement. The executable file is made 
available in 

HL7 Export Library source files are:
=====================================

btlHl7EcgExport.c
btlHl7EcgExport.h    	//Include this file in application to access the API functions
btlHl7ExpDebug.h	//Debug logging can be controlled from this file
btlHl7ExpParseHl7.c	//HL7 parser logic for parsing received ACK messages
btlHl7ExpParseHl7.h
btlHl7XmlNg.c		//BTL XML-NG parsing and meta data access
btlHl7XmlNg.h
btlHl7XmlExtraData.c	//ProtocolExtraData XML parsing (mete data from received work order HL7 message)
btlHl7XmlExtraData.h

btlHl7EcgExportTestmain.c : Not a library source file. Provides an example usage of the library to perform
df file export.

Other folders and files:
========================
libxml2\inc 		: libxml2 include files for windows
testInputFiles		: Sample pdf and BTL-XML-NG files for testing export operation
linuxCompile		: Folder containing simple compile script to build test executable using gcc for the Linux platform
iconv\include 		: Include files for iconv (libxml2 dependency)
ext_dlls		: External binary dlls (libxml2 and its dependencies)- only for windows platform
ext_lib			: External .lib files (libxml2 and its dependencies)- only for windows platform
dummyHIS_TestServer	: A dummy HIS server for testing pdf export. See README file in the dir.

Other folders and files such as .\x64, btlHl7EcgExport.sln are standard Visual Studio folders/ files. 

Compiling for Linux:
=====================
IMPORTANT: Install the libxml2 development package on ubuntu (or other distribution) before compiling:
sudo apt-get update
sudo apt-get install libxml2-dev

The directory by name linuxCompile contains the script compile_btlHl7Exp_Test.sh that
provides a basic script to compile the test executable. You may need to update the LIBXML2_INC_DIR and 
LIBS variables to point to the correct libxml2 include dir and shared object library. Run in the 
project root directory where all the library .c and .h files are present:

./linuxCompile/compile_btlHl7Exp_Test.sh

After compiling, first run the HIS server (either dummy test server provided or other HL7 export server):
NOTE: If Windows shows a popup asking to allow connection over the network for this application. allow
the permissions or the test program will not be able to connect to the dummy test server and will 
therefore fail to export the file.

	PS D:\btlHl7EcgExport\dummyHIS_TestServer> .\dummyHIS_Server_atPort_23727.exe
	Server listening on port 23727...
	Waiting for a client to connect...

Then run the compiled test export executable:
	./btlHl7EcgExportTestmain

The output from the test program would look like below (when using dummy test server):
=======================================================================================

Starting BTL HL7 PDF export test program..
Server Ip/ Port: 172.20.80.1/ 23727
Executing from directory: /proj/hl7export/btlHl7Export
================================

BTLHL7EXP: Export command received.
BTLHL7EXP: Initiating connection.
BTLHL7EXP INFO: Socket creation successful!
BTLHL7EXP: Connecting - waiting for server response
BTLHL7EXP: Processing export...
BTLHL7EXP: INFO: Transmitting chunk of 705 bytes (total till now=705)
BTLHL7EXP INFO: Pdf file size is 9309 bytes
BTLHL7EXP INFO: pdf OBX segment (pdf file left=3459 bytes):
BTLHL7EXP: INFO: Transmitting chunk of 7829 bytes (total till now=8534)
BTLHL7EXP INFO: pdf OBX segment (pdf file left=0 bytes):
BTLHL7EXP: INFO: Transmitting chunk of 4620 bytes (total till now=13154)
BTLHL7EXP: INFO: Waiting for server ack!
BTLHL7EXP: Export completed. HL7 Msg Total Length = 13154 bytes, waiting for srv ack..
BTLHL7EXP TEST : Export Pdf is in progress.
BTLHL7EXP TEST : Export Pdf is in progress.
BTLHL7EXP: INFO: ACK msg recvd!
BTLHL7EXP: INFO: ACK Status Rx is (OK): AA
BTLHL7EXP: INFO: Ack received from server!
BTLHL7EXP TEST : Export Pdf completed successfully.
BTLHL7EXP: Thread shutdown command received; Exiting thread!
BTLHL7EXP TEST: Client thread shutdown completed!

