#include "frame.hh"
#include <cstring>
#include "framemem.hh"

Framedata* frame_New()
{
	Framedata* frame = (Framedata*)framemem_Alloc(sizeof(Framedata));
	bzero(frame, sizeof(*frame));
	return frame;
}

