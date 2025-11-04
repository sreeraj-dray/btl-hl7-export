
#include "btlHl7ExpSslCommon.h"

#ifdef BTLHL7EXP_SSL_EN

static int isIpAddressStr(const char* s) {
    unsigned char buf[16];
    return s && (inet_pton(AF_INET, s, buf) == 1 || inet_pton(AF_INET6, s, buf) == 1);
}

/* Configure verification on an SSL* according to mode.
 * target = the string to verify against (IP address string for mode 2, hostname for mode 3).
 * For mode 1/0, target may be NULL.
 * If a hostname is set by user when connecting by IP, pass it in sni_host to help virtual hosts (mode 3 sets SNI automatically).
 */
static int configure_peer_verification(BtlHl7Export_t* pHl7Exp, int mode, const char* target, const char* sni_host_optional)
{
    SSL* ssl = pHl7Exp->ssl;  //Will not be NULL if we reach here

    switch (mode) {
    case BTLHL7EXP_SSL_VERIFY_NONE:
        BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: WARNING: Peer verification is disabled!\n");
        SSL_set_verify(ssl, SSL_VERIFY_NONE, NULL);
        return 1;

    case BTLHL7EXP_SSL_VERIFY_CA:
        /* Chain validation only; no reference-identity (name) check */
        BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: Peer certificate verification is enabled!\n");
        SSL_set_verify(ssl, SSL_VERIFY_PEER, NULL);
        /* Do NOT call SSL_set1_host / set1_ip here */
        return 1;

    case BTLHL7EXP_SSL_VERIFY_IP: {
        if (!target || !isIpAddressStr(target)) {
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: SSL: BTLHL7EXP_SSL_VERIFY_IP requires an IP literal in 'target'\n");
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_IPADDR;
            return 0;
        }
        SSL_set_verify(ssl, SSL_VERIFY_PEER, NULL);

        //1.0.2: use X509_VERIFY_PARAM directly; //TBD openssl_v1.1.x
        X509_VERIFY_PARAM* p = SSL_get0_param(ssl);
        if (!p) { 
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SSL_GET0_PARAM;
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: SSL: SSL_get0_param().\n");
            return 0; 
        }
        if (X509_VERIFY_PARAM_set1_ip_asc(p, target) != 1) {
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SSL_SET1_IP_ASC;
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: SSL: X509_VERIFY_PARAM_set1_ip_asc().\n");
            return 0;
        }

        /* Note: SNI requires a DNS name; do not set SNI to an IP */
        if (sni_host_optional && *sni_host_optional) {
            /* If user has set the hostname behind this IP and the server is virtual-hosted,
               setting SNI helps the server choose the right cert—even though verify is by IP only. */

            if (strchr(sni_host_optional, '*') == NULL) {
                //no wildcards in host name, so set SNI
                //SNI so the server picks the right cert on virtual hosts 
                SSL_set_tlsext_host_name(ssl, sni_host_optional);
            }
            else {
                BTLHL7EXP_DBUG0("BTLHL7EXP: SSL: Not setting SNI hostname(1)!\n");
            }
        }
        BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: Peer IP address verification is enabled!\n");
        return 1;
    }

    case BTLHL7EXP_SSL_VERIFY_HOST: {
        if (!target || isIpAddressStr(target)) {
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: SSL: BTLHL7EXP_SSL_VERIFY_HOST requires a DNS hostname in 'target'\n");
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SSL_HOST_MATCH_STR;
            return 0;
        }
        SSL_set_verify(ssl, SSL_VERIFY_PEER, NULL);

        if (strchr(target, '*') ==  NULL) {
                //no wildcards in host name, so set SNI
                //SNI so the server picks the right cert on virtual hosts 
            SSL_set_tlsext_host_name(ssl, target);
        }
        else {
            BTLHL7EXP_DBUG0("BTLHL7EXP: SSL: Not setting SNI hostname(2)!\n");
        }

        //1.0.2: use X509_VERIFY_PARAM directly; //TBD openssl_v1.1.x
        X509_VERIFY_PARAM* p = SSL_get0_param(ssl);
        if (!p) { 
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SSL_GET0_PARAM;
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: SSL: SSL_get0_param().\n");
            return 0; 
        }
        if (X509_VERIFY_PARAM_set1_host(p, target, 0) != 1) {
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SSL_SET1_HOST;
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: SSL: X509_VERIFY_PARAM_set1_host().\n");
            return 0; 
        } // 0 = strlen auto; wildcards allowed
        BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: Peer hostname verification is enabled!\n");
        return 1;
    }

    default:
        BTLHL7EXP_ERR("BTLHL7EXP ERROR: SSL: Unknown peer verify mode: %d, must be 0, 1, 2 or 3!\n", mode);
        pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_INVALID_VERIFY_MODE;
        return 0;
    }
}  //configure_peer_verification()

