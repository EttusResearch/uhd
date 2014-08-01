//
// Copyright 2014 Ettus Research LLC
//

#ifndef INCLUDED_AD9361_DISPATCH_H
#define INCLUDED_AD9361_DISPATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ad9361_transaction.h>

extern void ad9361_dispatch(const char* request, char* response);

typedef void (*msgfn)(const char*, ...);

extern void ad9361_set_msgfn(msgfn pfn);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_AD9361_DISPATCH_H */
