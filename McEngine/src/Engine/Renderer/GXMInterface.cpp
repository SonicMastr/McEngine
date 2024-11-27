//================ Copyright (c) 2024, PG, All rights reserved. =================//
//
// Purpose:		raw GXM graphics interface
//
// $NoKeywords: $GXMi
//===============================================================================//

#include "GXMInterface.h"

#ifdef MCENGINE_FEATURE_GXM

#include "Engine.h"
#include "ConVar.h"
#include "Camera.h"

#include "VisualProfiler.h"
#include "ResourceManager.h"

/*****************************************************************************
                        Regular Display Callback
 ****************************************************************************/

void displayCallback(const void *callbackData)
{
    const struct GXMInterface::DisplayCallbackData *cbData = (const GXMInterface::DisplayCallbackData *)callbackData;
    SceDisplayFrameBuf fb;
    memset(&fb, 0, sizeof(SceDisplayFrameBuf));
    fb.size = sizeof(SceDisplayFrameBuf);
    fb.base = cbData->addr;
    fb.pitch = DISPLAY_STRIDE_IN_PIXELS;
    fb.pixelformat = DISPLAY_PIXEL_FORMAT;
    fb.width = DISPLAY_WIDTH;
    fb.height = DISPLAY_HEIGHT;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);

    if (cbData->vblank)
    {
        sceDisplayWaitVblankStart();
    }
}

/*****************************************************************************
                    Shader Patcher Allocation Callbacks
 ****************************************************************************/
static void *patcherHostAlloc(void *userData, uint32_t size)
{
	return malloc(size);
}

static void patcherHostFree(void *userData, void *mem)
{
	free(mem);
}

static void *patcherBufferAlloc(void *userData, uint32_t size)
{
	return ((GXMInterface*)engine->getGraphics())->heapAlloc(GXMInterface::HEAP_TYPE_LPDDR_RW, size, 4);
}

static void *patcherVertexUsseAlloc(void *userData, uint32_t size, uint32_t *usseOffset)
{
	return ((GXMInterface*)engine->getGraphics())->heapAlloc(GXMInterface::HEAP_TYPE_VERTEX_USSE, size, 16, usseOffset);
}

static void *patcherFragmentUsseAlloc(void *userData, uint32_t size, uint32_t *usseOffset)
{
	return ((GXMInterface*)engine->getGraphics())->heapAlloc(GXMInterface::HEAP_TYPE_FRAGMENT_USSE, size, 16, usseOffset);
}

static void patcherFree(void *userData, void *mem)
{
	((GXMInterface*)engine->getGraphics())->heapFree(mem);
}

/*****************************************************************************
                        GXM Initialization Helpers
 ****************************************************************************/

SceUID GXMInterface::initGxmLib(void)
{
    SceGxmInitializeParams gxmInitParams;
    memset(&gxmInitParams, 0, sizeof(SceGxmInitializeParams));
    gxmInitParams.flags = 0;
    gxmInitParams.displayQueueMaxPendingCount = 3;
    gxmInitParams.displayQueueCallback = displayCallback;
    gxmInitParams.displayQueueCallbackDataSize = sizeof(DisplayCallbackData);
    gxmInitParams.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;
    return sceGxmInitialize(&gxmInitParams);
}

