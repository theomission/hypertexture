#include <cstdio>
#include <cstdlib>
#include "framemem.hh"

#define FRAMEBLOCK_SIZE (1024*1024)

struct FrameBlock {
	unsigned int m_size;
	unsigned int m_used;
	struct FrameBlock* m_next;
};

#define BLOCKDATA(p) ((char*)(p) + sizeof(struct FrameBlock))

struct FrameBlock *g_frameBlockHead;
struct FrameBlock *g_frameBlockCur;

void framemem_Init()
{
	g_frameBlockHead = (struct FrameBlock*)malloc(sizeof(struct FrameBlock) + FRAMEBLOCK_SIZE);
	g_frameBlockHead->m_size = FRAMEBLOCK_SIZE;
	g_frameBlockHead->m_used = 0;
	g_frameBlockHead->m_next = 0;
	g_frameBlockCur = g_frameBlockHead;
}

void framemem_Clear()
{
	struct FrameBlock* cur = g_frameBlockHead;
	while(cur) {
		cur->m_used = 0;
		cur = cur->m_next;
	}
}

void* framemem_Alloc(unsigned int size)
{
	struct FrameBlock* block = g_frameBlockCur;

	size = (size + 0xF) & (~0xF);
	if(size > block->m_size) {
		printf("frame alloc of %d can never be satisfied\n", size);
		exit(1);
	}

	unsigned int sizeLeft = block->m_size - block->m_used;
	if(size < sizeLeft)
	{
		char* result = BLOCKDATA(block) + block->m_used;
		block->m_used += size;
		return result;
	}

	block = block->m_next;
	if(!block)
	{
		block = (struct FrameBlock*)malloc(sizeof(struct FrameBlock) + FRAMEBLOCK_SIZE);
		block->m_size = FRAMEBLOCK_SIZE;
		block->m_used = 0;
		block->m_next = 0;
		g_frameBlockCur->m_next = block;
	}
	g_frameBlockCur = block;
	block->m_used += size;
	return BLOCKDATA(block);
}

