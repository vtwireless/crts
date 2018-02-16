#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/if_tun.h>

#include "crts/debug.h"
#include "tun_alloc.hpp"
#include "getTunViaUSocket.hpp"


// We require that the parent process be from an executable from the same
// directory as this program binary is in.  This is better that not
// checking at all.  This will plug some obvious security holes.
//
// Returns true on failure.
static bool checkParentPath(void)
{
    const size_t inc = 128;
    size_t bufLen = 128;
    char *buf = (char *) malloc(bufLen);

    ASSERT(buf, "malloc() failed");
    ssize_t rl = readlink("/proc/self/exe", buf, bufLen);
    ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");

    while(((size_t) rl) >= bufLen)
    {
        // The path may be truncated, so make a larger buffer.
        DASSERT(bufLen < 1024*1024, ""); // it should not get this large.
        buf = (char *) realloc(buf, bufLen += inc);
        ASSERT(buf, "realloc() failed");
        rl = readlink("/proc/self/exe", buf, bufLen);
        ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    }

    buf[rl] = '\0';
    // Now buf = path to program binary
    //
    // Now strip off to after "/" (Linux path separator)
    // by backing up one '/' chars.
    --rl;
    while(rl > 3 && buf[rl] != '/') --rl; // one '/'
    ASSERT(buf[rl] == '/', "");
    buf[rl] = '\0';
    // now buf is the directory where crts_mkTUN is.

    // If parentPath ain't long enough we fail as we should.
    bufLen += inc;
    char *parentPath = (char *) malloc(bufLen);
    ASSERT(parentPath, "malloc() failed");

    char pproc[63];
    int sn = snprintf(pproc, 63, "/proc/%u/exe", getppid());
    ASSERT(sn > 0, "snprintf() failed");
    ssize_t prl = readlink(pproc, parentPath, bufLen);
    ASSERT(prl > 0, "readlink(\"%s\",,) failed", parentPath);

    char *s = &parentPath[rl - 1];

    if(prl < rl)
    {
        ERROR("Bad path \"%s\"", parentPath);
        goto fail;
    }

    while(*s && *s != '/') ++s;

    if(*s != '/')
    {
        ERROR("Bad path \"%s\"", parentPath);
        goto fail;
    }
    *s = '\0';

    if(strcmp(parentPath, buf))
    {
        ERROR("Bad parent path \"%s\" != \"%s\"", parentPath, buf);
        goto fail;
    }

    INFO("This program has a good path \"%s\"", buf);


    free(parentPath);
    free(buf);
 
    return false; // success

fail:
    free(parentPath);
    free(buf);

    return true; // fail
}


//https://stackoverflow.com/questions/28003921/sending-file-descriptor-by-linux-socket/

// Return true on failure
static bool sendfd(int socket, int fd)  // send fd by socket
{
    struct msghdr msg = { 0 };
    char buf[CMSG_SPACE(sizeof(fd))];
    memset(buf, 0, sizeof(buf));
    struct iovec io = { .iov_base = (void *) "ABC", .iov_len = 3 };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

    *((int *) CMSG_DATA(cmsg)) = fd;

    msg.msg_controllen = cmsg->cmsg_len;

    errno = 0;

    if(sendmsg(socket, &msg, 0) < 0)
    {
        ERROR("Failed to send file descriptor");
        return true;
    }
    return false;
}


static int usage(const char *argv0)
{
    fprintf(SPEW_FILE,
        "\n"
        "  Usage: %s IP4_SUBNET\n"
        "\n"
        "     Create a TUN device that is is connected to the IP4 subnet IP4_SUBNET.\n"
        "  Then write the file descriptor to stdout which should be a UNIX domain socket.\n"
        "  For example:\n"
        "\n"
        "            %s 10.0.0.0/24\n"
        "\n",
        argv0, argv0);
    fprintf(SPEW_FILE,
        "     This program is run to get a TUN file descriptor from a UNIX domain socket, where\n"
        "  file descriptor 1 is the UNIX domain socket.  If file descriptor 1 is a tty device and\n"
        "  therefore not a UNIX domain socket this will happen.  You can use the code in\n"
        "  getTunViaUSocket.cpp to run and connect to this program to get a TUN file descriptor.\n\n");
    return 1;
}



int main(int argc, const char **argv)
{
    if(argc != 2 || argv[1][0] == '-')
        // -h --help or not valid anyway
        return usage(argv[0]);

    if(isatty(1))
    {
        fprintf(SPEW_FILE, "\n File descriptor 1 is a tty device and "
            "therefore not a UNIX domain socket.\n\n");
        return usage(argv[0]);
    }

    if(checkParentPath())
        return usage(argv[0]);


    if(checkSubnetAddress(argv[1]))
        return usage(argv[0]);


    INFO("Running: %s %s", argv[0], argv[1]);

    int fd = tun_alloc(argv[1]);
    if(fd < 0)
        return 1; // FAIL


    if(sendfd(1, fd)) // send fd to 1 which is a UNIX domain socket.
        return 1; // error

    INFO("Ran: %s %s", argv[0], argv[1]);

    return 0; // success
}