static void opensslInitOnce(BtlHl7Export_t* pHl7Exp) {
    //TBD openssl_v1.1.x
    if (pHl7Exp->sslInitDone == 0) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        pHl7Exp->sslInitDone = 1;
        BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: Openssl library initialised!\n");

    }
    return;
}  //opensslInitOnce()

    //creates SSL objects, returns 0 (NULL) on error, error status is set in  pHl7Exp->exportStatus 
SSL_CTX* btlHl7ExpCreateClientCtx(BtlHl7Export_t* pHl7Exp)
{
    int retVal;
        opensslInitOnce(pHl7Exp);

        pHl7Exp->sslCtx = SSL_CTX_new(SSLv23_client_method()); //TBD openssl_v1.1.x
        if (!pHl7Exp->sslCtx) { 
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SSL_CTX_CR;
            BTLHL7EXP_ERR("BTLHL7EXP ERROR: SSL: SSL_CTX_new().\n");
            return NULL; 
        }
        // Disable SSLv2/3 and other older TLS versions <1.2 on openssl 1.0.2 just in case
        SSL_CTX_set_options(pHl7Exp->sslCtx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1); //TBD openssl_v1.1.x
        //SSL_CTX_set_options(pHl7Exp->sslCtx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

        // Optional hardening on 1.0.x: Disable TLS-level compression
        //TBD openssl_v1.1.x
#ifdef SSL_OP_NO_COMPRESSION
       // SSL_CTX_set_options(pHl7Exp->sslCtx, SSL_OP_NO_COMPRESSION);
#endif

        //Set up peer verification based on user configuration
        if (pHl7Exp->sslPeerVerifyMode) {
                //user has configured to verify peer's (server's) certficate
                //The server's CA certificate file path must be available or else abort

            if ((*(pHl7Exp->peer_ca_file) == 0) || SSL_CTX_load_verify_locations(pHl7Exp->sslCtx, pHl7Exp->peer_ca_file, NULL) != 1) {
                SSL_CTX_free(pHl7Exp->sslCtx); 
                pHl7Exp->sslCtx = NULL;
                if (*(pHl7Exp->peer_ca_file) == 0) {
                    pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SSL_NO_PEER_CA_FILE;
                    BTLHL7EXP_ERR("BTLHL7EXP ERROR: Peer CA file not congiured!\n");
                }
                else {
                    pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SSL_LD_VERIFY_LOCS;
                    BTLHL7EXP_ERR("BTLHL7EXP ERROR:SSL_CTX_load_verify_locations()!\n");
                }
                return NULL;
            }
        }
        else {
            BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: Peer certificate/ host/ IP verification is disabled!\n");
        }

        // Ensure ECDHE is usable (1.0.2 needs this on some builds)
#ifdef SSL_CTX_set_ecdh_auto
        //SSL_CTX_set_ecdh_auto(pHl7Exp->sslCtx, 1); //Not to be done in client
#endif

        SSL_CTX_set1_curves_list(pHl7Exp->sslCtx, "P-256:P-384:P-521");

        // Exclude weak TLS 1.2 ciphers: "HIGH:!aNULL:!SHA1:!RC4:!3DES:!MD5"
        //Or explicitly include strong ciphers : "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-RSA-AES128-SHA256"
        //TBD openssl_v1.1.x may not need this
#ifdef UNDEF
        retVal = SSL_CTX_set_cipher_list(pHl7Exp->sslCtx,
            "ECDHE+AESGCM:ECDHE+CHACHA20:"   // AEAD with PFS
       "!aNULL:!eNULL:!MD5:!RC4:!3DES:!DES:!RC2:!PSK:!SRP:!DSS");
#else
        //retVal = SSL_CTX_set_cipher_list(pHl7Exp->sslCtx, "HIGH:!aNULL:!SHA1:!RC4:!3DES:!MD5");
        const char* client_ciphers =
            "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:"
            "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:"
            "ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA384:"
            "ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256"
            ":!aNULL:!eNULL:!MD5:!RC4:!3DES:!DES:!RC2:!SRP:!PSK:!DSS";

        retVal = SSL_CTX_set_cipher_list(pHl7Exp->sslCtx, client_ciphers);
#endif    

        if (retVal != 1) {
            // Handle error
            BTLHL7EXP_ERR("BTLHL7EXP ERR: SSL SSL_CTX_set_cipher_list() failed! Aborting export.\n");
            return NULL;
        }
        // (Optional) tighten TLS1.2 sigalgs we’ll accept from the server 
#ifdef SSL_CTX_set1_sigalgs_list
        SSL_CTX_set1_sigalgs_list(pHl7Exp->sslCtx, "RSA+SHA256:RSA+SHA384:ECDSA+SHA256:ECDSA+SHA384:AES256-SHA256:AES128-SHA256");
#endif

        //======================================== Create the SSL object
        pHl7Exp->ssl = SSL_new(pHl7Exp->sslCtx);
        if (!pHl7Exp->ssl) { 
            pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SSL_NEW_CREATE;
            BTLHL7EXP_ERR("BTLHL7EXP ERROR:SSL_new()!\n");
            return NULL;
        }
        //========================================
        char* target = pHl7Exp->srvIpAddrStr;
        char* sni_host_optional = pHl7Exp->sslPeerHostNameMatchStr;
        retVal = 1;
        if (pHl7Exp->sslPeerVerifyMode == BTLHL7EXP_SSL_VERIFY_HOST) {
            BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: Peer hostname verification is enabled! Hostname to match=%s\n", pHl7Exp->sslPeerHostNameMatchStr);
            target = pHl7Exp->sslPeerHostNameMatchStr;
            retVal = configure_peer_verification(pHl7Exp, pHl7Exp->sslPeerVerifyMode, target, NULL);
        }
        else if (pHl7Exp->sslPeerVerifyMode == BTLHL7EXP_SSL_VERIFY_IP) {
            BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: Peer IP address verification is enabled! Address to match=%s, sni_hostname=%s\n", pHl7Exp->srvIpAddrStr, sni_host_optional);
            retVal = configure_peer_verification(pHl7Exp, pHl7Exp->sslPeerVerifyMode, target, sni_host_optional);
        }
        else {
            retVal = configure_peer_verification(pHl7Exp, pHl7Exp->sslPeerVerifyMode, NULL, NULL);
        }

        if (retVal == 0) {
            BTLHL7EXP_ERR("BTLHL7EXP ERR: SSL configure_peer_verification() failed! Aborting export.\n");
            SSL_CTX_free(pHl7Exp->sslCtx);
            pHl7Exp->sslCtx = NULL;
            return NULL;  //error status already set above
        }

        if (pHl7Exp->sslUseOwnCertificate) {
                //client to use own certificate
            BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: sslUseOwnCertificate is true.\n");

            int certOrKeyFileErr = 0;
            if (*(pHl7Exp->own_key_file) && *(pHl7Exp->own_cert_file)) {
            }
            else {
                certOrKeyFileErr = 1;
                BTLHL7EXP_ERR("BTLHL7EXP ERROR:Key and/or own certificate file not configured!\n");
            }

            BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: Own cert file: %s\n", pHl7Exp->own_cert_file);
            BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: Own key file: %s\n", pHl7Exp->own_key_file);


            if (certOrKeyFileErr!=0 || 
                SSL_CTX_use_certificate_file(pHl7Exp->sslCtx, pHl7Exp->own_cert_file, SSL_FILETYPE_PEM) != 1 ||
                SSL_CTX_use_PrivateKey_file(pHl7Exp->sslCtx, pHl7Exp->own_key_file, SSL_FILETYPE_PEM) != 1 ||
                SSL_CTX_check_private_key(pHl7Exp->sslCtx) != 1) 
            {
                SSL_CTX_free(pHl7Exp->sslCtx); 
                pHl7Exp->sslCtx = 0;
                pHl7Exp->exportStatus = BTLHL7EXP_STATUS_ERR_SSL_OWN_CERT_KEY_FILE_S;
                BTLHL7EXP_ERR("BTLHL7EXP ERROR:Key and/or own certificate file not OK!\n");

                if (certOrKeyFileErr == 0) {
                    BTLHL7EXP_ERR("BTLHL7EXP ERROR:SSL_CTX_use_certificate_file(), key etc!\n");
                }
                return NULL;
            }
        }
        else {
            BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: sslUseOwnCertificate is false.\n");
        }

   return pHl7Exp->sslCtx;
}  //btlHl7ExpCreateClientCtx()

