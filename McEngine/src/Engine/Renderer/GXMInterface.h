//================ Copyright (c) 2024, PG, All rights reserved. =================//
//
// Purpose:		raw GXM graphics interface
//
// $NoKeywords: $GXMi
//===============================================================================//

#ifndef GXMINTERFACE_H
#define GXMINTERFACE_H

#include "cbase.h"
#include "NullGraphicsInterface.h"

#ifdef MCENGINE_FEATURE_GXM

#include <psp2/kernel/processmgr.h>
#include <psp2/appmgr.h>
#include <psp2/gxm.h>
#include <psp2/display.h>
#include <psp2/types.h>
#include <psp2/kernel/sysmem.h>

#define ALIGN(x, a) (((x) + ((a)-1)) & ~((a)-1))

// Heap sizes
#define HEAP_SIZE_LPDDR_R			(64*1024*1024)
#define HEAP_SIZE_LPDDR_RW			(32*1024*1024)
#define HEAP_SIZE_PHYCONT_RW		(20*1024*1024)
#define HEAP_SIZE_CDRAM_RW			(32*1024*1024)
#define HEAP_SIZE_VERTEX_USSE		(1*1024*1024)
#define HEAP_SIZE_FRAGMENT_USSE		(1*1024*1024)

#define DISPLAY_WIDTH				960
#define DISPLAY_HEIGHT				544
#define DISPLAY_STRIDE_IN_PIXELS	1024

#define DISPLAY_COLOR_FORMAT		SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_PIXEL_FORMAT		SCE_DISPLAY_PIXELFORMAT_A8B8G8R8
#define DISPLAY_DBGFONT_FORMAT		SCE_DBGFONT_PIXELFORMAT_A8B8G8R8
#define DISPLAY_BUFFER_COUNT		3
#define DISPLAY_MAX_PENDING_SWAPS	2

class GXMShader;

class GXMInterface : public NullGraphicsInterface
{
public:
    struct DisplayCallbackData 
    {
        void *addr;
        uint8_t vblank;
    };

    enum HeapType {
        HEAP_TYPE_LPDDR_R,
        HEAP_TYPE_LPDDR_RW,
        HEAP_TYPE_CDRAM_RW,
        HEAP_TYPE_PHYCONT_RW,
        HEAP_TYPE_VERTEX_USSE,
        HEAP_TYPE_FRAGMENT_USSE,
    };

public:
    GXMInterface();
    virtual ~GXMInterface();

    virtual void beginScene();
    virtual void endScene();

    inline SceGxmContext *getContext() { return m_context; }

    // Illegal
    void heapInitialize();
    void heapTerminate();
    void heapExtend(int32_t type, void *base, uint32_t size, uint32_t offset = 0);
    void *heapAlloc(int32_t type, uint32_t size, uint32_t alignment = 1, uint32_t *offset = NULL);
    void heapFree(void *mem);

protected:
    virtual void init();

private:

    bool m_bReady;
    Vector2 m_vResolution;

    SceGxmContext *m_context;

    // GXM Buffers
    void *m_contextHostMemBuffer;
    void *m_vdmRingBuffer;
    void *m_vertexRingBuffer;
    void *m_fragmentRingBuffer;
    void *m_fragmentUsseRingBuffer;
    void *m_patcherBuffer;
    void *m_patcherVertexUsse;
    void *m_patcherFragmentUsse;

    SceUID m_vdmRingBufferUid;
    SceUID m_vertexRingBufferUid;
    SceUID m_fragmentRingBufferUid;
    SceUID m_fragmentUsseRingBufferUid;
    SceUID m_patcherBufferUid;
    SceUID m_patcherVertexUsseUid;
    SceUID m_patcherFragmentUsseUid;

    SceGxmShaderPatcher *m_shaderPatcher;

    SceUID m_lpddrUid;
    SceUID m_cdramUid;
    SceUID m_phycontUid;

    SceUID initGxmLib();
    SceUID initGxmMem();
    SceUID initGxmImmediateContext();
    SceUID initGxmShaderPatcher();


    // Graphics Heap Management
    struct HeapBlock
    {
        HeapBlock *next;
        int32_t type;
        uintptr_t base;
        uint32_t offset;
        uint32_t size;
    };

    struct HeapContext
    {
        HeapBlock *allocList;
        HeapBlock *freeList;
    };

    HeapContext m_heapContext;

    HeapBlock *heapBlockAlloc();
    void heapBlockFree(HeapBlock *block);
    bool heapBlockCanMerge(HeapBlock *a, HeapBlock *b);
    void heapInsertFreeBlock(HeapContext *ctx, HeapBlock *block);
    HeapBlock *heapAllocBlock(HeapContext *ctx, int32_t type, uint32_t size, uint32_t alignment);
    void heapFreeBlock(HeapContext *ctx, uintptr_t base);
};


#endif

#endif