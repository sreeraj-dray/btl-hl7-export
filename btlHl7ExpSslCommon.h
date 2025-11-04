#ifndef BTLHL7_EXP_SSL_COMMON_H
#define BTLHL7_EXP_SSL_COMMON_H 1

#include "btlHl7EcgExport.h"

#ifdef BTLHL7EXP_SSL_EN

//Peer verification modes
#define BTLHL7EXP_SSL_VERIFY_NONE   (0)
#define BTLHL7EXP_SSL_VERIFY_CA     (1)
#define BTLHL7EXP_SSL_VERIFY_IP     (2)
#define BTLHL7EXP_SSL_VERIFY_HOST   (3)

//SSL Transmit status
#define BTLHL7EXP_SSL_SEND_OK             0
#define BTLHL7EXP_SSL_SEND_RETRY          1
#define BTLHL7EXP_SSL_SEND_FATAL         -1
#define BTLHL7EXP_SSL_SEND_PEER_CLOSED   -2


/* Print peer cert subject/issuer and verification result */
void     print_peer_cert(SSL* ssl, const char* label);

/* Log OpenSSL errors */
void     print_ssl_error(const char* msg);

//################### functions

SSL_CTX* btlHl7ExpCreateClientCtx(BtlHl7Export_t* pHl7Exp);
int btlHl7ExpSslSendOrRetry(BtlHl7Export_t* pHl7Exp);
int btlHl7ExpRxSslData(BtlHl7Export_t* pHl7Exp, char* buf, int buflen);
void btlHl7ExpClientShutdownTcpAndSsl(BtlHl7Export_t* pHl7Exp);

#endif //ifdef BTLHL7EXP_SSL_EN
#endif //ifdef BTLHL7_EXP_SSL_COMMON_H