SceUID GXMInterface::initGxmMem(void)
{
    heapInitialize();

    m_lpddrUid = sceKernelAllocMemBlock("GXM LPDDR", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, ALIGN(HEAP_SIZE_LPDDR_R + HEAP_SIZE_LPDDR_RW + HEAP_SIZE_VERTEX_USSE + HEAP_SIZE_FRAGMENT_USSE, 4 * 1024), NULL);
    if (m_lpddrUid < 0)
    {
        heapTerminate();
        return m_lpddrUid;
    }
    m_cdramUid = sceKernelAllocMemBlock("GXM CDRAM", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, ALIGN(HEAP_SIZE_CDRAM_RW, 256 * 1024), NULL);
    if (m_cdramUid < 0)
    {
        heapTerminate();
        return m_cdramUid;
    }
    m_phycontUid = sceKernelAllocMemBlock("GXM PHYCONT", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW, ALIGN(HEAP_SIZE_PHYCONT_RW, 1024 * 1024), NULL);
    if (m_phycontUid < 0)
    {
        heapTerminate();
        return m_phycontUid;
    }

    void *lpddrMemBase = NULL;
	void *cdramMemBase = NULL;
    void *phycontMemBase = NULL;
    if (sceKernelGetMemBlockBase(m_lpddrUid, &lpddrMemBase) < 0)
    {
        heapTerminate();
        return -1;
    }
    if (sceKernelGetMemBlockBase(m_cdramUid, &cdramMemBase) < 0)
    {
        heapTerminate();
        return -1;
    }
    if (sceKernelGetMemBlockBase(m_phycontUid, &phycontMemBase) < 0)
    {
        heapTerminate();
        return -1;
    }

    uint8_t *lpddrMem = (uint8_t *)lpddrMemBase;
    uint8_t *cdramMem = (uint8_t *)cdramMemBase;
    uint8_t *phycontMem = (uint8_t *)phycontMemBase;

    uint32_t usseOffset = 0;

    // LPDDR R
    if (sceGxmMapMemory(lpddrMem, HEAP_SIZE_LPDDR_R, SCE_GXM_MEMORY_ATTRIB_READ) != SCE_OK)
    {
        heapTerminate();
        return -1;
    }
    heapExtend(HEAP_TYPE_LPDDR_R, lpddrMem, HEAP_SIZE_LPDDR_R);
    lpddrMem += HEAP_SIZE_LPDDR_R;

    // LPDDR RW
    if (sceGxmMapMemory(lpddrMem, HEAP_SIZE_LPDDR_RW, SCE_GXM_MEMORY_ATTRIB_RW) != SCE_OK)
    {
        heapTerminate();
        return -1;
    }
    heapExtend(HEAP_TYPE_LPDDR_RW, lpddrMem, HEAP_SIZE_LPDDR_RW);
    lpddrMem += HEAP_SIZE_LPDDR_RW;

    // CDRAM
    if (sceGxmMapMemory(cdramMem, HEAP_SIZE_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_RW) != SCE_OK)
    {
        heapTerminate();
        return -1;
    }
    heapExtend(HEAP_TYPE_CDRAM_RW, cdramMem, HEAP_SIZE_CDRAM_RW);
    cdramMem += HEAP_SIZE_CDRAM_RW;

    // PHYCONT
    if (sceGxmMapMemory(phycontMem, HEAP_SIZE_PHYCONT_RW, SCE_GXM_MEMORY_ATTRIB_RW) != SCE_OK)
    {
        heapTerminate();
        return -1;
    }
    heapExtend(HEAP_TYPE_PHYCONT_RW, phycontMem, HEAP_SIZE_PHYCONT_RW);
    phycontMem += HEAP_SIZE_PHYCONT_RW;

    // Vertex USSE
    if (sceGxmMapVertexUsseMemory(lpddrMem, HEAP_SIZE_VERTEX_USSE, &usseOffset) != SCE_OK)
    {
        heapTerminate();
        return -1;
    }
    heapExtend(HEAP_TYPE_VERTEX_USSE, lpddrMem, HEAP_SIZE_VERTEX_USSE, usseOffset);
    lpddrMem += HEAP_SIZE_VERTEX_USSE;

    // Fragment USSE
    if (sceGxmMapFragmentUsseMemory(lpddrMem, HEAP_SIZE_FRAGMENT_USSE, &usseOffset) != SCE_OK)
    {
        heapTerminate();
        return -1;
    }
    heapExtend(HEAP_TYPE_FRAGMENT_USSE, lpddrMem, HEAP_SIZE_FRAGMENT_USSE, usseOffset);
    lpddrMem += HEAP_SIZE_FRAGMENT_USSE;

    return 0;
}

