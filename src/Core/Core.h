#pragma once

#ifdef NDEBUG
#	define result_assert(expression) expression;
#else
#	define result_assert(expression)                                                              \
		if (!(expression)) *((volatile char*)0) = 0;
#endif