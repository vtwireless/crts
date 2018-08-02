#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "crts/crts.hpp"
#include "crts/debug.h"

// The small crappy TCP/IP socket wrapper that sends a very limited form
// of JSON to and from the server that it connects to.  See crts.hpp.
//

CRTSTcpConnection::CRTSTcpConnection(const char *firstMessage,
        const char *address, int port)
{
    errno = 0;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        ERROR("socket() failed");
        throw "socket() failed";
    }

    struct sockaddr_in addr;
    memset(&addr, '0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(strcmp(address, "localhost") == 0)
        address = "127.0.0.1";
    if(inet_pton(AF_INET, address, &addr.sin_addr) <= 0)
    {
        close(fd);
        fd = -1;
        ERROR("inet_pton(AF_INET, \"%s\", ..) failed", address);
        throw "inet_pton(AF_INET,,) failed";
    }

    if(connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        close(fd);
        fd = -1;
        ERROR("connect() to %s:%hu failed", address, port);
        throw "connect() failed";
    }

    DSPEW("connected to %s:%hu", address, port);

    if(firstMessage && firstMessage[0] != '\0' && send(firstMessage))
        throw "Failed to send first message";
}

CRTSTcpConnection::~CRTSTcpConnection(void)
{
    DSPEW();
}

bool CRTSTcpConnection::load(const char *key, double value)
{

    return false; // success
}


bool CRTSTcpConnection::send(const char *buf)
{

    return false; // success
}

char * CRTSTcpConnection::receive(void)
{


    return 0; // TODO: MORE HERE
}