SceUID GXMInterface::initGxmImmediateContext(void)
{
    m_contextHostMemBuffer = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);

    unsigned int fragmentUsseOffset;
    m_vdmRingBuffer = heapAlloc(HEAP_TYPE_LPDDR_R, SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE, 4);
    m_vertexRingBuffer = heapAlloc(HEAP_TYPE_LPDDR_R, SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE, 4);
    m_fragmentRingBuffer = heapAlloc(HEAP_TYPE_LPDDR_R, SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE, 4);
    m_fragmentUsseRingBuffer = heapAlloc(HEAP_TYPE_FRAGMENT_USSE, SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE, 4, &fragmentUsseOffset);

    SceGxmContextParams contextParams;
    memset(&contextParams, 0, sizeof(SceGxmContextParams));
    contextParams.hostMem = m_contextHostMemBuffer;
    contextParams.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
    contextParams.vdmRingBufferMem = m_vdmRingBuffer;
    contextParams.vdmRingBufferMemSize = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
    contextParams.vertexRingBufferMem = m_vertexRingBuffer;
    contextParams.vertexRingBufferMemSize = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
    contextParams.fragmentRingBufferMem = m_fragmentRingBuffer;
    contextParams.fragmentRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
    contextParams.fragmentUsseRingBufferMem = m_fragmentUsseRingBuffer;
    contextParams.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
    contextParams.fragmentUsseRingBufferOffset = fragmentUsseOffset;

    return sceGxmCreateContext(&contextParams, &m_context);
}

SceUID GXMInterface::initGxmShaderPatcher(void)
{
    SceGxmShaderPatcherParams patcherParams;
    memset(&patcherParams, 0, sizeof(SceGxmShaderPatcherParams));
    patcherParams.userData = NULL;
    patcherParams.hostAllocCallback = &patcherHostAlloc;
    patcherParams.hostFreeCallback = &patcherHostFree;
    patcherParams.bufferAllocCallback = &patcherBufferAlloc;
    patcherParams.bufferFreeCallback = &patcherFree;
    patcherParams.vertexUsseAllocCallback = &patcherVertexUsseAlloc;
    patcherParams.vertexUsseFreeCallback = &patcherFree;
    patcherParams.fragmentUsseAllocCallback = &patcherFragmentUsseAlloc;
    patcherParams.fragmentUsseFreeCallback = &patcherFree;

    return sceGxmShaderPatcherCreate(&patcherParams, &m_shaderPatcher);
}
/*****************************************************************************
                            Actual GXM Stuff :P
 ****************************************************************************/

GXMInterface::GXMInterface() : NullGraphicsInterface()
{
    m_bReady = false;

    m_vResolution = engine->getScreenSize(); // initial viewport size = window size

}

GXMInterface::~GXMInterface()
{
    //SAFE_DELETE(m_shaderTexturedGeneric);
}

void GXMInterface::init()
{

    if (initGxmLib() != SCE_OK)
    {
        engine->showMessageErrorFatal("GXM Error", "sceGxmInitialize() failed");
        engine->shutdown();
        return;
    }

    if (initGxmMem() != SCE_OK)
    {
        engine->showMessageErrorFatal("GXM Error", "Memory Initialization failed");
        engine->shutdown();
        return;
    }

    if (initGxmImmediateContext() != SCE_OK)
    {
        engine->showMessageErrorFatal("GXM Error", "sceGxmCreateContext() failed");
        engine->shutdown();
        return;
    }

    if (initGxmShaderPatcher() != SCE_OK)
    {
        engine->showMessageErrorFatal("GXM Error", "sceGxmShaderPatcherCreate() failed");
        engine->shutdown();
        return;
    }

    // Default Render Targets Next


    m_bReady = true;
}

void GXMInterface::beginScene()
{
    // TODO: Implement
}

void GXMInterface::endScene()
{
    // TODO: Implement
}

/*****************************************************************************
                            Heap Management
 ****************************************************************************/

GXMInterface::HeapBlock *GXMInterface::heapBlockAlloc(void)
{
	return (HeapBlock *)malloc(sizeof(HeapBlock));
}

void GXMInterface::heapBlockFree(HeapBlock *block)
{
	free(block);
}

bool GXMInterface::heapBlockCanMerge(HeapBlock *a, HeapBlock *b)
{
	return a->type				== b->type
		&& a->base + a->size	== b->base
		&& a->offset + a->size	== b->offset;
}

void GXMInterface::heapInsertFreeBlock(HeapContext *ctx, HeapBlock *block)
{
	HeapBlock *curr = ctx->freeList;
	HeapBlock *prev = NULL;
	while (curr && curr->base < block->base) 
    {
		prev = curr;
		curr = curr->next;
	}

	if (prev)
		prev->next = block;
    else
		ctx->freeList = block;

	block->next = curr;

	if (curr && heapBlockCanMerge(block, curr)) 
    {
		block->size += curr->size;
		block->next = curr->next;
		heapBlockFree(curr);
	}

	if (prev  && heapBlockCanMerge(prev, block)) 
    {
		prev->size += block->size;
		prev->next = block->next;
		heapBlockFree(block);
	}
}

