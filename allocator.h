
/*
** Bundled memory allocator.
** based on LuaJITs allocator, HEAVILY modified to be mostly standalone.
**
** Beware: this is a HEAVILY CUSTOMIZED version of dlmalloc.
** The original bears the following remark:
**
**   This is a version (aka dlmalloc) of malloc/free/realloc written by
**   Doug Lea and released to the public domain, as explained at
**   http://creativecommons.org/licenses/publicdomain.
**
**   * Version pre-2.8.4 Wed Mar 29 19:46:29 2006    (dl at gee)
**
** No additional copyright is claimed over the customizations.
** Please do NOT bother the original author about this version here!
**
** If you want to use dlmalloc in another project, you should get
** the original from: ftp://gee.cs.oswego.edu/pub/misc/
** For thread-safe derivatives, take a look at:
** - ptmalloc: http://www.malloc.de/
** - nedmalloc: http://www.nedprod.com/programs/portable/nedmalloc/
*/

#ifndef _LJ_ALLOC_H
#define _LJ_ALLOC_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#if defined(__GLIBC__) || defined(__MUSL__)
    #include <malloc.h>
#endif

#if (defined(__linux__) || defined(__CYGWIN__)) && !defined(_GNU_SOURCE)
    #define _GNU_SOURCE
#endif

#if defined(__STRICT_ANSI__) || defined(__PCC__)
    #define MEMPOOL_INLINE static
#else
    #if defined(__GNUC__) || defined(__TINYC__)
        #define MEMPOOL_INLINE static __attribute__((always_inline)) inline
    #else
        #define MEMPOOL_INLINE static inline
    #endif
#endif

#if defined(__cplusplus)
    #define MEMPOOL_CPP_BEGINEXTERN() extern "C" {
    #define MEMPOOL_CPP_ENDEXTERN() }
#else
    #define MEMPOOL_CPP_BEGINEXTERN()
    #define MEMPOOL_CPP_ENDEXTERN()
#endif


/* Select native OS if no target OS defined. */
#if defined(MEMPOOL_CONFIG_FORCEGENERIC) && (MEMPOOL_CONFIG_FORCEGENERIC == 1)
    #define MEMPOOL_TARGET_GENERIC
#else
    #if defined(__linux__)
        #if defined(__SDCC)
            #define MEMPOOL_TARGET_GENERIC
        #else
            #define MEMPOOL_TARGET_LINUX
        #endif
    #elif defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER) || defined(__CYGWIN__)
        #define MEMPOOL_TARGET_WINDOWS
    #elif defined(__MACH__) && defined(__APPLE__)
        #define MEMPOOL_TARGET_OSX
    #elif(defined(__sun__) && defined(__svr4__))
        #define MEMPOOL_TARGET_POSIX
    #else
        #define MEMPOOL_TARGET_GENERIC
    #endif
#endif

/* if no platform can be detected, or for testing; forces MEMPOOL_TARGET_GENERIC, which uses malloc/free */
#if !defined(MEMPOOL_CONFIG_FORCEGENERIC)
    #define MEMPOOL_CONFIG_FORCEGENERIC 0
#endif

/*
#if defined(MEMPOOL_TARGET_WINDOWS)
    #undef MEMPOOL_TARGET_WINDOWS
    #define MEMPOOL_TARGET_GENERIC
#endif
*/

#if defined(MEMPOOL_TARGET_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #if defined(_MSC_VER)
        #include <intrin.h>
    #endif
#else
    #if defined(MEMPOOL_TARGET_LINUX)
        #include <errno.h>
        #include <sys/mman.h>
    #endif
#endif

#ifdef __CELLOS_LV2__
    #define MEMPOOL_TARGET_PS3 1
#endif

#ifdef __ORBIS__
    #define MEMPOOL_TARGET_PS4 1
    #undef NULL
    #define NULL ((void*)0)
#endif

/* Set target architecture properties. */

#if defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(__amd64) || defined(_M_AMD64)
    #define MEMPOOL_ARCH_IS64BIT
#endif

#ifndef MEMPOOL_PAGESIZE
    #define MEMPOOL_PAGESIZE 4096
#endif


#if !defined(MEMPOOL_LIKELY)
    #if defined(__GNUC__)
        #define MEMPOOL_LIKELY(x) __builtin_expect(!!(x), 1)
        #define MEMPOOL_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #else
        #define MEMPOOL_LIKELY(x) x
        #define MEMPOOL_UNLIKELY(x) x
    #endif
#endif

/* Bin types, widths and sizes */
#define MEMPOOL_CONST_MAXSIZET (~0)
#define MEMPOOL_CONST_MALLOCALIGNMENT (8)

/* #define MEMPOOL_CONST_DEFAULTGRANULARITY (128 * 1024) */
#define MEMPOOL_CONST_DEFAULTGRANULARITY (32 * 1024)


/* #define MEMPOOL_CONST_DEFAULTTRIMTHRESHOLD (2 * (1024 * 1024)) */
#define MEMPOOL_CONST_DEFAULTTRIMTHRESHOLD (1 * (1024 * 1024))

/* #define MEMPOOL_CONST_DEFAULTMMAPTHRESHOLD (128 * 1024) */
#define MEMPOOL_CONST_DEFAULTMMAPTHRESHOLD (32 * 1024)

#define MEMPOOL_CONST_MAXRELEASECHECKRATE (255)

/* ------------------- size_t and alignment properties -------------------- */

/* The byte and bit size of a size_t */
#define MEMPOOL_CONST_SIZETSIZE (sizeof(size_t))
#define MEMPOOL_CONST_SIZETBITSIZE (sizeof(size_t) << 3)

/* Some constants coerced to size_t */
/* Annoying but necessary to avoid errors on some platforms */
#define MEMPOOL_CONST_SIZETONE (1)
#define MEMPOOL_CONST_SIZETTWO (2)
#define MEMPOOL_CONST_TWOSIZETSIZES (MEMPOOL_CONST_SIZETSIZE << 1)
#define MEMPOOL_CONST_FOURSIZETSIZES (MEMPOOL_CONST_SIZETSIZE << 2)
#define MEMPOOL_CONST_SIXSIZETSIZES (MEMPOOL_CONST_FOURSIZETSIZES + MEMPOOL_CONST_TWOSIZETSIZES)


#define MEMPOOL_CONST_NSMALLBINS (32)
#define MEMPOOL_CONST_NTREEBINS (32)
#define MEMPOOL_CONST_SMALLBINSHIFT (3)
#define MEMPOOL_CONST_TREEBINSHIFT (8)
#define MEMPOOL_CONST_MINLARGESIZE (MEMPOOL_CONST_SIZETONE << MEMPOOL_CONST_TREEBINSHIFT)
#define MEMPOOL_CONST_MAXSMALLSIZE (MEMPOOL_CONST_MINLARGESIZE - MEMPOOL_CONST_SIZETONE)
#define MEMPOOL_CONST_MAXSMALLREQUEST (MEMPOOL_CONST_MAXSMALLSIZE - MEMPOOL_CHUNK_ALIGNMASK - MEMPOOL_CHUNK_OVERHEAD)

/* weird cornercase in which mremap doesn't get declared despite _GNU_SOURCE. fucking dumb */
#if defined(MEMPOOL_TARGET_LINUX) || defined(MEMPOOL_TARGET_POSIX)
    MEMPOOL_CPP_BEGINEXTERN()
    void* mremap(void* old_address, size_t old_size, size_t new_size, int flags, ...);
    MEMPOOL_CPP_ENDEXTERN()
#endif

/* Described below */
typedef size_t MempoolBindex;

/* Described below */
typedef unsigned int MempoolBinMap;

typedef struct MempoolPlainChunk MempoolPlainChunk;
typedef struct MempoolSegment MempoolSegment;
typedef struct MempoolTreeChunk MempoolTreeChunk;
typedef struct MempoolState MempoolState;

struct MempoolPlainChunk
{
    /* Size of previous chunk (if free).  */
    size_t prev_foot;
    /* Size and inuse bits. */
    size_t head;
    /* double links -- used only if free. */
    MempoolPlainChunk *fd;
    MempoolPlainChunk *bk;
};


struct MempoolTreeChunk
{
    /* The first four fields must be compatible with MempoolPlainChunk */
    size_t prev_foot;
    size_t head;
    MempoolTreeChunk *fd;
    MempoolTreeChunk *bk;

    MempoolTreeChunk *child[2];
    MempoolTreeChunk *parent;
    MempoolBindex index;
};

struct MempoolSegment
{
    /* base address */
    char *base;
    /* allocated size */
    size_t size;
    MempoolSegment *next;
};

struct MempoolState
{
    MempoolBinMap smallmap;
    MempoolBinMap treemap;
    size_t dvsize;
    size_t topsize;
    MempoolPlainChunk* dv;
    MempoolPlainChunk* top;
    size_t trim_check;
    size_t release_checks;
    MempoolPlainChunk* smallbins[(MEMPOOL_CONST_NSMALLBINS+1)*2];
    MempoolTreeChunk* treebins[MEMPOOL_CONST_NTREEBINS];
    MempoolSegment seg;
};

MEMPOOL_CPP_BEGINEXTERN()

/**
* the only functions you should be using
*/

/* creates a memory pool state. required to use mempool_user* */
void *mempool_createpool();

