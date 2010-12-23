#ifndef INCLUDED_LWIPPOOLS_H
#define INCLUDED_LWIPPOOLS_H

/*
 * from comment at top of mem.c:
 *
 * To let mem_malloc() use pools (prevents fragmentation and is much faster than
 * a heap but might waste some memory), define MEM_USE_POOLS to 1, define
 * MEM_USE_CUSTOM_POOLS to 1 and create a file "lwippools.h" that includes a list
 * of pools like this (more pools can be added between _START and _END):
 *
 * Define three pools with sizes 256, 512, and 1512 bytes
 * LWIP_MALLOC_MEMPOOL_START
 * LWIP_MALLOC_MEMPOOL(20, 256)
 * LWIP_MALLOC_MEMPOOL(10, 512)
 * LWIP_MALLOC_MEMPOOL(5, 1512)
 * LWIP_MALLOC_MEMPOOL_END
 */

LWIP_MALLOC_MEMPOOL_START
LWIP_MALLOC_MEMPOOL(2, 256)
LWIP_MALLOC_MEMPOOL_END

#endif /* INCLUDED_LWIPPOOLS_H */