//#################################################################################################
//SSL Transmit helpers
//#################################################################################################

 // Helper: check if the last OS error is "would block"
static int isWouldBlock(void) {
#ifdef _WIN32
    int w = WSAGetLastError();
    return (w == WSAEWOULDBLOCK || w == WSAEINTR);
#else
    return (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR);
#endif
}

//non-blocking write of exactly len bytes (or tell caller to retry) 
int btlHl7ExpSslSendOrRetry(BtlHl7Export_t* pHl7Exp)
{

    SSL* ssl = pHl7Exp->ssl;
    void* buf = pHl7Exp->outputHl7Buf;
    int len = pHl7Exp->outputHl7Len;

    if (!ssl) {return BTLHL7EXP_SSL_SEND_FATAL; }

    int nBytesSent = SSL_write(ssl, buf, len);

    if (nBytesSent == len) {
        return BTLHL7EXP_SSL_SEND_OK;  /* all accepted */
    }

    if (nBytesSent > 0 && nBytesSent < len) {
        // Shouldn't happen without SSL_MODE_ENABLE_PARTIAL_WRITE; treat as retry 
        return BTLHL7EXP_SSL_SEND_RETRY;
    }

    // nBytesSent <= 0: determine if fatal error or retry required
    int sslError = SSL_get_error(ssl, nBytesSent);

    switch (sslError) {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        // TLS needs the opposite direction to progress; just retry later. 
        return BTLHL7EXP_SSL_SEND_RETRY;

    case SSL_ERROR_ZERO_RETURN:
        //Peer sent close_notify: clean shutdown 
        BTLHL7EXP_DBUG0("BTLHL7EXP: SSL peer closed connection!\n");
        return BTLHL7EXP_SSL_SEND_PEER_CLOSED;

    case SSL_ERROR_SYSCALL:
        if (isWouldBlock()) {
            return BTLHL7EXP_SSL_SEND_RETRY;
        }
        // nBytesSent == 0 with SSL_ERROR_SYSCALL can indicate abrupt EOF
        BTLHL7EXP_ERR("BTLHL7EXP ERR: SSL_write failed with error %d\n", sslError);
        return BTLHL7EXP_SSL_SEND_FATAL;

    case SSL_ERROR_SSL:
    default:
        BTLHL7EXP_ERR("BTLHL7EXP ERR: SSL_write failed with error %d\n", sslError);
        return BTLHL7EXP_SSL_SEND_FATAL;
    }
}  //btlHl7ExpSslSendOrRetry()