GXMInterface::HeapBlock *GXMInterface::heapAllocBlock(HeapContext *ctx, int32_t type, uint32_t size, uint32_t alignment)
{
	// find a suitable block in the free list
	HeapBlock *curr = ctx->freeList;
	HeapBlock *prev = NULL;
	while (curr) 
    {
		// check this block can handle alignment and size
		uint32_t const skip = ALIGN(curr->base, alignment) - curr->base;
		if (curr->type == type && skip + size <= curr->size) 
        {
			// allocate any blocks we need now to avoid complicated rollback
			HeapBlock *skipBlock = NULL;
			HeapBlock *unusedBlock = NULL;
			if (skip != 0) 
            {
				skipBlock = heapBlockAlloc();
				if (!skipBlock)
					return NULL;
			}
			if (skip + size != curr->size) 
            {
				unusedBlock = heapBlockAlloc();
				if (!unusedBlock) 
                {
					if (skipBlock)
						heapBlockFree(skipBlock);
					return NULL;
				}
			}

			// add block for skipped memory
			if (skip != 0) 
            {
				// link in
				if (prev)
					prev->next = skipBlock;
				else
					ctx->freeList = skipBlock;
				skipBlock->next = curr;

				// set sizes
				skipBlock->type = curr->type;
				skipBlock->base = curr->base;
				skipBlock->offset = curr->offset;
				skipBlock->size = skip;
				curr->base += skip;
				curr->offset += skip;
				curr->size -= skip;

				// update prev
				prev = skipBlock;
			}

			// add block for unused memory
			if (size != curr->size) 
            {
				// link in
				unusedBlock->next = curr->next;
				curr->next = unusedBlock;

				// set sizes
				unusedBlock->type = curr->type;
				unusedBlock->base = curr->base + size;
				unusedBlock->offset = curr->offset + size;
				unusedBlock->size = curr->size - size;
				curr->size = size;
			}

			// unlink from free list
			if (prev)
				prev->next = curr->next;
			else
				ctx->freeList = curr->next;

			// push onto alloc list
			curr->next = ctx->allocList;
			ctx->allocList = curr;
			return curr;
		}

		// advance
		prev = curr;
		curr = curr->next;
	}

	// no block found
	return NULL;
}

void GXMInterface::heapFreeBlock(HeapContext *ctx, uintptr_t base)
{
	// find in the allocate block list
	HeapBlock *curr = ctx->allocList;
	HeapBlock *prev = NULL;
	while (curr && curr->base != base) {
		prev = curr;
		curr = curr->next;
	}

	// early out if not found
	if (!curr)
		return;

	// unlink from allocated list
	if (prev) {
		prev->next = curr->next;
	} else {
		ctx->allocList = curr->next;
	}
	curr->next = NULL;

	// add as free block
	heapInsertFreeBlock(&m_heapContext, curr);
}

void GXMInterface::heapInitialize()
{
	m_heapContext.allocList = NULL;
	m_heapContext.freeList = NULL;
}

void GXMInterface::heapTerminate()
{

}

void GXMInterface::heapExtend(int32_t type, void *base, uint32_t size, uint32_t offset)
{
	HeapBlock *block = heapBlockAlloc();
	block->next = NULL;
	block->type = type;
	block->base = (uintptr_t)base;
	block->offset = offset;
	block->size = size;
	heapInsertFreeBlock(&m_heapContext, block);
}

void *GXMInterface::heapAlloc(int32_t type, uint32_t size, uint32_t alignment, uint32_t *offset)
{
	// try to allocate
	HeapBlock *block = heapAllocBlock(&m_heapContext, type, size, alignment);

	// early out if failed
	if (!block)
		return NULL;

	// write offset and return base address
	if (offset) {
		*offset = block->offset;
	}
	return (void *)block->base;
}

void GXMInterface::heapFree(void *addr)
{
	heapFreeBlock(&m_heapContext, (uintptr_t)addr);
}

#endif