//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/transport/if_addrs.hpp>

uhd::transport::if_addrs_t::if_addrs_t(void){
    /* NOP */
}

/***********************************************************************
 * Interface address discovery through ifaddrs api
 **********************************************************************/
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#include <boost/asio/ip/address_v4.hpp>

static boost::asio::ip::address_v4 sockaddr_to_ip_addr(sockaddr *addr){
    if (addr->sa_family == AF_INET) return boost::asio::ip::address_v4(ntohl(
        reinterpret_cast<sockaddr_in*>(addr)->sin_addr.s_addr
    ));
    return boost::asio::ip::address_v4::any();
}

static bool ifaddrs_valid(const struct ifaddrs *ifaddrs){
    return (
        ifaddrs->ifa_addr->sa_family == AF_INET and
        sockaddr_to_ip_addr(ifaddrs->ifa_addr) != boost::asio::ip::address_v4::loopback()
    );
}

std::vector<uhd::transport::if_addrs_t> uhd::transport::get_if_addrs(void){
    std::vector<if_addrs_t> if_addrs;
    struct ifaddrs *ifap;
    if (getifaddrs(&ifap) == 0){
        for (struct ifaddrs *iter = ifap; iter != NULL; iter = iter->ifa_next){
            if (not ifaddrs_valid(iter)) continue;
            if_addrs_t if_addr;
            if_addr.inet = sockaddr_to_ip_addr(iter->ifa_addr).to_string();
            if_addr.mask = sockaddr_to_ip_addr(iter->ifa_netmask).to_string();
            if_addr.bcast = sockaddr_to_ip_addr(iter->ifa_broadaddr).to_string();
            if_addrs.push_back(if_addr);
        }
        freeifaddrs(ifap);
    }
    return if_addrs;
}

/***********************************************************************
 * Interface address discovery through windows api (TODO)
 **********************************************************************/
//#elif HAVE_XXX_H

/***********************************************************************
 * Interface address discovery not included
 **********************************************************************/
#else /* HAVE_IFADDRS_H */

std::vector<uhd::transport::if_addrs_t> uhd::transport::get_if_addrs(void){
    return std::vector<if_addrs_t>();
}

#endif /* HAVE_IFADDRS_H */

////////////////////////////////////////////////////////////////////////
// How to extract the ip address: unix/windows
// http://www.developerweb.net/forum/showthread.php?t=5085
////////////////////////////////////////////////////////////////////////

