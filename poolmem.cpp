#include "poolmem.hh"

////////////////////////////////////////////////////////////////////////////////
PoolAlloc::PoolAlloc(int itemsPerBlock, int itemSize)
	: m_itemsPerBlock(itemsPerBlock)
	, m_itemSize((itemSize+15) & ~15)
	, m_block(nullptr)
	, m_next(nullptr)
{
}

PoolAlloc::~PoolAlloc()
{
	Block* block = m_block;
	while(block) {
		Block* next = block->m_next;
		delete[] (char*)block;
		block = next;
	}
}	

PoolAlloc::Block* PoolAlloc::AllocBlock()
{
	constexpr int blockSize = (sizeof(Block)+15) & ~15;
	char* data = new char[blockSize + m_itemsPerBlock * m_itemSize];
	Block* block = reinterpret_cast<Block*>(data);
	bzero(block, blockSize + m_itemsPerBlock * m_itemSize);
	for(int i = 0; i < m_itemsPerBlock-1; ++i)
	{
		int offset = + blockSize + i * m_itemSize;
		BlockItem *item = reinterpret_cast<BlockItem*>(data + offset );
		item->m_next = reinterpret_cast<BlockItem*>(data + offset + m_itemSize  );
	}
	return block;
}

void* PoolAlloc::Alloc()
{
	if(!m_next)
	{
		Block* block = AllocBlock();
		block->m_next = m_block;
		m_block = block;
		m_next = block->First();
	}

	void* result = reinterpret_cast<void*>(m_next);
	m_next = m_next->m_next;
	return result;
}

void PoolAlloc::Free(void* ptr)
{
	if(!ptr) return;
	BlockItem* item = reinterpret_cast<BlockItem*>(ptr);
	item->m_next = m_next;
	m_next = item;
}