//btlHl7ExpRxSslData():
//returns 0 if  rx failed (socket error/ disconnect)
//returns > 0 on succesful Rx (returns number of bytes received)
//returns -1 if receive should be tried later
int btlHl7ExpRxSslData(BtlHl7Export_t* pHl7Exp, char* buf, int buflen)
{
    int nBytesRx = SSL_read(pHl7Exp->ssl, buf, buflen);
    if (nBytesRx > 0) {
        // got n bytes
        return nBytesRx;
    }
    else {
        int errSslRx = SSL_get_error(pHl7Exp->ssl, nBytesRx);
        switch (errSslRx) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // retry later (timer or readiness); you may change the buffer on retries
            return -1;
        case SSL_ERROR_ZERO_RETURN:
            // clean TLS close: peer sent close_notify
            // -> stop reading
            break;
        case SSL_ERROR_SYSCALL:
            // if errno/WSAGetLastError == 0 => unclean EOF; else OS I/O error
            break;
        default:
            // SSL_ERROR_SSL (protocol error) or something else => fatal
            break;
        }  //switch()
    }
    return 0;
}  //btlHl7ExpRxSslData

void btlHl7ExpClientShutdownTcpAndSsl(BtlHl7Export_t* pHl7Exp)
{
    if (pHl7Exp->sslEnable) {
        //SSL mode - so destroy the SSL instance
        if (pHl7Exp->ssl) {
            BTLHL7EXP_DBUG1("BTLHL7EXP: SSL: shutting down..(%d)\n", gDebugMarker1);
            SSL_shutdown(pHl7Exp->ssl);
            Sleep(20);
            SSL_free(pHl7Exp->ssl);    //this will close the socket
            pHl7Exp->client_socket = 0;
            SSL_CTX_free(pHl7Exp->sslCtx);
            pHl7Exp->ssl = 0;
            pHl7Exp->sslCtx = 0;
        }
        else {
            BTLHL7EXP_DBUG1("BTLHL7EXP: Closing sockets..(SSL not active)!\n");
            _btlHl7ExpCloseClientSocket(pHl7Exp);
        }
    }
    else {
        //TCP node, no SSL
        BTLHL7EXP_DBUG1("BTLHL7EXP: Closing sockets..\n");
        _btlHl7ExpCloseClientSocket(pHl7Exp);
    }

    return;
}  //btlHl7ExpClientShutdownTcpAndSsl

#endif //#ifdef BTLHL7EXP_SSL_EN
