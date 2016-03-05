//------------------------------------------------------------------------------
#pragma once

#include <eight/core/debug.h>

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#ifndef _ASSERT
#define _ASSERT(expr) eiASSERT(expr)
#endif 

#ifndef _ASSERTE
#define _ASSERTE(expr) eiASSERT(expr)
#endif

//#ifndef _ASSERT_EXPR
//#define _ASSERT_EXPR(expr, expr_str) eiASSERTMSG(expr, expr_str)
//#endif

#include <windows.h>

//------------------------------------------------------------------------------
