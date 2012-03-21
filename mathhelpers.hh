#pragma once

#define EPSILON (1e-3f)
#define EPSILON_SQ (1e-3f * 1e-3f)

template<class T> 
inline T Min(const T& l, const T& r) {
	return (l < r ? l : r);
}

template<class T> 
inline T Max(const T& l, const T& r) {
	return (l > r ? l : r);
}

template<class T>
inline T Clamp(const T& v, const T& lo, const T& hi) {
	return Min(Max(v,lo), hi);
}

template<class T>
inline T Lerp(float t, const T& a, const T& b) {
	return (1.f - t) * a + t * b;
}

