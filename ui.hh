#pragma once

#include "commonmath.hh"

void ui_Init();
void ui_DrawColoredQuad(float x, float y, float w, float h, float border, 
	const Color& color, const Color& borderColor = Color(1,1,1));
void ui_DrawQuad(float x, float y, float w, float h, float border);

