#pragma once

#include <cstring>

class PoolAlloc
{
public:
	PoolAlloc(int itemsPerBlock, int itemSize);
	~PoolAlloc();
	PoolAlloc(const PoolAlloc&) = delete;
	PoolAlloc& operator=(const PoolAlloc&) = delete;

	void* Alloc();
	void Free(void*);
private:
	class BlockItem {
	public:
		BlockItem* m_next;
	};
	class Block { 
	public:
		Block* m_next;
		BlockItem* First() { 
			constexpr int blockSize = (sizeof(Block)+15) & ~15;
			return reinterpret_cast<BlockItem*>(
				reinterpret_cast<char*>(this) + blockSize);
		}
	};

	Block* AllocBlock();

	int m_itemsPerBlock;
	int m_itemSize;
	Block* m_block;
	BlockItem* m_next;
};

