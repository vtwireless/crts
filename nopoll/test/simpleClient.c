#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <nopoll.h>



static nopoll_bool ssl_check_handler(
        noPollCtx *ctx,
        noPollConn  *conn,
        noPollPtr SSL_CTX,
        noPollPtr ssl,
        noPollPtr user_data)
{
    // Do here some additional checks on the certificate received (using SSL_CTX and SSL). 
    // I the case of error, return nopoll_false to ensure the connection is not accepted. 
    return nopoll_true; // to accept connection 
}



int main(void)
{
    noPollConn *conn;
    noPollCtx *ctx;
    noPollConnOpts *opts;

    ctx = nopoll_ctx_new();
    nopoll_log_enable(ctx, 1/*spew on*/);
    nopoll_log_color_enable(ctx, 1/*spew on*/);
    opts = nopoll_conn_opts_new();

    if(!opts)
    {
        printf ("nopoll_conn_opts_new() failed\n");
        return 1;
    }

    nopoll_conn_opts_set_ssl_protocol(opts, NOPOLL_METHOD_TLSV1_2);

    nopoll_ctx_set_post_ssl_check(ctx, ssl_check_handler, NULL);

    /* create connection */
    conn = nopoll_conn_tls_new(ctx, opts, "localhost", "9190", NULL, NULL, NULL, NULL);
    if(!nopoll_conn_is_ok(conn)) {
	printf ("nopoll_conn_tls_new() failed\n");
	return 1;
    }

    if (! nopoll_conn_wait_until_connection_ready (conn, 10)) {
        printf ("nopoll_conn_wait_until_connection_ready() failed\n");
	return 1;
    }

    const size_t maxLen = 1024;
    char msg[maxLen];

    int i;
    for(i=0; i<10; ++i)
    {
        snprintf(msg, maxLen, "Hello SerVER %d", i);

        long len = strlen(msg);

        int ret = nopoll_conn_send_text (conn, msg, len);
        if(ret != len)
        {
	    printf ("nopoll_conn_send_text(,,len=%ld)=%d failed\n", len, ret);
	    return nopoll_false;
	}
    }

    printf("sleeping\n");
    sleep(6);
    printf("finished sleeping\n");


    nopoll_conn_close(conn);	
    nopoll_ctx_unref(ctx);

    return 0;
}

