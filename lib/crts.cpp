#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <jansson.h>

#include "crts/debug.h"
#include "crts/crts.hpp"

// The small crappy TCP/IP socket wrapper that sends a string and reads
// input as JSON using libjansson.
//

CRTSTcpClient::CRTSTcpClient(const char *address, unsigned short port):
    fd(-1)
{
    errno = 0;

    // Initialize read buffers and pointers.
    //
    current = end = buf;
    *buf = '\0';


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

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        ERROR("socket() failed");
        throw "socket() failed";
    }

    if(connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        close(fd);
        fd = -1;
        ERROR("connect() to %s:%hu failed", address, port);
        throw "connect() failed";
    }

    DSPEW("connected to %s:%hu", address, port);
}


CRTSTcpClient::~CRTSTcpClient(void)
{
    if(fd >= 0)
    {
        close(fd);
        fd = -1;
    }

    DSPEW();
}


// return false on success or true on failure.
//
static inline bool writen(int fd, const char *buf, size_t len)
{
    while(len)
    {
        ssize_t n = write(fd, buf, len);
        if(n < 1)
        {
            if(n == 0)
                ERROR("server closed connection");
            else
                ERROR("write to server failed");
            return true; // fail
        }
        buf += n;
        len -= n;
    }

    return false; // success
}



// We note that we do not send the '\0' to a node server,
// it tends to choke on it.
//
bool CRTSTcpClient::send(const char *buf) const
{
    if(writen(fd, buf, strlen(buf)))
    {
        throw "failed to write() to TCP socket";
        return true; // failure
    }

    return false; // success
}


json_t *CRTSTcpClient::receiveJson(std::atomic<bool> &isRunning)
{
    // Read until we get the next JSON at "}\004" which is } plus a ascii
    // code 4 (end of transmission, EOT); and than replace the EOT
    // with a '\0' so as to terminate the string.
    //
    // reference: https://json.org/
    //

    DASSERT(MAXREAD > (size_t)(uintptr_t)(end - buf), "");

    errno = 0;

    do
    {
        // This call will block.  So most of the time we sit here
        // waiting to read.
        //
        ssize_t n = read(fd, end, MAXREAD - (end - buf));

        if(n < 1)
        {
            if(n == 0)
                ERROR("read end of file for socket");
            else if(errno == EINTR && !isRunning)
            {
                // we got signaled to stop
                INFO("socket read was interupted");
                return 0;
            }
            else
                ERROR("socket read() failed");

            return 0; // failure.
        }

        // Go to the end of the string.
        end += n;
        // Terminate the end of the string.
        *end = '\0';

        while(*current && *current != '\004')
            ++current;

        if(*current)
        {
            DASSERT(*(current-1) == '}', "");

            // Terminate the string.
            *current = '\0';

            json_t *root;
            json_error_t error;

            root = json_loads(buf, 0, &error);

            if(end > current)
                // Copy any extra data from the end of the buffer to
                // the front of the buffer.
                memmove(buf, current, end - current);

            // reset pointers.
            end = buf + (end - current);


            if(!root)
            {
                ERROR("json error on line %d: %s\n", error.line, error.text);
                throw "bad JSON read from socket";
            }

            // We have an end of a JSON expression.
            return root;
        }

        if(end >= buf + MAXREAD)
        {
            WARN("Failed to read good JSON in %zu bytes", MAXREAD);

            // Reinitialize read buffers and pointers.
            //
            current = end = buf;
            *buf = '\0';
        }

    } while(isRunning);


    return 0;
}
