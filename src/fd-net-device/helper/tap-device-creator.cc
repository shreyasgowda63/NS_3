/*
 * Copyright (c) 2009 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "creator-utils.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <linux/if_tun.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define TAP_MAGIC 95549

//
// Lots of the following helper code taken from corresponding functions in src/node.
//
#define ASCII_DOT (0x2e)
#define ASCII_ZERO (0x30)
#define ASCII_COLON (0x3a)
#define ASCII_a (0x41)
#define ASCII_z (0x5a)
#define ASCII_A (0x61)
#define ASCII_Z (0x7a)

using namespace ns3;

/**
 * Struct holding IPv6 address data
 */
struct in6_ifreq
{
    struct in6_addr ifr6_addr; //!< IPv6 address
    uint32_t ifr6_prefixlen;   //!< IPv6 prefix length
    int32_t ifr6_ifindex;      //!< interface index
};

char
AsciiToLowCase(char c)
{
    if (c >= ASCII_a && c <= ASCII_z)
    {
        return c;
    }
    else if (c >= ASCII_A && c <= ASCII_Z)
    {
        return c + (ASCII_a - ASCII_A);
    }
    else
    {
        return c;
    }
}

static void
AsciiToMac48(const char* str, uint8_t addr[6])
{
    int i = 0;
    while (*str != 0 && i < 6)
    {
        uint8_t byte = 0;
        while (*str != ASCII_COLON && *str != 0)
        {
            byte <<= 4;
            char low = AsciiToLowCase(*str);
            if (low >= ASCII_a)
            {
                byte |= low - ASCII_a + 10;
            }
            else
            {
                byte |= low - ASCII_ZERO;
            }
            str++;
        }
        addr[i] = byte;
        i++;
        if (*str == 0)
        {
            break;
        }
        str++;
    }
}

void
SetIpv4(const char* deviceName, const char* ip, const char* netmask)
{
    struct ifreq ifr = {};
    struct sockaddr_in* sin;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Set the IP address of the new interface/device.
    strncpy(ifr.ifr_name, deviceName, IFNAMSIZ - 1);

    sin = (struct sockaddr_in*)&ifr.ifr_addr;
    inet_pton(AF_INET, ip, &sin->sin_addr);
    ifr.ifr_addr.sa_family = AF_INET;

    ABORT_IF(ioctl(sock, SIOCSIFADDR, &ifr) == -1, "Could not set IP address", true);
    LOG("Set device IP address to " << ip);

    // Set the net mask of the new interface/device
    ifr = {};
    strncpy(ifr.ifr_name, deviceName, IFNAMSIZ - 1);

    sin = (struct sockaddr_in*)&ifr.ifr_netmask;
    inet_pton(AF_INET, netmask, &sin->sin_addr);
    ifr.ifr_addr.sa_family = AF_INET;

    ABORT_IF(ioctl(sock, SIOCSIFNETMASK, &ifr) == -1, "Could not set net mask", true);
    LOG("Set device Net Mask to " << netmask);

    close(sock);
}

void
SetIpv6(const char* deviceName, const char* ip, int netprefix)
{
    struct ifreq ifr = {};
    struct sockaddr_in6 sin = {};
    struct in6_ifreq ifr6 = {};

    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    strncpy(ifr.ifr_name, deviceName, IFNAMSIZ - 1);

    ABORT_IF(ioctl(sock, SIOGIFINDEX, &ifr) == -1, "Could not get interface index", true);

    LOG("Set device IP v6 address to " << ip);

    sin.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip, (void*)&sin.sin6_addr);

    memcpy((char*)&ifr6.ifr6_addr, (char*)&sin.sin6_addr, sizeof(struct in6_addr));

    ifr6.ifr6_ifindex = ifr.ifr_ifindex;
    ifr6.ifr6_prefixlen = netprefix;

    // Set the IP address of the new interface/device.
    ABORT_IF(ioctl(sock, SIOCSIFADDR, &ifr6) == -1, "Could not set IP v6 address", true);
    LOG("Set device IP v6 address to " << ip);

    close(sock);
}