/*
#include <stdio.h>

#ifdef WIN32
# include <windows.h>
# include <winsock.h>
# include <iphlpapi.h>
#else
# include <unistd.h>
# include <stdlib.h>
# include <sys/socket.h>
# include <netdb.h>
# include <netinet/in.h>
# include <net/if.h>
# include <sys/ioctl.h>
#endif

#include <string.h>
#include <sys/stat.h>

typedef unsigned long uint32;

#if defined(__FreeBSD__) || defined(BSD) || defined(__APPLE__) || defined(__linux__)
# define USE_GETIFADDRS 1
# include <ifaddrs.h>
static uint32 SockAddrToUint32(struct sockaddr * a)
{
   return ((a)&&(a->sa_family == AF_INET)) ? ntohl(((struct sockaddr_in *)a)->sin_addr.s_addr) : 0;
}
#endif

// convert a numeric IP address into its string representation
static void Inet_NtoA(uint32 addr, char * ipbuf)
{
   sprintf(ipbuf, "%li.%li.%li.%li", (addr>>24)&0xFF, (addr>>16)&0xFF, (addr>>8)&0xFF, (addr>>0)&0xFF);
}

// convert a string represenation of an IP address into its numeric equivalent
static uint32 Inet_AtoN(const char * buf)
{
   // net_server inexplicably doesn't have this function; so I'll just fake it
   uint32 ret = 0;
   int shift = 24;  // fill out the MSB first
   bool startQuad = true;
   while((shift >= 0)&&(*buf))
   {
      if (startQuad)
      {
         unsigned char quad = (unsigned char) atoi(buf);
         ret |= (((uint32)quad) << shift);
         shift -= 8;
      }
      startQuad = (*buf == '.');
      buf++;
   }
   return ret;
}

static void PrintNetworkInterfaceInfos()
{
#if defined(USE_GETIFADDRS)
   // BSD-style implementation
   struct ifaddrs * ifap;
   if (getifaddrs(&ifap) == 0)
   {
      struct ifaddrs * p = ifap;
      while(p)
      {
         uint32 ifaAddr  = SockAddrToUint32(p->ifa_addr);
         uint32 maskAddr = SockAddrToUint32(p->ifa_netmask);
         uint32 dstAddr  = SockAddrToUint32(p->ifa_dstaddr);
         if (ifaAddr > 0)
         {
            char ifaAddrStr[32];  Inet_NtoA(ifaAddr,  ifaAddrStr);
            char maskAddrStr[32]; Inet_NtoA(maskAddr, maskAddrStr);
            char dstAddrStr[32];  Inet_NtoA(dstAddr,  dstAddrStr);
            printf("  Found interface:  name=[%s] desc=[%s] address=[%s] netmask=[%s] broadcastAddr=[%s]\n", p->ifa_name, "unavailable", ifaAddrStr, maskAddrStr, dstAddrStr);
         }
         p = p->ifa_next;
      }
      freeifaddrs(ifap);
   }
#elif defined(WIN32)
   // Windows XP style implementation

   // Adapted from example code at http://msdn2.microsoft.com/en-us/library/aa365917.aspx
   // Now get Windows' IPv4 addresses table.  Once again, we gotta call GetIpAddrTable()
   // multiple times in order to deal with potential race conditions properly.
   MIB_IPADDRTABLE * ipTable = NULL;
   {
      ULONG bufLen = 0;
      for (int i=0; i<5; i++)
      {
         DWORD ipRet = GetIpAddrTable(ipTable, &bufLen, false);
         if (ipRet == ERROR_INSUFFICIENT_BUFFER)
         {
            free(ipTable);  // in case we had previously allocated it
            ipTable = (MIB_IPADDRTABLE *) malloc(bufLen);
         }
         else if (ipRet == NO_ERROR) break;
         else
         {
            free(ipTable);
            ipTable = NULL;
            break;
         }
     }
   }

   if (ipTable)
   {
      // Try to get the Adapters-info table, so we can given useful names to the IP
      // addresses we are returning.  Gotta call GetAdaptersInfo() up to 5 times to handle
      // the potential race condition between the size-query call and the get-data call.
      // I love a well-designed API :^P
      IP_ADAPTER_INFO * pAdapterInfo = NULL;
      {
         ULONG bufLen = 0;
         for (int i=0; i<5; i++)
         {
            DWORD apRet = GetAdaptersInfo(pAdapterInfo, &bufLen);
            if (apRet == ERROR_BUFFER_OVERFLOW)
            {
               free(pAdapterInfo);  // in case we had previously allocated it
               pAdapterInfo = (IP_ADAPTER_INFO *) malloc(bufLen);
            }
            else if (apRet == ERROR_SUCCESS) break;
            else
            {
               free(pAdapterInfo);
               pAdapterInfo = NULL;
               break;
            }
         }
      }

      for (DWORD i=0; i<ipTable->dwNumEntries; i++)
      {
         const MIB_IPADDRROW & row = ipTable->table[i];

         // Now lookup the appropriate adaptor-name in the pAdaptorInfos, if we can find it
         const char * name = NULL;
         const char * desc = NULL;
         if (pAdapterInfo)
         {
            IP_ADAPTER_INFO * next = pAdapterInfo;
            while((next)&&(name==NULL))
            {
               IP_ADDR_STRING * ipAddr = &next->IpAddressList;
               while(ipAddr)
               {
                  if (Inet_AtoN(ipAddr->IpAddress.String) == ntohl(row.dwAddr))
                  {
                     name = next->AdapterName;
                     desc = next->Description;
                     break;
                  }
                  ipAddr = ipAddr->Next;
               }
               next = next->Next;
            }
         }
         char buf[128];
         if (name == NULL)
         {
            sprintf(buf, "unnamed-%i", i);
            name = buf;
         }

         uint32 ipAddr  = ntohl(row.dwAddr);
         uint32 netmask = ntohl(row.dwMask);
         uint32 baddr   = ipAddr & netmask;
         if (row.dwBCastAddr) baddr |= ~netmask;

         char ifaAddrStr[32];  Inet_NtoA(ipAddr,  ifaAddrStr);
         char maskAddrStr[32]; Inet_NtoA(netmask, maskAddrStr);
         char dstAddrStr[32];  Inet_NtoA(baddr,   dstAddrStr);
         printf("  Found interface:  name=[%s] desc=[%s] address=[%s] netmask=[%s] broadcastAddr=[%s]\n", name, desc?desc:"unavailable", ifaAddrStr, maskAddrStr, dstAddrStr);
      }

      free(pAdapterInfo);
      free(ipTable);
   }
#else
   // Dunno what we're running on here!
#  error "Don't know how to implement PrintNetworkInterfaceInfos() on this OS!"
#endif
}

int main(int, char **)
{
   PrintNetworkInterfaceInfos();
   return 0;
}
*/

////////////////////////////////////////////////////////////////////////
// How to extract the mac address: linux/windows
// http://old.nabble.com/MAC-Address-td19111197.html
////////////////////////////////////////////////////////////////////////

/*
Linux:

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <ifaddrs.h>

ifaddrs * ifap = 0;
if(getifaddrs(&ifap) == 0)
{
     ifaddrs * iter = ifap;
     while(iter)
     {
         sockaddr_ll * sal =
                    reinterpret_cast<sockaddr_ll*>(iter->ifa_addr);
        if(sal->sll_family == AF_PACKET)
        {
                 // get the mac bytes
                // copy(sal->sll_addr,
                 //      sal->sll_addr+sal->sll_hallen,
                 //      buffer);
        }
         iter = iter->ifa_next;
     }
     freeifaddrs(ifap);
}

Windows:
#include <winsock2.h>
#include <iphlpapi.h>

std::vector<boost::uint8_t> buf;
DWORD bufLen = 0;
GetAdaptersAddresses(0, 0, 0, 0, &bufLen);
if(bufLen)
{
     buf.resize(bufLen, 0);
     IP_ADAPTER_ADDRESSES * ptr =
                     reinterpret_cast<IP_ADAPTER_ADDRESSES*>(&buf[0]);
     DWORD err = GetAdaptersAddresses(0, 0, 0, ptr, &bufLen);
     if(err == NO_ERROR)
     {
          while(ptr)
         {
              if(ptr->PhysicalAddressLength)
              {
                 // get the mac bytes
                // copy(ptr->PhysicalAddress,
                 //      ptr->PhysicalAddress+ptr->PhysicalAddressLength,
                 //      buffer);
              }
              ptr = ptr->Next;
          }
     }
} 
*/
