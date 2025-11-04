
Note:
=====
Dummy HIS Server runs at port 23727 or 23856 (use the appropriate executables). It accepts connection for PDF export, prints the HL7 messages it
receives and send ACk once a complete HL7 message is received. It continues to wait for client
connections once a client disconnects.

In the filename, the delay before sending client ACK is specified : Example: tcpTestServer_port-23727_ackDelay-10s.exe delays the ACK response by 10+ seconds so that 
ACK timeout at the client can be tested.

The executables starting with SSL_ are for testing secure TLS connections from HL7 export client. These need certificates, key etc in the subfolder certs.