void
SetMacAddress(int fd, const char* mac)
{
    struct ifreq ifr = {};

    ifr.ifr_hwaddr.sa_family = 1; // this is ARPHRD_ETHER from if_arp.h
    AsciiToMac48(mac, (uint8_t*)ifr.ifr_hwaddr.sa_data);
    ABORT_IF(ioctl(fd, SIOCSIFHWADDR, &ifr) == -1, "Could not set MAC address", true);
    LOG("Set device MAC address to " << mac);
}

void
SetUp(const char* deviceName)
{
    struct ifreq ifr = {};

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    strncpy(ifr.ifr_name, deviceName, IFNAMSIZ - 1);

    ABORT_IF(ioctl(sock, SIOCGIFFLAGS, &ifr) == -1, "Could not get flags for interface", true);
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;

    ABORT_IF(ioctl(sock, SIOCSIFFLAGS, &ifr) == -1, "Could not bring interface up", true);
    LOG("Device is up");

    close(sock);
}

static int
CreateTap(const char* deviceName,
          const char* mac,
          bool ifftap,
          bool iffpi,
          const char* ip4,
          const char* netmask,
          const char* ip6,
          const int netprefix,
          const char* mode)
{
    //
    // Creation and management of Tap devices is done via the tun device
    //
    int fd = open("/dev/net/tun", O_RDWR);
    ABORT_IF(fd == -1, "Could not open /dev/net/tun", true);

    // Set flags for device type and PI header.
    struct ifreq ifr;
    ifr.ifr_flags = (ifftap ? IFF_TAP : IFF_TUN);
    if (!iffpi)
    {
        ifr.ifr_flags |= IFF_NO_PI;
    }

    // If we provide a null name to the ifr.ifr_name, we tell the kernel to pick
    // a name for us (i.e., tapn where n = 0..255.
    //
    // If the device does not already exist, the system will create one.
    strncpy(ifr.ifr_name, deviceName, IFNAMSIZ - 1);
    int status = ioctl(fd, TUNSETIFF, (void*)&ifr);
    ABORT_IF(status == -1, "Could not allocate tap device", true);

    std::string tapDeviceName = (char*)ifr.ifr_name;
    LOG("Allocated TAP device " << tapDeviceName);

    // Operating mode "2" corresponds to USE_LOCAL and "3" to USE_BRIDGE mode.
    // This means that we expect that the user will have named, created and
    // configured a network tap that we are just going to use.  So don't mess
    // up his hard work by changing anything, just return the tap fd.
    if (std::string(mode) == "2" || std::string(mode) == "3")
    {
        LOG("Returning precreated tap ");
        return fd;
    }

    // Set the hardware (MAC) address of the new device
    if (ifftap)
    {
        SetMacAddress(fd, mac);
    }

    // Set the IP address and netmask of the new interface/device.
    if (ip4)
    {
        SetIpv4(deviceName, ip4, netmask);
    }

    if (ip6)
    {
        SetIpv6(deviceName, ip6, netprefix);
    }

    // Bring the interface up.
    SetUp(deviceName);

    return fd;
}