/* destroys a memory pool */
void mempool_destroypool(void *msp);

void *mempool_usermalloc(void *msp, size_t nsize);
void *mempool_userfree(void *msp, void *ptr);
void *mempool_userrealloc(void *msp, void *ptr, size_t nsize);


MEMPOOL_CPP_ENDEXTERN()

/*
* if need be, everything below this comment can be moved into a separate file
*/

/* The bit mask value corresponding to MEMPOOL_CONST_MALLOCALIGNMENT */
#define MEMPOOL_CHUNK_ALIGNMASK  (MEMPOOL_CONST_MALLOCALIGNMENT - MEMPOOL_CONST_SIZETONE)


/* -------------------------- MMAP support ------------------------------- */

#define MEMPOOL_ON_MFAIL ((void*)(MEMPOOL_CONST_MAXSIZET))
#define MEMPOOL_ON_CMFAIL ((char*)(MEMPOOL_ON_MFAIL)) /* defined for convenience */

#define MEMPOOL_ISDIRECTBIT (MEMPOOL_CONST_SIZETONE)

#if defined(MEMPOOL_TARGET_WINDOWS)
    #if defined(MEMPOOL_ARCH_IS64BIT)
        /* Number of top bits of the lower 32 bits of an address that must be zero.
        ** Apparently 0 gives us full 64 bit addresses and 1 gives us the lower 2GB.
        */
        #define MEMPOOL_NTAVM_ZEROBITS 1
    #else
        #define mempool_util_initmmap() ((void)0)
    #endif
#else
    #define MEMPOOL_MMAP_PROT (PROT_READ | PROT_WRITE)
    #if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
        #define MAP_ANONYMOUS MAP_ANON
    #endif
    #define MEMPOOL_MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)

    #if defined(MEMPOOL_ARCH_IS64BIT)
    /* 64 bit mode needs special support for allocating memory in the lower 2GB. */

        #if defined(MAP_32BIT)
        #elif defined(MEMPOOL_TARGET_OSX) || defined(MEMPOOL_TARGET_PS4)

            /* OSX mmap() uses a naive first-fit linear search.
            ** That's perfect for us. Except that -pagezero_size must be set for OSX,
            ** otherwise the lower 4GB are blocked. And the 32GB RLIMIT_DATA needs
            ** to be reduced to 250MB.
            */
            #if defined(MEMPOOL_TARGET_OSX)
                #define MEMPOOL_MMAP_REGION_START ((uintptr_t)0x10000)
            #elif defined(MEMPOOL_TARGET_PS4)
                #define MEMPOOL_MMAP_REGION_START ((uintptr_t)0x4000)
            #else
                #define MEMPOOL_MMAP_REGION_START ((uintptr_t)0x10000000)
            #endif
            #define MEMPOOL_MMAP_REGION_END ((uintptr_t)0x80000000)

            #if !defined(MEMPOOL_TARGET_PS4)
                #include <sys/resource.h>
            #endif
        #else
            #if 0
                #warning "NYI: need an equivalent of MAP_32BIT for this 64 bit OS"
            #endif
        #endif
    #endif
    #define mempool_util_initmmap() ((void)0)
    #define mempool_util_calldirectmmap(s) mempool_util_callvirtalloc(s)
    #if defined(MEMPOOL_TARGET_LINUX)
        #define mempool_util_callvirtremap(addr, osz, nsz, mv) mempool_util_callvirtremapactual((addr), (osz), (nsz), (mv))
        #define MEMPOOL_CALLMREMAPNOMOVE 0
        /* #define CALL_MREMAP_MAYMOVE 1 */
        #if defined(MEMPOOL_ARCH_IS64BIT)
            #define MEMPOOL_CALLMREMAPMV MEMPOOL_CALLMREMAPNOMOVE
        #else
            #define MEMPOOL_CALLMREMAPMV CALL_MREMAP_MAYMOVE
        #endif
    #endif
#endif

#ifndef mempool_util_callvirtremap
    #define mempool_util_callvirtremap(addr, osz, nsz, mv) ((void)osz, MEMPOOL_ON_MFAIL)
#endif

/* ------------------- Chunks sizes and alignments ----------------------- */

#define MEMPOOL_MCHUNK_SIZE (sizeof(MempoolPlainChunk))

#define MEMPOOL_CHUNK_OVERHEAD (MEMPOOL_CONST_SIZETSIZE)

/* Direct chunks need a second word of overhead ... */
#define MEMPOOL_DIRECT_CHUNK_OVERHEAD (MEMPOOL_CONST_TWOSIZETSIZES)
/* ... and additional padding for fake next-chunk at foot */
#define MEMPOOL_DIRECTFOOTPAD (MEMPOOL_CONST_FOURSIZETSIZES)

/* The smallest size we can malloc is an aligned minimal chunk */
#define MEMPOOL_MINCHUNKSIZE \
    ((MEMPOOL_MCHUNK_SIZE + MEMPOOL_CHUNK_ALIGNMASK) & ~MEMPOOL_CHUNK_ALIGNMASK)


/* Bounds on request (not chunk) sizes. */
#define MEMPOOL_REQUESTS_MAX ((~MEMPOOL_MINCHUNKSIZE + 1) << 2)
#define MEMPOOL_REQUESTS_MIN (MEMPOOL_MINCHUNKSIZE - MEMPOOL_CHUNK_OVERHEAD - MEMPOOL_CONST_SIZETONE)


/* the number of bytes to offset an address to align it */
MEMPOOL_INLINE size_t mempool_util_alignoffset(void* a)
{
    if(((size_t)a & MEMPOOL_CHUNK_ALIGNMASK) == 0)
    {
        return 0;
    }
    return ((MEMPOOL_CONST_MALLOCALIGNMENT - ((size_t)a & MEMPOOL_CHUNK_ALIGNMASK)) & MEMPOOL_CHUNK_ALIGNMASK);
}

/* conversion from malloc headers to user pointers, and back */
MEMPOOL_INLINE void* mempool_util_chunk2mem(void* p)
{
    return ((void*)((char*)p + MEMPOOL_CONST_TWOSIZETSIZES));
}

MEMPOOL_INLINE MempoolPlainChunk* mempool_util_mem2chunk(void* mem)
{
    return ((MempoolPlainChunk*)((char*)mem - MEMPOOL_CONST_TWOSIZETSIZES));
}
    
/* chunk associated with aligned address a */
MEMPOOL_INLINE MempoolPlainChunk* mempool_util_alignaschunk(char* a)
{
    return (MempoolPlainChunk*)(a + mempool_util_alignoffset(mempool_util_chunk2mem(a)));
}

/* pad request bytes into a usable size */
MEMPOOL_INLINE size_t mempool_util_padrequest(size_t req)
{
    return (((req) + MEMPOOL_CHUNK_OVERHEAD + MEMPOOL_CHUNK_ALIGNMASK) & ~MEMPOOL_CHUNK_ALIGNMASK);
}

/* pad request, checking for minimum (but not maximum) */
MEMPOOL_INLINE size_t mempool_util_request2size(size_t req)
{
    return (((req) < MEMPOOL_REQUESTS_MIN) ? MEMPOOL_MINCHUNKSIZE : mempool_util_padrequest(req));
}

/* ------------------ Operations on head and foot fields ----------------- */

#define MEMPOOL_PINUSE_BIT (MEMPOOL_CONST_SIZETONE)
#define MEMPOOL_CINUSE_BIT (MEMPOOL_CONST_SIZETTWO)
#define MEMPOOL_INUSE_BITS (MEMPOOL_PINUSE_BIT | MEMPOOL_CINUSE_BIT)

/* Head value for fenceposts */
#define MEMPOOL_FENCEPOST_HEAD (MEMPOOL_INUSE_BITS | MEMPOOL_CONST_SIZETSIZE)

/* extraction of fields from head words */
#define mempool_util_cinuse(p) ((p)->head & MEMPOOL_CINUSE_BIT)
#define mempool_util_pinuse(p) ((p)->head & MEMPOOL_PINUSE_BIT)
#define mempool_util_chunksize(p) ((p)->head & ~(MEMPOOL_INUSE_BITS))

#define mempool_util_clearpinuse(p) ((p)->head &= ~MEMPOOL_PINUSE_BIT)

/* Treat space at ptr +/- offset as a chunk */
#define mempool_util_chunkplusoffset(p, s) ((MempoolPlainChunk*)(((char*)(p)) + (s)))
#define mempool_util_chunkminusoffset(p, s) ((MempoolPlainChunk*)(((char*)(p)) - (s)))


/* Set cinuse and pinuse of this chunk and pinuse of next chunk */
#define mempool_util_setinuseandpinuse(p, s) \
    { \
        (p)->head = (s | MEMPOOL_PINUSE_BIT | MEMPOOL_CINUSE_BIT); \
        ((MempoolPlainChunk*)(((char*)(p)) + (s)))->head |= MEMPOOL_PINUSE_BIT; \
    }


/* Set size, cinuse and pinuse bit of this chunk */
#define mempool_util_setsizeandpinuseofinusechunk(p, s) \
    { \
        (p)->head = ((s) | MEMPOOL_PINUSE_BIT | MEMPOOL_CINUSE_BIT); \
    }


