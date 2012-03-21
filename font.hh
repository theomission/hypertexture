#pragma once

class Color;

int font_Init();
void font_Print(float x, float y, const char* str, const Color& color, float size);
void font_GetDims(const char* str, float size, float *width, float *height);