int
main(int argc, char* argv[])
{
    int c;
    char* dev = (char*)"";
    char* ip4 = nullptr;
    char* ip6 = nullptr;
    char* mac = nullptr;
    char* netmask = nullptr;
    char* operatingMode = nullptr;
    char* path = nullptr;
    bool tap = false;
    bool pi = false;
    int prefix = -1;

    while ((c = getopt(argc, argv, "vd:i:I:m:n:P:o:thp:")) != -1)
    {
        switch (c)
        {
        case 'd':
            dev = optarg; // name of the new tap device
            break;
        case 'i':
            ip4 = optarg; // ip v4 address of the new device
            break;
        case 'I':
            ip6 = optarg; // ip v6 address of the new device
            break;
        case 'm':
            mac = optarg; // mac address of the new device
            break;
        case 'n':
            netmask = optarg; // ip v4 net mask for the new device
            break;
        case 'P':
            prefix = atoi(optarg); // ip v6 prefix for the new device
            break;
        case 'o':
            operatingMode = optarg; // operating mode of tap bridge
            break;
        case 't':
            tap = true; // mode for the device (TAP or TUN)
            break;
        case 'h':
            pi = true; // set the IFF_NO_PI flag
            break;
        case 'p':
            path = optarg; // path back to the tap bridge
            break;
        case 'v':
            gVerbose = true;
            break;
        }
    }

    //
    // We have got to be able to coordinate the name of the tap device we are
    // going to create and or open with the device that an external Linux host
    // will use.  If this name is provided we use it.  If not we let the system
    // create the device for us.  This name is given in dev
    //
    LOG("Provided Device Name is \"" << dev << "\"");

    //
    // We have got to be able to assign an IP address to the tap device we are
    // allocating.  This address is allocated in the simulation and assigned to
    // the tap bridge.  This address is given in ip.
    //
    ABORT_IF(ip4 == nullptr && ip6 == nullptr, "IP Address is a required argument", 0);
    if (ip4)
    {
        ABORT_IF(netmask == nullptr, "Net mask is a required argument", 0);
        LOG("Provided IP v4 Address is \"" << ip4 << "\"");
        LOG("Provided IP v4 Net Mask is \"" << netmask << "\"");
    }
    if (ip6)
    {
        ABORT_IF(prefix == -1, "Prefix is a required argument", 0);
        LOG("Provided IP v6 Address is \"" << ip6 << "\"");
        LOG("Provided IP v6 Prefix is \"" << prefix << "\"");
    }

    //
    // We have got to be able to assign a Mac address to the tap device we are
    // allocating.  This address is allocated in the simulation and assigned to
    // the bridged device.  This allows packets addressed to the bridged device
    // to appear in the Linux host as if they were received there.
    //
    ABORT_IF(mac == nullptr, "MAC Address is a required argument", 0);
    LOG("Provided MAC Address is \"" << mac << "\"");

    // We have got to know whether or not to create the TAP.
    ABORT_IF(operatingMode == nullptr, "Operating Mode is a required argument", 0);
    LOG("Provided Operating Mode is \"" << operatingMode << "\"");

    if (tap)
    {
        LOG("Provided device Mode is TAP");
    }
    else
    {
        LOG("Provided device Mode is TUN");
    }

    // IFF_NO_PI flag.
    if (pi)
    {
        LOG("IFF_NO_PI flag set. Packet Information will be present in the traffic");
    }

    // This program is spawned by a tap bridge running in a simulation.  It
    // wants to create a socket as described below.  We are going to do the
    // work here since we're running suid root.  Once we create the socket,
    // we have to send it back to the tap bridge.  We do that over a Unix
    // (local interprocess) socket.  The tap bridge created a socket to
    // listen for our response on, and it is expected to have encoded the address
    // information as a string and to have passed that string as an argument to
    // us.  We see it here as the "path" string.  We can't do anything useful
    // unless we have that string.
    ABORT_IF(path == nullptr, "path is a required argument", 0);
    LOG("Provided path is \"" << path << "\"");

    // The whole reason for all of the hoops we went through to call out to this
    // program will pay off here.  We created this program to run as suid root
    // in order to keep the main simulation program from having to be run with
    // root privileges.  We need root privileges to be able to futz with the
    // Tap device underlying all of this.  So all of these hoops are to allow
    // us to execute the following code:
    LOG("Creating Tap");
    int sock = CreateTap(dev, mac, tap, pi, ip4, netmask, ip6, prefix, operatingMode);
    ABORT_IF(sock == -1, "main(): Unable to create tap socket", 1);

    //
    // Send the socket back to the tap net device so it can go about its business
    //
    SendSocket(path, sock, TAP_MAGIC);

    return 0;
}
