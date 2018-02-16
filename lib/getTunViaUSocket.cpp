#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

#include "crts/debug.h"
#include "getTunViaUSocket.hpp"


// Return true if we have data ready to read.
static int checkHaveRead(int fd, long sec, long usec)
{
    fd_set rfds;
    struct timeval tv = { .tv_sec = sec, .tv_usec = usec };
    int ret;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    ret = select(fd+1, &rfds, 0, 0, &tv);
    if(ret > 0)
        return ret;

    if(ret == -1)
        ERROR("select(, fd=%d,,) failed", fd);
    else
        WARN("read timed out in %lu %sseconds",
                sec?sec:usec, sec?"":"micro ");
    return ret;
}


//https://stackoverflow.com/questions/28003921/sending-file-descriptor-by-linux-socket/

// Read the TUN file descriptor from the socket and then close the socket.
// Will close the socket and set it to -1 before it returns.
// Returns -1 on error.
static inline int recvfd(int &socket)  // receive fd from socket
{
    struct msghdr msg = {0};

    char m_buffer[256];
    struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    char c_buffer[256];
    msg.msg_control = c_buffer;
    msg.msg_controllen = sizeof(c_buffer);

    errno = 0;

    INFO("Calling recvmsg(fd=%d,,,)", socket);

    if(recvmsg(socket, &msg, 0) < 0)
    {
        close(socket);
        socket = -1;
        ERROR("Failed to receive file descriptor message");
        return -1;
    }

    struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);

    unsigned char * data = CMSG_DATA(cmsg);

    int fd = *((int*) data);

    INFO("Got from UNIX Socket TUN fd=%d", fd);

    close(socket);

#if 1
    // We do not need this socket any more so we'll close it
    // and move the TUN fd to it's value.
    if((fd = dup2(fd, socket)) != socket)
    {
        ERROR("dup2() failed");
        close(fd);
        socket = -1;
        return -1;
    }
#endif

    socket = -1;

    INFO("Got TUN duped to fd=%d", fd);

    return fd;
}

// We assume that the program crts_mkTUN is in the same directory as the
// program that we are executing is in.
//
// This returns a malloc()'ed string that is the full path to crts_mkTUN
//
static inline char *getPathTo_CRTS_mkTUN(void)
{
    const char *mkTUN = "crts_mkTUN";
    const size_t addSuffixLen = strlen(mkTUN) + 1; // plus '\0'
    const size_t inc = 128;
    size_t bufLen = 128;

    char *buf = (char *) malloc(bufLen);

    ASSERT(buf, "malloc() failed");
    ssize_t rl = readlink("/proc/self/exe", buf, bufLen);
    ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");

    while( ((size_t) rl) + addSuffixLen >= bufLen)
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
    ++rl;
    strcpy(&buf[rl], mkTUN);

    return buf;

    // for example
    //
    //     buf = "/home/lance/installed/bin/" + "crts_mkTUN"
    //     
    //     buf = "/home/lance/git/crts/bin/" + "crts_mkTUN"
    //
    //     buf = "/usr/local/encap/crts/bin/" + "crts_mkTUN"
    //
}


// We wish to restrict this string to only certain values that we feel
// will not interfere with the other networks that are setup.
//
// TODO: add a network policy configuration that changes this function.
//
// Returns true on error.
bool checkSubnetAddress(const char *subnet/* like "10.0.0.1/24" */)
{
    // The string must be of the form *.*.*.*/nn   24  >= nn >= 31
    //
    // If nn is not in that range it may interfere with most systems
    // networking.

    unsigned long val;
    size_t len = strlen(subnet);
    const char *s = subnet;

    if(len > 18 || len < 10) goto fail;
    if(subnet[len-3] != '/') goto fail;

    errno = 0;
    val = strtoul(&subnet[len-2], 0, 10);
    if(errno || val > 31 || val < 24) goto fail;

    // We should have  m.n.o.p/qr  where m,n,o,p,q,r are numbers

    for(int i=0; i<3; ++i)
    {
        val = strtoul(s, 0, 10); // 1,2,3
        if(errno || val > 255) goto fail;

        // go to next 0 to 255 (byte) value.
        while(*s && *s != '.') ++s;
        if(! *s) goto fail;
        ++s;
    }

    val = strtoul(s, 0, 10); // 4
    if(errno || val > 255) goto fail;

    return false;

    fail:

        ERROR("\"%s\" is not a safe subnet address\n"
            "Try something like: \"10.0.0.0/24\"", subnet);
        return true;
}




// Returns file descriptor to the TUN device that has the given IP4 subnet

int getTunViaUSocket(const char *subnet/* like "10.0.0.1/24" */)
{

    if(checkSubnetAddress(subnet))
        return -1; // fail

    INFO("Making TUN with subnet \"%s\"", subnet);

    int sv[2] = { -1, -1 };
    int tunFd = -1;
    pid_t pid = -1;
    int status = 0;


    int ret;
    
    errno = 0;
    ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
    if(ret)
    {
        ERROR("socketpair(AF_UNIX, SOCK_DGRAM, 0,) failed");
        goto fail;
    }
    DASSERT(sv[0] > -1 && sv[1] > -1, "");

    pid = fork();
    if(pid < 0)
    {
        ERROR("fork() failed");
        goto fail;
    }

    if(pid == 0)
    {
        // I am the child.
        close(sv[0]); // the child will write to sv[1].
        errno = 0;
        if(1 != dup2(sv[1], 1))
        {
            ERROR("dup2(fd=%d, 0) failed", sv[1]);
            exit(1);
        }
        char *mkTUN = getPathTo_CRTS_mkTUN();

        // now stdin and stdout are the UNIX domain socketpair
        execl(mkTUN, mkTUN, subnet, 0);

        ERROR("exec(\"%s\", \"%s\", 0) failed", mkTUN, subnet);

        // We do not need to free mkTUN we are exiting.
        exit(1);
    }

    // I am the parent


    close(sv[1]); // The parent will read from sv[0].
    sv[1] = -1;

    // By calling wait() before reading the UNIX domain socket we
    // are end up returning much sooner for the failing causes,
    // and the successful case is not noticeable slower.

    // I am the parent.
    ret = waitpid(pid, &status, 0);
    if(ret == -1)
    {
        ERROR("waitpid(pid=%u,,) failed", pid);
        goto fail;
    }

    INFO("wait(pid=%u,,) returned status=%d", pid, status);

    if((ret = checkHaveRead(sv[0], 0, 100/*micro seconds*/)) > 0)
        tunFd = recvfd(sv[0]);

    return tunFd;

fail:

    if(pid != -1) kill(pid, SIGTERM);
    if(sv[0] > -1) close(sv[0]);
    if(sv[1] > -1) close(sv[0]);

    return -1; // fail
}