/* Ptr to next or previous physical MempoolPlainChunk. */
MEMPOOL_INLINE MempoolPlainChunk* mempool_util_nextchunk(MempoolPlainChunk* p)
{
    return ((MempoolPlainChunk*)(((char*)p) + (p->head & ~MEMPOOL_INUSE_BITS)));
}

/* Get/set size at footer */
MEMPOOL_INLINE void mempool_util_setfoot(MempoolPlainChunk* p, size_t s)
{
    ((MempoolPlainChunk*)((char*)p + s))->prev_foot = s;
}

/* Set size, pinuse bit, and foot */
MEMPOOL_INLINE void mempool_util_setsizeand_pinuseoffreechunk(MempoolPlainChunk* p, size_t s)
{
    p->head = (s | MEMPOOL_PINUSE_BIT);
    mempool_util_setfoot(p, s);
}

/* Set size, pinuse bit, foot, and clear next pinuse */
MEMPOOL_INLINE void mempool_util_setfreewithpinuse(MempoolPlainChunk* p, size_t s, MempoolPlainChunk* n)
{
    mempool_util_clearpinuse(n);
    mempool_util_setsizeand_pinuseoffreechunk(p, s);
}

MEMPOOL_INLINE bool mempool_util_isdirect(MempoolPlainChunk* p)
{
    return (!(p->head & MEMPOOL_PINUSE_BIT) && (p->prev_foot & MEMPOOL_ISDIRECTBIT));
}

/* Get the internal overhead associated with chunk p */
MEMPOOL_INLINE size_t mempool_util_overheadfor(MempoolPlainChunk* p)
{
    return (mempool_util_isdirect(p) ? MEMPOOL_DIRECT_CHUNK_OVERHEAD : MEMPOOL_CHUNK_OVERHEAD);
}

/* ---------------------- Overlaid data structures ----------------------- */

/* A little helper macro for trees */
MEMPOOL_INLINE MempoolTreeChunk* mempool_util_leftmostchild(MempoolTreeChunk* t)
{
    return (t->child[0] != 0 ? t->child[0] : t->child[1]);
}

MEMPOOL_INLINE bool mempool_util_isstateinitialized(MempoolState* mst)
{
    return (mst->top != 0);
}

/* -------------------------- system alloc setup ------------------------- */

/* page-align a size */
MEMPOOL_INLINE size_t mempool_util_pagealign(size_t sz)
{
    return ((sz + (MEMPOOL_PAGESIZE - MEMPOOL_CONST_SIZETONE)) & ~(MEMPOOL_PAGESIZE - MEMPOOL_CONST_SIZETONE));
}

/* granularity-align a size */
MEMPOOL_INLINE size_t mempool_util_granularityalign(size_t sz)
{
    return ((sz + (MEMPOOL_CONST_DEFAULTGRANULARITY - MEMPOOL_CONST_SIZETONE)) & ~(MEMPOOL_CONST_DEFAULTGRANULARITY - MEMPOOL_CONST_SIZETONE));
}

/*  True if segment segm holds address a */
MEMPOOL_INLINE bool mempool_util_segmentholds(MempoolSegment* segm, void* a)
{
    return ((char*)a >= segm->base && (char*)a < segm->base + segm->size);
}

MEMPOOL_INLINE size_t mempool_util_virtalign(size_t sz)
{
    #if defined(MEMPOOL_TARGET_WINDOWS)
        return mempool_util_granularityalign(sz);
    #else
        return mempool_util_pagealign(sz);
    #endif
}

