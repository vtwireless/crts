#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <linux/if_tun.h>


#include "crts/debug.h"
#include "tun_alloc.hpp"


#define CLONE_DEV  "/dev/net/tun"


static bool Run(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

// Run the printf composed command via system(3).
// Returns false on success.
static bool Run(const char *fmt, ...)
{
    // If there is a command longer than this than something has gone
    // wrong.
    const int BUFLEN = 512;
    char cmd[BUFLEN];
    va_list ap;
    va_start(ap, fmt);
    int l = vsnprintf(cmd, BUFLEN, fmt, ap);
    va_end(ap);
    if(l >= BUFLEN)
    {
        ERROR("The command: \"%s\" is too long", cmd);
        return true; // FAIL
    }

    INFO("Running: \"%s\"", cmd);

    errno = 0;

    l = system(cmd);

    if(l)
    {
        ERROR("Running: \"%s\" FAILED", cmd);
        return true; // fail
    }
    return false; // success
}

#if 0
// Example;
//
//    TUNExists("tun10.0.0.0_24");
//
// return true if it exists
//
static bool TUNExists(const char *tunName)
{
    const ssize_t LEN = 254;
    char buf[LEN];
    ASSERT(LEN >
        snprintf(buf, LEN, "/sys/class/net/%s", tunName), "");
    INFO("Checking for TUN \"%s\"", buf);
    bool exists = (access(buf, R_OK) == 0);
    if(exists)
        INFO("TUN \"%s\" exists", buf);
    return exists;
}
#endif


// Build a TUN device name that is a function of the ipSubNet
//
// So for ipSubNet = "10.2.0.1/24"
//
//   10.2.1
//
//  int inet_aton(const char *cp, struct in_addr *inp);
//
//
//
//   We get a tunName of  "tun01000200000124"  17 char with is the
//   max tun name length   tun 010 002 000 001 24
//
//   and ipSubNet = "10.2.0.1/9"   -> tunName = tun01000200000109
//
// So the TUN name is a 1 to 1 mapping to and from the ipSubnet.

//TODO:  add tun names that the number equal to the subnet address
//       so we can not interfere with old existing tun sub-networks
//       just by comparing numbers in the tun names.


// http://backreference.org/2010/03/26/tuntap-interface-tutorial/
//
// Many thanks to Davide Brini
//
// Returns an open file descriptor to the TUN device.
int tun_alloc(const char *ipSubNet/*like "10.0.0.1/24"*/, bool setOwner)
{
    struct ifreq ifr;
    int fd = -1;
    int err;
    char *tunName;

    uid_t eu = geteuid(); // may be root if set uid bit was set
    uid_t u = getuid();  // who is running this

    /* open the clone device */
    errno = 0;
    fd = open(CLONE_DEV, O_RDWR);
    if(fd < 0)
    {
        ERROR("open(\"" CLONE_DEV "\", O_RDWR) failed");
        goto fail;
    }

    memset(&ifr, 0, sizeof(ifr));

    /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */
   
    /* We must set IFF_NO_PI so we do not have to remove
     * some prefix data for each read().
     */
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    //strncpy(ifr.ifr_name, tunName, tunNameLen + 1);

    /* try to create the device */
    if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0)
    {
        ERROR("ioctl(fd=%d, TUNSETIFF,)=%d failed", fd, err);
        goto fail;
    }
    tunName = ifr.ifr_name;
    INFO("Created TUN named: \"%s\"", tunName);

#if 0 // Not having the TUN unless the program that creates it is running
    // can be a good thing.
    if(ioctl(fd, TUNSETPERSIST, 1) < 0)
    {
        ERROR("ioctl(fd=%d, TUNSETPERSIST, 1)=%d failed", fd, err);
        goto fail;
    }
#endif


    // Setting the owner and group should let the owner be able to
    // connect and read/write the TUN with this tunName.

    if(setOwner && eu != u)
    {
        // Man page says: These functions are always successful.
        if((err = ioctl(fd, TUNSETOWNER, u)) < 0)
        {
            ERROR("ioctl(fd=%d, TUNSETOWNER, %u)=%d failed", fd, u, err);
            goto fail;
        }
        else
            INFO("Set TUN \"%s\" owner id to %u", tunName, u);
    }
    
    if(setOwner && 0)
    {
        // Man page says: These functions are always successful.
        gid_t eg = getegid(); // root
        gid_t g = getgid();  // who is running this
        if(eg == g || (err = ioctl(fd, TUNSETGROUP, g)) < 0)
        {
            ERROR("ioctl(fd=%d, TUNSETGROUP, %u)=%d failed", fd, g, err);
            goto fail;
        }
        else
            INFO("Set TUN \"%s\" group owner id to %u", tunName, g);
    }


    if(Run("/sbin/ip link set dev %s up", tunName))
        goto fail;

#if 1
    if(Run("/sbin/ip addr add %s dev %s", ipSubNet, tunName))
        goto fail;
#endif


#if 0 // This looks like the simplest interface to route IP to the TUN device
    if(Run("/sbin/ifconfig %s %s netmask %s", tunName, ipAddress, netmask))
        goto fail;
#endif


    // To remove the TUN is it was set with TUNSETPERSIST
    //
    // RUN:    ip link delete tunN

#if 1

    // We should not need to run as root from here out,
    // so we change to the user that run this program.

    if(eu == 0 || u != eu)
    {
        if((err = setuid(u)))
        {
            ERROR("setuid(%u)=%d failed", u, err);
            goto fail;
        }
        INFO("Set user id to: %u", u);
    }
#endif

    /* this is the special file descriptor that the caller will use to
     * talk with the virtual interface */
    return fd;

fail:

  if(fd >= 0) close(fd);

  return -1; // fail
}
