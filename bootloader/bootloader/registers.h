#ifndef REGISTER_UTIL_H
#define REGISTER_UTIL_H

#include "stdint.h"
#include "bdefs.h"


// 
// checks if a specifc bit in a given value is 1
//
#define BIT_IS_SET( val, bit ) ( (( (ULONG64)(val) ) & ( (ULONG64)(bit) )) != 0 )

//
// sets a specific bit in given value to 1
//
#define BIT_SET( val, bit ) ( ( (ULONG64)(val) ) |= ( (ULONG64)(bit) ) )

//
// clears a specific bit in given value to 0
//
#define BIT_CLEAR( val, bit ) ( ( (ULONG64)(val) ) &= ~( (ULONG64)(bit) )  )

#endif

