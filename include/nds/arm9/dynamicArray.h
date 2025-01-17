// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Jason Rogers (dovoto)

// A simple vector like dynamic data structure

/*! \file dynamicArray.h
	\brief A dynamically resizing array for general use.
*/

#ifndef __DYNAMICARRAY_H__
#define __DYNAMICARRAY_H__

#include <stdlib.h>
#include <string.h>

#include <nds/ndstypes.h>


//! A resizable array
typedef struct DynamicArray
{
	void** data; 			//!< pointer to array of void pointers
	unsigned int cur_size; 	//!< currently allocated size of the array
}DynamicArray;



/*! \brief Initializes an array with the supplied initial size
	\param v the array to initialize
	\param initialSize the initial size to allocate
	\return a pointer to the data, or NULL on error.
*/
void* DynamicArrayInit(DynamicArray* v, unsigned int initialSize);


/*! \brief Frees memory allocated by the dynamic array
	\param v The array to delete
*/
void DynamicArrayDelete(DynamicArray* v);


/*! \brief Gets the entry at the supplied index
	\param v The array to get from.
	\param index The index of the data to get.
	\return The data or NULL if v is NULL or the index is out of range.
*/
void* DynamicArrayGet(DynamicArray* v, unsigned int index);


/*! \brief Sets the entry to the supplied value
	\param v The array to set
	\param index The index of the data to set (array will be resized to fit the index).
	\param item The data to set.
	\return false if v is NULL or there isn't enough memory, true otherwise
*/
bool DynamicArraySet(DynamicArray *v, unsigned int index, void* item);


#endif
