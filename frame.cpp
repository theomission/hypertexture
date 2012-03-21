#include "frame.hh"
#include <cstring>
#include "framemem.hh"
#include "planet.hh"

Framedata* frame_New()
{
	Framedata* frame = (Framedata*)framemem_Alloc(sizeof(Framedata));
	bzero(frame, sizeof(*frame));
	return frame;
}

void frame_AppendTile(Framedata& frame, PlanetTile* tilePtr)
{
	if(frame.m_tilesMax == frame.m_tilesNum)
	{
		int newMax = frame.m_tilesMax > 0 ? frame.m_tilesMax + 1024 : 4096;
		const PlanetTile** newAry = 
			(const PlanetTile**)framemem_Alloc(sizeof(PlanetTile*) * newMax);
		bzero(newAry, sizeof(PlanetTile*)*frame.m_tilesMax);
		if(frame.m_tiles)
			memcpy(newAry, frame.m_tiles, sizeof(PlanetTile*) * frame.m_tilesNum);
		frame.m_tilesMax = newMax;
		frame.m_tiles = newAry;
	}

	int idx = frame.m_tilesNum++;
	frame.m_tiles[idx] = tilePtr;
}

