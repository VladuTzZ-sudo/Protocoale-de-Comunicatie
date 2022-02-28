#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#include "stdio.h"
#include "string.h"

int main()
{
    BIO *bio;
    SSL *ssl;
    SSL_CTX *ctx;

    int p;
    FILE *f = fopen("pagina.html", "w");
    char *request = "GET / HTTP/1.0\r\nHost: www.verisign.com\r\n\r\n";
    char r[1024];

    /* initializare librarie */

    SSL_library_init();
    ERR_load_BIO_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    /* TO DO initializare context */
    ctx = SSL_CTX_new(SSLv23_client_method());

    /* incarca trust store */

    if (!SSL_CTX_load_verify_locations(ctx, "TrustStore.pem", NULL))
    {
        fprintf(stderr, "Error loading trust store\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return 0;
    }

    /* stabileste conexiune */

    bio = BIO_new_ssl_connect(ctx);

    /* Seteaza flag SSL_MODE_AUTO_RETRY  */

    BIO_get_ssl(bio, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    /* TO DO setup conexiune */
    bio = BIO_new_ssl_connect(ctx);
    BIO_set_conn_hostname(bio, "www.verisign.com:443");
    if (BIO_do_connect(bio) <= 0)
        perror("error connection");

    /* TO DO verifica certificat */

    /* Trimite request */

    BIO_write(bio, request, strlen(request));

    memset(r, 0, 1024);

    while (BIO_read(bio, r, sizeof(r)))
    {
        fwrite(r, sizeof(char), 1024, f);
        memset(r, 0, 1024);
        /* TO DO Citeste raspuns si pregateste  output*/
    }

    /* Inchide conexiune si elibereaza context */

    fclose(f);
    BIO_free_all(bio);
    SSL_CTX_free(ctx);
    return 0;
}
