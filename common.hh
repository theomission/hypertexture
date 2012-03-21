#pragma once

#define CARRAY_SIZE(ary) (sizeof(ary)/sizeof(ary[0]))

template<class T, int N>
char (&array_size_helper(T (&)[N]))[N];
#define ARRAY_SIZE(ary) (sizeof(array_size_helper((ary))))

#define DEBUGBREAK() __asm__("int $3") ;
//#define DEBUGBREAK() __debugbreak();

#ifdef DEBUG
#define ASSERT(condition) do { if(!(condition)) { DEBUGBREAK() } } while(0)
#else
#define ASSERT(condition) do { (void)sizeof(condition); } while(0)
#endif

