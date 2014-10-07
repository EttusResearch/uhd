// Copyright 2013 Ettus Research LLC

#include <print_addrs.h>
#include <stddef.h>
#include <stdint.h>
#include <printf.h>

#define MAX_MAC_CHARS 24
#define MAX_IP_CHARS 16

static const char hex[16] = "0123456789ABCDEF";

char *mac_addr_to_str_r(const void *addr, char *str)
{
    uint8_t *p = (uint8_t *)addr;
    size_t j = 0;
    for(size_t i = 0; i < 6; i++)
    {
        if (i) str[j++] = ':';
        str[j++] = hex[(p[i] >> 4) & 0xf];
        str[j++] = hex[p[i] & 0xf];
    }
    str[j++] = '\0';
    return str;
}

char *ip_addr_to_str_r(const void *addr, char *str)
{
    uint8_t *p = (uint8_t *)addr;
    sprintf(str, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    return str;
}

char *mac_addr_to_str(const void *addr)
{
    static size_t index = 0;
    index = (index + 1) % 4;
    static char str[4][MAX_MAC_CHARS];
    return mac_addr_to_str_r(addr, str[index]);
}

char *ip_addr_to_str(const void *addr)
{
    static size_t index = 0;
    index = (index + 1) % 4;
    static char str[4][MAX_IP_CHARS];
    return ip_addr_to_str_r(addr, str[index]);
}

/*
void print_mac_addr(const void *addr)
{
    char str[MAX_MAC_CHARS];
    mac_addr_to_str_r(addr, str);
    printf("%s", str);
}

void print_ip_addr(const void *addr)
{
    char str[MAX_IP_CHARS];
    ip_addr_to_str_r(addr, str);
    printf("%s", str);
}
*/