#if defined(MEMPOOL_TARGET_WINDOWS)
    #if defined(MEMPOOL_ARCH_IS64BIT)
        /* Undocumented, but hey, that's what we all love so much about Windows. */
        typedef long (*PNTAVM)(HANDLE handle, void** addr, ULONG zbits, size_t* size, ULONG alloctype, ULONG prot);
        static PNTAVM ntavm;
    #endif
    MEMPOOL_INLINE void mempool_util_initmmap()
    {
        ntavm = (PNTAVM)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtAllocateVirtualMemory");
    }

    /* Win64 32 bit MMAP via NtAllocateVirtualMemory. */
    MEMPOOL_INLINE void* mempool_util_callvirtalloc(size_t size)
    {
        #if defined(MEMPOOL_ARCH_IS64BIT)
            long st;
            DWORD olderr;
            void* ptr;
            olderr = GetLastError();
            ptr = NULL;
            st = ntavm(INVALID_HANDLE_VALUE, &ptr, MEMPOOL_NTAVM_ZEROBITS, &size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            SetLastError(olderr);
            return st == 0 ? ptr : MEMPOOL_ON_MFAIL;
        #else
            void* ptr;
            DWORD olderr;
            olderr = GetLastError();
            ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            SetLastError(olderr);
            return ptr ? ptr : MEMPOOL_ON_MFAIL;
        #endif
    }

    /* For direct MMAP, use MEM_TOP_DOWN to minimize interference */
    MEMPOOL_INLINE void* mempool_util_calldirectmmap(size_t size)
    {
        #if defined(MEMPOOL_ARCH_IS64BIT)
            long st;
            DWORD olderr;
            void* ptr;
            olderr = GetLastError();
            ptr = NULL;
            st = ntavm(INVALID_HANDLE_VALUE, &ptr, MEMPOOL_NTAVM_ZEROBITS, &size, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
            SetLastError(olderr);
            return st == 0 ? ptr : MEMPOOL_ON_MFAIL;
        #else
            void* ptr;
            DWORD olderr;
            olderr = GetLastError();
            ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
            SetLastError(olderr);
            return ptr ? ptr : MEMPOOL_ON_MFAIL;
        #endif
    }

    /* This function supports releasing coalesed segments */
    MEMPOOL_INLINE int mempool_util_callvirtfree(void* ptr, size_t size)
    {
        char* cptr;
        DWORD olderr;
        MEMORY_BASIC_INFORMATION minfo;
        olderr = GetLastError();
        cptr = (char*)ptr;
        while(size)
        {
            if(VirtualQuery(cptr, &minfo, sizeof(minfo)) == 0)
            {
                return -1;
            }
            if(minfo.BaseAddress != cptr || minfo.AllocationBase != cptr || minfo.State != MEM_COMMIT || minfo.RegionSize > size)
            {
                return -1;
            }
            if(VirtualFree(cptr, 0, MEM_RELEASE) == 0)
            {
                return -1;
            }
            cptr += minfo.RegionSize;
            size -= minfo.RegionSize;
        }
        SetLastError(olderr);
        return 0;
    }
#else
    MEMPOOL_INLINE void* mempool_util_callvirtalloc(size_t size)
    {
        #if defined(MEMPOOL_ARCH_IS64BIT)
        /* 64 bit mode needs special support for allocating memory in the lower 2GB. */
            #if defined(MAP_32BIT)
                /* Actually this only gives us max. 1GB in current Linux kernels. */
                int olderr;
                void* ptr;
                olderr = errno;
                ptr = mmap(NULL, size, MEMPOOL_MMAP_PROT, MAP_32BIT | MEMPOOL_MMAP_FLAGS, -1, 0);
                errno = olderr;
                return ptr;
            #elif defined(MEMPOOL_TARGET_OSX) || defined(MEMPOOL_TARGET_PS4)
                int olderr;
                int retry;
                void* p;
                #if !defined(MEMPOOL_TARGET_PS4)
                    struct rlimit rlim;
                #endif
                static uintptr_t allochint = MEMPOOL_MMAP_REGION_START;
                #if !defined(MEMPOOL_TARGET_PS4)
                    static int rlimit_modified = 0;
                #endif
                    olderr = errno;
                    /* Hint for next allocation. Doesn't need to be thread-safe. */
                    retry = 0;
                    #if !defined(MEMPOOL_TARGET_PS4)
                        if(MEMPOOL_UNLIKELY(rlimit_modified == 0))
                        {
                            rlim.rlim_cur = rlim.rlim_max = MEMPOOL_MMAP_REGION_START;
                            setrlimit(RLIMIT_DATA, &rlim); /* Ignore result. May fail below. */
                            rlimit_modified = 1;
                        }
                    #endif
                    for(;;)
                    {
                        p = mmap((void*)allochint, size, MEMPOOL_MMAP_PROT, MEMPOOL_MMAP_FLAGS, -1, 0);
                        if((uintptr_t)p >= MEMPOOL_MMAP_REGION_START && (uintptr_t)p + size < MEMPOOL_MMAP_REGION_END)
                        {
                            allochint = (uintptr_t)p + size;
                            errno = olderr;
                            return p;
                        }
                        if(p != MEMPOOL_ON_CMFAIL)
                        {
                            munmap(p, size);
                        }
                        if(retry)
                        {
                            break;
                        }
                        retry = 1;
                        allochint = MEMPOOL_MMAP_REGION_START;
                    }
                    errno = olderr;
                    return MEMPOOL_ON_CMFAIL
            #elif defined(MEMPOOL_TARGET_GENERIC)
                return malloc(size);
            #else
                #warning "NYI: need an equivalent of MAP_32BIT for this 64 bit OS"
            #endif
        #else
            /* 32 bit mode is easy. */
                #if defined(MEMPOOL_TARGET_GENERIC)
                    return malloc(size);
                #else
                    int olderr;
                    void* ptr;
                    olderr = errno;
                    ptr = mmap(NULL, size, MEMPOOL_MMAP_PROT, MEMPOOL_MMAP_FLAGS, -1, 0);
                    errno = olderr;
                    return ptr;
                #endif
        #endif
    }
    MEMPOOL_INLINE int mempool_util_callvirtfree(void* ptr, size_t size)
    {
        #if defined(MEMPOOL_TARGET_GENERIC)
            (void)size;
            free(ptr);
            return 0;
        #else    
            int ret;
            int olderr;
            olderr = errno;
            ret = munmap(ptr, size);
            errno = olderr;
            return ret;
        #endif

    }
    #if defined(MEMPOOL_TARGET_LINUX)
        /* Need to define _GNU_SOURCE to get the mremap prototype. */
        MEMPOOL_INLINE void* mempool_util_callvirtremapactual(void* ptr, size_t osz, size_t nsz, int flags)
        {
            int olderr;
            olderr = errno;
            ptr = mremap(ptr, osz, nsz, flags);
            errno = olderr;
            return ptr;
        }
    #endif
#endif

/* In generic mode the host allocator (malloc/free) keeps released segments in
** its own arena, so the pages are never given back to the OS the way munmap
** would. Nudge the host allocator to actually release the free memory. */
#if defined(MEMPOOL_TARGET_GENERIC)
    #if defined(__GLIBC__) || defined(__MUSL__)
        MEMPOOL_INLINE void mempool_util_releasetoos()
        {
            malloc_trim(0);
        }
    #else
        MEMPOOL_INLINE void mempool_util_releasetoos()
        {
            (void)0;
        }
    #endif
#else
    MEMPOOL_INLINE void mempool_util_releasetoos()
    {
        (void)0;
    }
#endif

/*
* do not attempt to optimize these two functions further.
* bad things will happen if you do.
*/
MEMPOOL_INLINE int mempool_util_nativebitscanreverse(uint64_t x)
{
    static const char bsr_debruijntable[64] = {
        0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28, 16, 3,  61,
        54, 58, 35, 52, 50, 42, 21, 44, 38, 32, 29, 23, 17, 11, 4,  62,
        46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43, 31, 22, 10, 45,
        25, 39, 14, 33, 19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63,
    };
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return bsr_debruijntable[(x * 0x03f79d71b4cb0a89) >> 58];
}

MEMPOOL_INLINE int mempool_util_nativebitscanforward(uint64_t x)
{
    uint32_t l;
    uint32_t r;
    x &= -x;
    l = x | x >> 32;
    r = !!(x >> 32);
    r <<= 1;
    r += !!((l & 0xffff0000));
    r <<= 1;
    r += !!((l & 0xff00ff00));
    r <<= 1;
    r += !!((l & 0xf0f0f0f0));
    r <<= 1;
    r += !!((l & 0xcccccccc));
    r <<= 1;
    r += !!((l & 0xaaaaaaaa));
    return r;
}

/* Return segment holding given address */
MEMPOOL_INLINE MempoolSegment* mempool_util_segmentholding(MempoolState* m, char* addr)
{
    MempoolSegment* sp;
    sp = &m->seg;
    for(;;)
    {
        if(addr >= sp->base && addr < sp->base + sp->size)
        {
            return sp;
        }
        if((sp = sp->next) == 0)
        {
            return NULL;
        }
    }
    return NULL;
}

/* Return true if segment contains a segment link */
MEMPOOL_INLINE int mempool_util_hassegmentlink(MempoolState* m, MempoolSegment* ss)
{
    MempoolSegment* sp;
    sp = &m->seg;
    for(;;)
    {
        if((char*)sp >= ss->base && (char*)sp < ss->base + ss->size)
        {
            return 1;
        }
        if((sp = sp->next) == 0)
        {
            return 0;
        }
    }
    return 0;
}

/*
  mempool_util_topfootsize is padding at the end of a segment, including space
  that may be needed to place segment records and fenceposts when new
  noncontiguous segments are added.
*/
MEMPOOL_INLINE size_t mempool_util_topfootsize()
{
    return (mempool_util_alignoffset(mempool_util_chunk2mem(0)) + mempool_util_padrequest(sizeof(MempoolSegment)) + MEMPOOL_MINCHUNKSIZE);
}

/* ---------------------------- Indexing Bins ---------------------------- */

MEMPOOL_INLINE bool mempool_util_issmall(size_t s)
{
    return ((s >> MEMPOOL_CONST_SMALLBINSHIFT) < MEMPOOL_CONST_NSMALLBINS);
}

MEMPOOL_INLINE MempoolBindex mempool_util_smallindex(MempoolBindex s)
{
    return (s >> MEMPOOL_CONST_SMALLBINSHIFT);
}

MEMPOOL_INLINE size_t mempool_util_smallindex2size(MempoolBindex i)
{
    return (i << MEMPOOL_CONST_SMALLBINSHIFT);
}

/* addressing by index. See above about smallbin repositioning */
MEMPOOL_INLINE MempoolPlainChunk* mempool_util_smallbinat(MempoolState* mst, size_t i)
{
    return ((MempoolPlainChunk*)((char*)&(mst->smallbins[i << 1])));
}

MEMPOOL_INLINE MempoolTreeChunk** mempool_util_treebinat(MempoolState* mst, size_t i)
{
    return (&(mst->treebins[i]));
}

/* assign tree index for size sz to variable desti */
MEMPOOL_INLINE void mempool_util_computetreeindex(size_t sz, size_t* desti)
{
    unsigned int x;
    unsigned int k;
    x = (unsigned int)(sz >> MEMPOOL_CONST_TREEBINSHIFT);
    if(x == 0)
    {
        (*desti) = 0;
    }
    else if(x > 0xFFFF)
    {
        (*desti) = MEMPOOL_CONST_NTREEBINS - 1;
    }
    else
    {
        k = mempool_util_nativebitscanreverse(x);
        (*desti) = (MempoolBindex)((k << 1) + ((sz >> (k + (MEMPOOL_CONST_TREEBINSHIFT - 1)) & 1)));
    }
}

/* Shift placing maximum resolved bit in a treebin at i as sign bit */
MEMPOOL_INLINE size_t mempool_util_leftshiftfortreeindex(size_t i)
{
    if(i == MEMPOOL_CONST_NTREEBINS - 1)
    {
        return 0;
    }
    return ((MEMPOOL_CONST_SIZETBITSIZE - MEMPOOL_CONST_SIZETONE) - ((i >> 1) + MEMPOOL_CONST_TREEBINSHIFT - 2));
}


/* ------------------------ Operations on bin maps ----------------------- */

/* bit corresponding to given index */
MEMPOOL_INLINE MempoolBinMap mempool_util_idx2bit(size_t i)
{
    return ((MempoolBinMap)(1) << i);
}

/* Mark/Clear bits with given index */
MEMPOOL_INLINE void mempool_util_marksmallmap(MempoolState* mst, size_t i)
{
    mst->smallmap |= mempool_util_idx2bit(i);
}

MEMPOOL_INLINE void mempool_util_clearsmallmap(MempoolState* mst, size_t i)
{
    mst->smallmap &= ~mempool_util_idx2bit(i);
}

MEMPOOL_INLINE bool mempool_util_smallmapismarked(MempoolState* mst, size_t i)
{
    return (mst->smallmap & mempool_util_idx2bit(i));
}

MEMPOOL_INLINE void mempool_util_marktreemap(MempoolState* mst, size_t i)
{
    mst->treemap |= mempool_util_idx2bit(i);
}

MEMPOOL_INLINE void mempool_util_cleartreemap(MempoolState* mst, size_t i)
{
    mst->treemap &= ~mempool_util_idx2bit(i);
}

MEMPOOL_INLINE bool mempool_util_treemapismarked(MempoolState* mst, size_t i)
{
    return (mst->treemap & mempool_util_idx2bit(i));
}

/* mask with all bits to left of least bit of x on */
MEMPOOL_INLINE MempoolBinMap mempool_util_leftbits(MempoolBinMap x)
{
    return ((x << 1) | (~(x << 1) + 1));
}

/* Set cinuse bit and pinuse bit of next chunk */
MEMPOOL_INLINE void mempool_util_setinuse(MempoolPlainChunk* p, size_t s)
{
    p->head = ((p->head & MEMPOOL_PINUSE_BIT) | s | MEMPOOL_CINUSE_BIT);
    ((MempoolPlainChunk*)(((char*)p) + s))->head |= MEMPOOL_PINUSE_BIT;
}

/* ----------------------- Operations on smallbins ----------------------- */

/* Link a free chunk into a smallbin  */
MEMPOOL_INLINE void mempool_util_insertsmallchunk(MempoolState* mst, MempoolPlainChunk* p, size_t sz)
{
    MempoolBindex i;
    MempoolPlainChunk* b;
    MempoolPlainChunk* f;
    i = mempool_util_smallindex(sz);
    b = mempool_util_smallbinat(mst, i);
    f = b;
    if(!mempool_util_smallmapismarked(mst, i))
    {
        mempool_util_marksmallmap(mst, i);
    }
    else
    {
        f = b->fd;
    }
    b->fd = p;
    f->bk = p;
    p->fd = f;
    p->bk = b;
}

/* Unlink a chunk from a smallbin  */
MEMPOOL_INLINE void mempool_util_unlinksmallchunk(MempoolState* mst, MempoolPlainChunk* p, size_t sz)
{
    MempoolBindex i;
    MempoolPlainChunk* b;
    MempoolPlainChunk* f;
    f = p->fd;
    b = p->bk;
    i = mempool_util_smallindex(sz);
    if(f == b)
    {
        mempool_util_clearsmallmap(mst, i);
    }
    else
    {
        f->bk = b;
        b->fd = f;
    }
}

/* Unlink the first chunk from a smallbin */
MEMPOOL_INLINE void mempool_util_unlinkfirstsmallchunk(MempoolState* mst, MempoolPlainChunk* b, MempoolPlainChunk* p, size_t i)
{
    MempoolPlainChunk* f;
    f = p->fd;
    if(b == f)
    {
        mempool_util_clearsmallmap(mst, i);
    }
    else
    {
        b->fd = f;
        f->bk = b;
    }
}

/* Replace dv node, binning the old one */
/* Used only when dvsize known to be small */
MEMPOOL_INLINE void mempool_util_replacedv(MempoolState* mst, MempoolPlainChunk* p, size_t sz)
{
    size_t dvs;
    MempoolPlainChunk* dval;
    dvs = mst->dvsize;
    if(dvs != 0)
    {
        dval = mst->dv;
        mempool_util_insertsmallchunk(mst, dval, dvs);
    }
    mst->dvsize = sz;
    mst->dv = p;
}


/* ------------------------- Operations on trees ------------------------- */

/* Insert chunk into tree */
MEMPOOL_INLINE void mempool_util_insertlargechunk(MempoolState* mst, MempoolTreeChunk* tchunk, size_t psz)
{
    size_t k;

    MempoolBindex i;
    MempoolTreeChunk* t;
    MempoolTreeChunk** hp;
    MempoolTreeChunk** cchunk;
    mempool_util_computetreeindex(psz, &i);
    hp = mempool_util_treebinat(mst, i);
    tchunk->index = i;
    tchunk->child[0] = tchunk->child[1] = 0;
    if(!mempool_util_treemapismarked(mst, i))
    {
        mempool_util_marktreemap(mst, i);
        *hp = tchunk;
        tchunk->parent = (MempoolTreeChunk*)hp;
        tchunk->fd = tchunk->bk = tchunk;
    }
    else
    {
        t = *hp;
        k = psz << mempool_util_leftshiftfortreeindex(i);
        for(;;)
        {
            if(mempool_util_chunksize(t) != psz)
            {
                cchunk = &(t->child[(k >> (MEMPOOL_CONST_SIZETBITSIZE - MEMPOOL_CONST_SIZETONE)) & 1]);
                k <<= 1;
                if(*cchunk != 0)
                {
                    t = *cchunk;
                }
                else
                {
                    *cchunk = tchunk;
                    tchunk->parent = t;
                    tchunk->fd = tchunk->bk = tchunk;
                    break;
                }
            }
            else
            {
                MempoolTreeChunk* f = t->fd;
                t->fd = f->bk = tchunk;
                tchunk->fd = f;
                tchunk->bk = t;
                tchunk->parent = 0;
                break;
            }
        }
    }
}

MEMPOOL_INLINE void mempool_util_unlinklargechunk(MempoolState* mst, MempoolTreeChunk* tchunk)
{
    MempoolTreeChunk* f;
    MempoolTreeChunk* r;
    MempoolTreeChunk* c0;
    MempoolTreeChunk* c1;
    MempoolTreeChunk* xp;
    MempoolTreeChunk** rp;
    MempoolTreeChunk** cp;
    MempoolTreeChunk** hp;
    xp = tchunk->parent;
    if(tchunk->bk != tchunk)
    {
        f = tchunk->fd;
        r = tchunk->bk;
        f->bk = r;
        r->fd = f;
    }
    else
    {
        if(((r = *(rp = &(tchunk->child[1]))) != 0) || ((r = *(rp = &(tchunk->child[0]))) != 0))
        {
            while((*(cp = &(r->child[1])) != 0) || (*(cp = &(r->child[0])) != 0))
            {
                r = *(rp = cp);
            }
            *rp = 0;
        }
    }
    if(xp != 0)
    {
        hp = mempool_util_treebinat(mst, tchunk->index);
        if(tchunk == *hp)
        {
            if((*hp = r) == 0)
            {
                mempool_util_cleartreemap(mst, tchunk->index);
            }
        }
        else
        {
            if(xp->child[0] == tchunk)
            {
                xp->child[0] = r;
            }
            else
            {
                xp->child[1] = r;
            }
        }
        if(r != 0)
        {

            r->parent = xp;
            if((c0 = tchunk->child[0]) != 0)
            {
                r->child[0] = c0;
                c0->parent = r;
            }
            if((c1 = tchunk->child[1]) != 0)
            {
                r->child[1] = c1;
                c1->parent = r;
            }
        }
    }
}

/* Relays to large vs small bin operations */

MEMPOOL_INLINE void mempool_util_insertchunk(MempoolState* mst, MempoolPlainChunk* p, size_t psz)
{
    MempoolTreeChunk* tp;
    if(mempool_util_issmall(psz))
    {
        mempool_util_insertsmallchunk(mst, p, psz);
    }
    else
    {
        tp = (MempoolTreeChunk*)p;
        mempool_util_insertlargechunk(mst, tp, psz);
    }
}

MEMPOOL_INLINE void mempool_util_unlinkchunk(MempoolState* mst, MempoolPlainChunk* p, size_t psz)
{
    MempoolTreeChunk* tp;
    if(mempool_util_issmall(psz))
    {
        mempool_util_unlinksmallchunk(mst, p, psz);
    }
    else
    {
        tp = (MempoolTreeChunk*)p;
        mempool_util_unlinklargechunk(mst, tp);
    }
}

/* -----------------------  Direct-mmapping chunks ----------------------- */

MEMPOOL_INLINE void* mempool_util_directalloc(size_t nb)
{
    size_t psize;
    size_t mmsize;
    size_t offset;
    char* mm;
    MempoolPlainChunk* p;
    mmsize = mempool_util_virtalign(nb + MEMPOOL_CONST_SIXSIZETSIZES + MEMPOOL_CHUNK_ALIGNMASK);
    if(MEMPOOL_LIKELY(mmsize > nb))
    { /* Check for wrap around 0 */
        mm = (char*)(mempool_util_calldirectmmap(mmsize));
        if(mm != MEMPOOL_ON_CMFAIL)
        {
            offset = mempool_util_alignoffset(mempool_util_chunk2mem(mm));
            psize = mmsize - offset - MEMPOOL_DIRECTFOOTPAD;
            p = (MempoolPlainChunk*)(mm + offset);
            p->prev_foot = offset | MEMPOOL_ISDIRECTBIT;
            p->head = psize | MEMPOOL_CINUSE_BIT;
            mempool_util_chunkplusoffset(p, psize)->head = MEMPOOL_FENCEPOST_HEAD;
            mempool_util_chunkplusoffset(p, psize + MEMPOOL_CONST_SIZETSIZE)->head = 0;
            return mempool_util_chunk2mem(p);
        }
    }
    return NULL;
}

MEMPOOL_INLINE MempoolPlainChunk* mempool_util_directresize(MempoolPlainChunk* oldp, size_t nb)
{
    size_t psize;
    size_t offset;
    size_t oldsize;
    size_t oldmmsize;
    size_t newmmsize;
    char* cp;
    MempoolPlainChunk* newp;
    oldsize = mempool_util_chunksize(oldp);
    if(mempool_util_issmall(nb)) /* Can't shrink direct regions below small size */
    {
        return NULL;
    }
    /* Keep old chunk if big enough but not too big */
    if(oldsize >= nb + MEMPOOL_CONST_SIZETSIZE && (oldsize - nb) <= (MEMPOOL_CONST_DEFAULTGRANULARITY >> 1))
    {
        return oldp;
    }
    else
    {
        offset = oldp->prev_foot & ~MEMPOOL_ISDIRECTBIT;
        oldmmsize = oldsize + offset + MEMPOOL_DIRECTFOOTPAD;
        newmmsize = mempool_util_virtalign(nb + MEMPOOL_CONST_SIXSIZETSIZES + MEMPOOL_CHUNK_ALIGNMASK);
        cp = (char*)mempool_util_callvirtremap((char*)oldp - offset, oldmmsize, newmmsize, MEMPOOL_CALLMREMAPMV);
        if(cp != MEMPOOL_ON_CMFAIL)
        {
            newp = (MempoolPlainChunk*)(cp + offset);
            psize = newmmsize - offset - MEMPOOL_DIRECTFOOTPAD;
            newp->head = psize | MEMPOOL_CINUSE_BIT;
            mempool_util_chunkplusoffset(newp, psize)->head = MEMPOOL_FENCEPOST_HEAD;
            mempool_util_chunkplusoffset(newp, psize + MEMPOOL_CONST_SIZETSIZE)->head = 0;
            return newp;
        }
    }
    return NULL;
}

/* -------------------------- mspace management -------------------------- */

/* Initialize top chunk and its size */
MEMPOOL_INLINE void mempool_util_inittop(MempoolState* m, MempoolPlainChunk* p, size_t psize)
{
    /* Ensure alignment */
    size_t offset;
    offset = mempool_util_alignoffset(mempool_util_chunk2mem(p));
    p = (MempoolPlainChunk*)((char*)p + offset);
    psize -= offset;
    m->top = p;
    m->topsize = psize;
    p->head = psize | MEMPOOL_PINUSE_BIT;
    /* set size of fake trailing chunk holding overhead space only once */
    mempool_util_chunkplusoffset(p, psize)->head = mempool_util_topfootsize();
    m->trim_check = MEMPOOL_CONST_DEFAULTTRIMTHRESHOLD; /* reset on each update */
}

/* Initialize bins for a new state that is otherwise zeroed out */
MEMPOOL_INLINE void mempool_util_initbins(MempoolState* m)
{
    /* Establish circular links for smallbins */
    MempoolBindex i;
    MempoolPlainChunk* bin;
    for(i = 0; i < MEMPOOL_CONST_NSMALLBINS; i++)
    {
        bin = mempool_util_smallbinat(m, i);
        bin->fd = bin->bk = bin;
    }
}

/* Allocate chunk and prepend remainder with chunk in successor base. */
MEMPOOL_INLINE void* mempool_util_prependalloc(MempoolState* m, char* newbase, char* oldbase, size_t nb)
{
    size_t nsize;
    size_t dsize;
    size_t tsize;
    size_t qsize;
    size_t psize;
    MempoolPlainChunk* p;
    MempoolPlainChunk* q;
    MempoolPlainChunk* oldfirst;
    p = mempool_util_alignaschunk(newbase);
    oldfirst = mempool_util_alignaschunk(oldbase);
    psize = (size_t)((char*)oldfirst - (char*)p);
    q = mempool_util_chunkplusoffset(p, nb);
    qsize = psize - nb;
    mempool_util_setsizeandpinuseofinusechunk(p, nb);
    /* consolidate remainder with first chunk of old base */
    if(oldfirst == m->top)
    {
        tsize = m->topsize += qsize;
        m->top = q;
        q->head = tsize | MEMPOOL_PINUSE_BIT;
    }
    else if(oldfirst == m->dv)
    {
        dsize = m->dvsize += qsize;
        m->dv = q;
        mempool_util_setsizeand_pinuseoffreechunk(q, dsize);
    }
    else
    {
        if(!mempool_util_cinuse(oldfirst))
        {
            nsize = mempool_util_chunksize(oldfirst);
            mempool_util_unlinkchunk(m, oldfirst, nsize);
            oldfirst = mempool_util_chunkplusoffset(oldfirst, nsize);
            qsize += nsize;
        }
        mempool_util_setfreewithpinuse(q, qsize, oldfirst);
        mempool_util_insertchunk(m, q, qsize);
    }
    return mempool_util_chunk2mem(p);
}

/* Add a segment to hold a new noncontiguous region */
MEMPOOL_INLINE void mempool_util_addsegment(MempoolState* m, char* tbase, size_t tsize)
{
    /* Determine locations and sizes of segment, fenceposts, old top */
    size_t psize;
    size_t ssize;
    size_t offset;
    char* oldtop;
    char* oldend;
    char* rawsp;
    char* asp;
    char* csp;
    MempoolSegment* oldsp;
    MempoolPlainChunk* sp;
    MempoolSegment* ss;
    MempoolPlainChunk* tnext;
    MempoolPlainChunk* p;
    MempoolPlainChunk* nextp;
    MempoolPlainChunk* q;
    MempoolPlainChunk* tn;
    oldtop = (char*)m->top;
    oldsp = mempool_util_segmentholding(m, oldtop);
    oldend = oldsp->base + oldsp->size;
    ssize = mempool_util_padrequest(sizeof(MempoolSegment));
    rawsp = oldend - (ssize + MEMPOOL_CONST_FOURSIZETSIZES + MEMPOOL_CHUNK_ALIGNMASK);
    offset = mempool_util_alignoffset(mempool_util_chunk2mem(rawsp));
    asp = rawsp + offset;
    csp = (asp < (oldtop + MEMPOOL_MINCHUNKSIZE)) ? oldtop : asp;
    sp = (MempoolPlainChunk*)csp;
    ss = (MempoolSegment*)(mempool_util_chunk2mem(sp));
    tnext = mempool_util_chunkplusoffset(sp, ssize);
    p = tnext;
    /* reset top to new space */
    mempool_util_inittop(m, (MempoolPlainChunk*)tbase, tsize - mempool_util_topfootsize());
    /* Set up segment record */
    mempool_util_setsizeandpinuseofinusechunk(sp, ssize);
    *ss = m->seg; /* Push current record */
    m->seg.base = tbase;
    m->seg.size = tsize;
    m->seg.next = ss;
    /* Insert trailing fenceposts */
    for(;;)
    {
        nextp = mempool_util_chunkplusoffset(p, MEMPOOL_CONST_SIZETSIZE);
        p->head = MEMPOOL_FENCEPOST_HEAD;
        if((char*)(&(nextp->head)) < oldend)
        {
            p = nextp;
        }
        else
        {
            break;
        }
    }
    /* Insert the rest of old top into a bin as an ordinary free chunk */
    if(csp != oldtop)
    {
        q = (MempoolPlainChunk*)oldtop;
        psize = (size_t)(csp - oldtop);
        tn = mempool_util_chunkplusoffset(q, psize);
        mempool_util_setfreewithpinuse(q, psize, tn);
        mempool_util_insertchunk(m, q, psize);
    }
}

/* -------------------------- System allocation -------------------------- */

MEMPOOL_INLINE void* mempool_util_allocsys(MempoolState* m, size_t nb)
{
    size_t req;
    size_t tsize;
    size_t rsize;
    void* mem;
    char* mp;
    char* tbase;
    char* oldbase;
    MempoolSegment* sp;
    MempoolPlainChunk* p;
    MempoolPlainChunk* r;
    tbase = MEMPOOL_ON_CMFAIL;
    tsize = 0;
    /* Directly map large chunks */
    if(MEMPOOL_UNLIKELY(nb >= MEMPOOL_CONST_DEFAULTMMAPTHRESHOLD))
    {
        mem = mempool_util_directalloc(nb);
        if(mem != 0)
        {
            return mem;
        }
    }
    req = nb + mempool_util_topfootsize() + MEMPOOL_CONST_SIZETONE;
    rsize = mempool_util_granularityalign(req);
    if(MEMPOOL_LIKELY(rsize > nb))
    { /* Fail if wraps around zero */
        mp = (char*)(mempool_util_callvirtalloc(rsize));
        if(mp != MEMPOOL_ON_CMFAIL)
        {
            tbase = mp;
            tsize = rsize;
        }
    }
    if(tbase != MEMPOOL_ON_CMFAIL)
    {
        sp = &m->seg;
        /* Try to merge with an existing segment */
        while(sp != 0 && tbase != sp->base + sp->size)
        {
            sp = sp->next;
        }
        if(sp != 0 && mempool_util_segmentholds(sp, m->top))
        { /* append */
            sp->size += tsize;
            mempool_util_inittop(m, m->top, m->topsize + tsize);
        }
        else
        {
            sp = &m->seg;
            while(sp != 0 && sp->base != tbase + tsize)
            {
                sp = sp->next;
            }
            if(sp != 0)
            {
                oldbase = sp->base;
                sp->base = tbase;
                sp->size += tsize;
                return mempool_util_prependalloc(m, tbase, oldbase, nb);
            }
            else
            {
                mempool_util_addsegment(m, tbase, tsize);
            }
        }
        if(nb < m->topsize)
        { /* Allocate from new or extended top space */
            rsize = m->topsize -= nb;
            p = m->top;
            r = m->top = mempool_util_chunkplusoffset(p, nb);
            r->head = rsize | MEMPOOL_PINUSE_BIT;
            mempool_util_setsizeandpinuseofinusechunk(p, nb);
            return mempool_util_chunk2mem(p);
        }
    }
    return NULL;
}

/* -----------------------  system deallocation -------------------------- */

/* Unmap and unlink any mmapped segments that don't contain used chunks */
MEMPOOL_INLINE size_t mempool_util_releaseunusedsegments(MempoolState* m)
{
    size_t released = 0;
    size_t nsegs = 0;
    MempoolSegment* pred = &m->seg;
    MempoolSegment* sp = pred->next;
    while(sp != 0)
    {
        char* base = sp->base;
        size_t size = sp->size;
        MempoolSegment* next = sp->next;
        nsegs++;
        {
            MempoolPlainChunk* p = mempool_util_alignaschunk(base);
            size_t psize = mempool_util_chunksize(p);
            /* Can unmap if first chunk holds entire segment and not finned */
            if(!mempool_util_cinuse(p) && (char*)p + psize >= base + size - mempool_util_topfootsize())
            {
                MempoolTreeChunk* tp = (MempoolTreeChunk*)p;
                if(p == m->dv)
                {
                    m->dv = 0;
                    m->dvsize = 0;
                }
                else
                {
                    mempool_util_unlinklargechunk(m, tp);
                }
                if(mempool_util_callvirtfree(base, size) == 0)
                {
                    released += size;
                    /* unlink obsoleted record */
                    sp = pred;
                    sp->next = next;
                }
                else
                { /* back out if cannot unmap */
                    mempool_util_insertlargechunk(m, tp, psize);
                }
            }
        }
        pred = sp;
        sp = next;
    }
    /* Reset check counter */
    m->release_checks = ((nsegs > MEMPOOL_CONST_MAXRELEASECHECKRATE) ? nsegs : MEMPOOL_CONST_MAXRELEASECHECKRATE);
    /* In generic mode free() alone doesn't return the pages to the OS, so
    ** nudge the host allocator to release the segments we just freed. */
    if(released != 0)
    {
        mempool_util_releasetoos();
    }
    return released;
}

MEMPOOL_INLINE int mempool_util_alloctrim(MempoolState* m, size_t pad)
{
    size_t released;
    #if !defined(MEMPOOL_TARGET_GENERIC)
        size_t unit;
        size_t extra;
        size_t newsize;
        MempoolSegment* sp;
    #endif
    released = 0;
    if(pad < MEMPOOL_REQUESTS_MAX && mempool_util_isstateinitialized(m))
    {
        pad += mempool_util_topfootsize(); /* ensure enough room for segment overhead */
        #if !defined(MEMPOOL_TARGET_GENERIC)
            /* Shrinking the top segment only makes sense with mmap-like virtual
            ** memory, where a tail sub-range can be unmapped. With a malloc/free
            ** based host allocator the tail is not an independently allocatable
            ** block, so attempting to free it would corrupt the heap. */
            if(m->topsize > pad)
            {
                /* Shrink top space in granularity-size units, keeping at least one */
                unit = MEMPOOL_CONST_DEFAULTGRANULARITY;
                extra = ((m->topsize - pad + (unit - MEMPOOL_CONST_SIZETONE)) / unit - MEMPOOL_CONST_SIZETONE) * unit;
                sp = mempool_util_segmentholding(m, (char*)m->top);
                if(sp->size >= extra && !mempool_util_hassegmentlink(m, sp))
                { /* can't shrink if pinned */
                    newsize = sp->size - extra;
                    /* Prefer mremap, fall back to munmap */
                    if((mempool_util_callvirtremap(sp->base, sp->size, newsize, MEMPOOL_CALLMREMAPNOMOVE) != MEMPOOL_ON_MFAIL) || (mempool_util_callvirtfree(sp->base + newsize, extra) == 0))
                    {
                        released = extra;
                    }
                }
                if(released != 0)
                {
                    sp->size -= released;
                    mempool_util_inittop(m, m->top, m->topsize - released);
                }
            }
        #endif
        /* Unmap any unused mmapped segments */
        released += mempool_util_releaseunusedsegments(m);
        /* Return the released pages to the OS when the host allocator is free()-based */
        if(released != 0)
        {
            mempool_util_releasetoos();
        }
        /* On failure, disable autotrim to avoid repeated failed future calls */
        if(released == 0 && m->topsize > m->trim_check)
        {
            m->trim_check = MEMPOOL_CONST_MAXSIZET;
        }
    }
    return (released != 0) ? 1 : 0;
}

/* ---------------------------- malloc support --------------------------- */

/* allocate a large request from the best fitting chunk in a treebin */
MEMPOOL_INLINE void* mempool_util_tmalloclarge(MempoolState* m, size_t nb)
{
    size_t trem;
    size_t rsize;
    size_t sizebits;
    MempoolPlainChunk* r;
    MempoolTreeChunk* v;
    MempoolTreeChunk* t;
    MempoolTreeChunk* rst;
    MempoolTreeChunk* rt;
    MempoolBindex idx;
    MempoolBinMap leftbits;
    v = 0;
    rsize = ~nb + 1; /* Unsigned negation */
    mempool_util_computetreeindex(nb, &idx);
    if((t = *mempool_util_treebinat(m, idx)) != 0)
    {
        /* Traverse tree for this bin looking for node with size == nb */
        sizebits = nb << mempool_util_leftshiftfortreeindex(idx);
        rst = 0; /* The deepest untaken right subtree */
        for(;;)
        {
            trem = mempool_util_chunksize(t) - nb;
            if(trem < rsize)
            {
                v = t;
                if((rsize = trem) == 0)
                {
                    break;
                }
            }
            rt = t->child[1];
            t = t->child[(sizebits >> (MEMPOOL_CONST_SIZETBITSIZE - MEMPOOL_CONST_SIZETONE)) & 1];
            if(rt != 0 && rt != t)
            {
                rst = rt;
            }
            if(t == 0)
            {
                t = rst; /* set t to least subtree holding sizes > nb */
                break;
            }
            sizebits <<= 1;
        }
    }
    if(t == 0 && v == 0)
    { /* set t to root of next non-empty treebin */
        leftbits = mempool_util_leftbits(mempool_util_idx2bit(idx)) & m->treemap;
        if(leftbits != 0)
        {
            t = *mempool_util_treebinat(m, mempool_util_nativebitscanforward(leftbits));
        }
    }
    while(t != 0)
    { /* find smallest of tree or subtree */
        trem = mempool_util_chunksize(t) - nb;
        if(trem < rsize)
        {
            rsize = trem;
            v = t;
        }
        t = mempool_util_leftmostchild(t);
    }
    /*  If dv is a better fit, return NULL so malloc will use it */
    if(v != 0 && rsize < (size_t)(m->dvsize - nb))
    {
        r = mempool_util_chunkplusoffset(v, nb);
        mempool_util_unlinklargechunk(m, v);
        if(rsize < MEMPOOL_MINCHUNKSIZE)
        {
            mempool_util_setinuseandpinuse(v, (rsize + nb));
        }
        else
        {
            mempool_util_setsizeandpinuseofinusechunk(v, nb);
            mempool_util_setsizeand_pinuseoffreechunk(r, rsize);
            mempool_util_insertchunk(m, r, rsize);
        }
        return mempool_util_chunk2mem(v);
    }
    return NULL;
}

/* allocate a small request from the best fitting chunk in a treebin */
MEMPOOL_INLINE void* mempool_util_tmallocsmall(MempoolState* m, size_t nb)
{
    size_t trem;
    size_t rsize;
    MempoolTreeChunk* t;
    MempoolTreeChunk* v;
    MempoolPlainChunk* r;
    MempoolBindex i;
    i = mempool_util_nativebitscanforward(m->treemap);
    v = t = *mempool_util_treebinat(m, i);
    rsize = mempool_util_chunksize(t) - nb;
    while((t = mempool_util_leftmostchild(t)) != 0)
    {
        trem = mempool_util_chunksize(t) - nb;
        if(trem < rsize)
        {
            rsize = trem;
            v = t;
        }
    }
    r = mempool_util_chunkplusoffset(v, nb);
    mempool_util_unlinklargechunk(m, v);
    if(rsize < MEMPOOL_MINCHUNKSIZE)
    {
        mempool_util_setinuseandpinuse(v, (rsize + nb));
    }
    else
    {
        mempool_util_setsizeandpinuseofinusechunk(v, nb);
        mempool_util_setsizeand_pinuseoffreechunk(r, rsize);
        mempool_util_replacedv(m, r, rsize);
    }
    return mempool_util_chunk2mem(v);
}

/* ----------------------------------------------------------------------- */

void* mempool_createpool()
{
    size_t msize;
    size_t tsize;
    char* tbase;
    MempoolState* m;
    MempoolPlainChunk* mn;
    MempoolPlainChunk* msp;
    tsize = MEMPOOL_CONST_DEFAULTGRANULARITY;
    mempool_util_initmmap();
    tbase = (char*)(mempool_util_callvirtalloc(tsize));
    if(tbase != MEMPOOL_ON_CMFAIL)
    {
        msize = mempool_util_padrequest(sizeof(MempoolState));
        msp = mempool_util_alignaschunk(tbase);
        m = (MempoolState*)(mempool_util_chunk2mem(msp));
        memset(m, 0, msize);
        msp->head = (msize | MEMPOOL_PINUSE_BIT | MEMPOOL_CINUSE_BIT);
        m->seg.base = tbase;
        m->seg.size = tsize;
        m->release_checks = MEMPOOL_CONST_MAXRELEASECHECKRATE;
        mempool_util_initbins(m);
        mn = mempool_util_nextchunk(mempool_util_mem2chunk(m));
        mempool_util_inittop(m, mn, (size_t)((tbase + tsize) - (char*)mn) - mempool_util_topfootsize());
        return m;
    }
    return NULL;
}

void mempool_destroypool(void* msp)
{
    size_t size;
    char* base;
    MempoolState* ms;
    MempoolSegment* sp;
    ms = (MempoolState*)msp;
    sp = &ms->seg;
    while(sp != 0)
    {
        base = sp->base;
        size = sp->size;
        sp = sp->next;
        mempool_util_callvirtfree(base, size);
    }
    mempool_util_releasetoos();
}

void* mempool_usermalloc(void* msp, size_t nsize)
{
    size_t nb;
    size_t dvs;
    size_t rsize;
    void* mem;
    MempoolBindex i;
    MempoolBindex idx;
    MempoolBinMap leftbits;
    MempoolBinMap smallbits;
    MempoolPlainChunk* p;
    MempoolPlainChunk* r;
    MempoolPlainChunk* b;
    MempoolState* ms;
    ms = (MempoolState*)msp;
    if(nsize <= MEMPOOL_CONST_MAXSMALLREQUEST)
    {
        nb = (nsize < MEMPOOL_REQUESTS_MIN) ? MEMPOOL_MINCHUNKSIZE : mempool_util_padrequest(nsize);
        idx = mempool_util_smallindex(nb);
        smallbits = ms->smallmap >> idx;
        if((smallbits & 0x3) != 0)
        { /* Remainderless fit to a smallbin. */

            idx += ~smallbits & 1; /* Uses next bin if idx empty */
            b = mempool_util_smallbinat(ms, idx);
            p = b->fd;
            mempool_util_unlinkfirstsmallchunk(ms, b, p, idx);
            mempool_util_setinuseandpinuse(p, mempool_util_smallindex2size(idx));
            mem = mempool_util_chunk2mem(p);
            return mem;
        }
        else if(nb > ms->dvsize)
        {
            if(smallbits != 0)
            { /* Use chunk in next nonempty smallbin */
                leftbits = (smallbits << idx) & mempool_util_leftbits(mempool_util_idx2bit(idx));
                i = mempool_util_nativebitscanforward(leftbits);
                b = mempool_util_smallbinat(ms, i);
                p = b->fd;
                mempool_util_unlinkfirstsmallchunk(ms, b, p, i);
                rsize = mempool_util_smallindex2size(i) - nb;
                /* Fit here cannot be remainderless if 4byte sizes */
                if(MEMPOOL_CONST_SIZETSIZE != 4 && rsize < MEMPOOL_MINCHUNKSIZE)
                {
                    mempool_util_setinuseandpinuse(p, mempool_util_smallindex2size(i));
                }
                else
                {
                    mempool_util_setsizeandpinuseofinusechunk(p, nb);
                    r = mempool_util_chunkplusoffset(p, nb);
                    mempool_util_setsizeand_pinuseoffreechunk(r, rsize);
                    mempool_util_replacedv(ms, r, rsize);
                }
                mem = mempool_util_chunk2mem(p);
                return mem;
            }
            else if(ms->treemap != 0 && (mem = mempool_util_tmallocsmall(ms, nb)) != 0)
            {
                return mem;
            }
        }
    }
    else if(nsize >= MEMPOOL_REQUESTS_MAX)
    {
        nb = MEMPOOL_CONST_MAXSIZET; /* Too big to allocate. Force failure (in sys alloc) */
    }
    else
    {
        nb = mempool_util_padrequest(nsize);
        if(ms->treemap != 0 && (mem = mempool_util_tmalloclarge(ms, nb)) != 0)
        {
            return mem;
        }
    }
    if(nb <= ms->dvsize)
    {
        rsize = ms->dvsize - nb;
        p = ms->dv;
        if(rsize >= MEMPOOL_MINCHUNKSIZE)
        { /* split dv */
            r = ms->dv = mempool_util_chunkplusoffset(p, nb);
            ms->dvsize = rsize;
            mempool_util_setsizeand_pinuseoffreechunk(r, rsize);
            mempool_util_setsizeandpinuseofinusechunk(p, nb);
        }
        else
        { /* exhaust dv */
            dvs = ms->dvsize;
            ms->dvsize = 0;
            ms->dv = 0;
            mempool_util_setinuseandpinuse(p, dvs);
        }
        mem = mempool_util_chunk2mem(p);
        return mem;
    }
    else if(nb < ms->topsize)
    { /* Split top */
        rsize = ms->topsize -= nb;
        p = ms->top;
        r = ms->top = mempool_util_chunkplusoffset(p, nb);
        r->head = rsize | MEMPOOL_PINUSE_BIT;
        mempool_util_setsizeandpinuseofinusechunk(p, nb);
        mem = mempool_util_chunk2mem(p);
        return mem;
    }
    return mempool_util_allocsys(ms, nb);
}

void* mempool_userfree(void* msp, void* ptr)
{
    size_t psize;
    size_t prevsize;
    size_t tsize;
    size_t dsize;
    size_t nsize;
    MempoolState* fm;
    MempoolTreeChunk* tp;
    MempoolPlainChunk* p;
    MempoolPlainChunk* prev;
    MempoolPlainChunk* next;
    if(ptr != 0)
    {
        p = mempool_util_mem2chunk(ptr);
        fm = (MempoolState*)msp;
        psize = mempool_util_chunksize(p);
        next = mempool_util_chunkplusoffset(p, psize);
        if(!mempool_util_pinuse(p))
        {
            prevsize = p->prev_foot;
            if((prevsize & MEMPOOL_ISDIRECTBIT) != 0)
            {
                prevsize &= ~MEMPOOL_ISDIRECTBIT;
                psize += prevsize + MEMPOOL_DIRECTFOOTPAD;
                mempool_util_callvirtfree((char*)p - prevsize, psize);
                return NULL;
            }
            else
            {
                prev = mempool_util_chunkminusoffset(p, prevsize);
                psize += prevsize;
                p = prev;
                /* consolidate backward */
                if(p != fm->dv)
                {
                    mempool_util_unlinkchunk(fm, p, prevsize);
                }
                else if((next->head & MEMPOOL_INUSE_BITS) == MEMPOOL_INUSE_BITS)
                {
                    fm->dvsize = psize;
                    mempool_util_setfreewithpinuse(p, psize, next);
                    return NULL;
                }
            }
        }
        if(!mempool_util_cinuse(next))
        { /* consolidate forward */
            if(next == fm->top)
            {
                tsize = fm->topsize += psize;
                fm->top = p;
                p->head = tsize | MEMPOOL_PINUSE_BIT;
                if(p == fm->dv)
                {
                    fm->dv = 0;
                    fm->dvsize = 0;
                }
                if(tsize > fm->trim_check)
                {
                    mempool_util_alloctrim(fm, 0);
                }
                return NULL;
            }
            else if(next == fm->dv)
            {
                dsize = fm->dvsize += psize;
                fm->dv = p;
                mempool_util_setsizeand_pinuseoffreechunk(p, dsize);
                return NULL;
            }
            else
            {
                nsize = mempool_util_chunksize(next);
                psize += nsize;
                mempool_util_unlinkchunk(fm, next, nsize);
                mempool_util_setsizeand_pinuseoffreechunk(p, psize);
                if(p == fm->dv)
                {
                    fm->dvsize = psize;
                    return NULL;
                }
            }
        }
        else
        {
            mempool_util_setfreewithpinuse(p, psize, next);
        }

        if(mempool_util_issmall(psize))
        {
            mempool_util_insertsmallchunk(fm, p, psize);
        }
        else
        {
            tp = (MempoolTreeChunk*)p;
            mempool_util_insertlargechunk(fm, tp, psize);
            if(--fm->release_checks == 0)
            {
                mempool_util_releaseunusedsegments(fm);
            }
        }
    }
    return NULL;
}

void* mempool_userrealloc(void* msp, void* ptr, size_t nsize)
{
    size_t nb;
    size_t oc;
    size_t rsize;
    size_t oldsize;
    size_t newsize;
    size_t newtopsize;
    void* newmem;
    MempoolState* m;
    MempoolPlainChunk* oldp;
    MempoolPlainChunk* next;
    MempoolPlainChunk* newp;
    MempoolPlainChunk* newtop;
    if(ptr == NULL)
    {
        return mempool_usermalloc(msp, nsize);
    }
    if(nsize >= MEMPOOL_REQUESTS_MAX)
    {
        return NULL;
    }
    else
    {
        m = (MempoolState*)msp;
        oldp = mempool_util_mem2chunk(ptr);
        oldsize = mempool_util_chunksize(oldp);
        next = mempool_util_chunkplusoffset(oldp, oldsize);
        newp = 0;
        nb = mempool_util_request2size(nsize);

        /* Try to either shrink or extend into top. Else malloc-copy-free */
        if(mempool_util_isdirect(oldp))
        {
            newp = mempool_util_directresize(oldp, nb); /* this may return NULL. */
        }
        else if(oldsize >= nb)
        { /* already big enough */
            rsize = oldsize - nb;
            newp = oldp;
            if(rsize >= MEMPOOL_MINCHUNKSIZE)
            {
                MempoolPlainChunk* rem = mempool_util_chunkplusoffset(newp, nb);
                mempool_util_setinuse(newp, nb);
                mempool_util_setinuse(rem, rsize);
                mempool_userfree(m, mempool_util_chunk2mem(rem));
            }
        }
        else if(next == m->top && oldsize + m->topsize > nb)
        {
            /* Expand into top */
            newsize = oldsize + m->topsize;
            newtopsize = newsize - nb;
            newtop = mempool_util_chunkplusoffset(oldp, nb);
            mempool_util_setinuse(oldp, nb);
            newtop->head = newtopsize | MEMPOOL_PINUSE_BIT;
            m->top = newtop;
            m->topsize = newtopsize;
            newp = oldp;
        }

        if(newp != 0)
        {
            return mempool_util_chunk2mem(newp);
        }
        else
        {
            newmem = mempool_usermalloc(m, nsize);
            if(newmem != 0)
            {
                oc = oldsize - mempool_util_overheadfor(oldp);
                memcpy(newmem, ptr, oc < nsize ? oc : nsize);
                mempool_userfree(m, ptr);
            }
            return newmem;
        }
    }
}


#endif
