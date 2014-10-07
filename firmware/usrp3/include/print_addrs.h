// Copyright 2013 Ettus Research LLC

#ifndef INCLUDED_PRINT_ADDRS_H
#define INCLUDED_PRINT_ADDRS_H

char *mac_addr_to_str_r(const void *addr, char *str);
char *ip_addr_to_str_r(const void *addr, char *str);

char *mac_addr_to_str(const void *addr);
char *ip_addr_to_str(const void *addr);

//void print_mac_addr(const void *addr);
//void print_ip_addr(const void *addr);

#endif /* INCLUDED_PRINT_ADDRS_H */
