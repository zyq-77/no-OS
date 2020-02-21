/***************************************************************************//**
 *   @file   list.h
 *   @brief  List library header
 *   @author Mihail Chindris (mihail.chindris@analog.com)
********************************************************************************
 *   @copyright
 * Copyright 2020(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************************
 *
 *  @section details Library description
 *   This library handles double linked lists and it expose inseart,
 *   read, get and delete functions. \n
 *   The list also can be accessed using an
 *   adapter or an interator. \n
 *  @subsection example Sample code
 *   @code{.c}
 *	List_desc		list1, list2;
 *	Iterator  		it;
 *	struct list_adapter	*ad;
 *
 *	list_init(&list1, NULL, NULL, LIST_DEFAULT);
 *	list_init(&list2, NULL, &ad, LIST_STACK);
 *
 *	// Using library functions
 *	list_add_last(list1, 1);
 *	list_add_last(list1, 2);
 *	printf("Last: %d\n", list_read_last(list1, NULL));
 *
 *	// Using adaptor
 *	ad->push(ad->list, 1);
 *	ad->push(ad->list, 2);
 *	printf("Last: %d\n", ad->top_next(ad->list, NULL));
 *
 *	// Using iterator
 *	iterator_init(&it, list1, 0);
 *	iterator_move(it, -1);
 *	printf("Last: %d\n", iterator_read(it, NULL));
 *
 *	iterator_remove(it);
 *	list_remove(list1);
 *	list_remove(list2);
 *    @endcode
*******************************************************************************/

#ifndef LIST_H
#define LIST_H

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/******************************************************************************/
/*************************** Types Declarations *******************************/
/******************************************************************************/

/** @brief Reference of an iterator on a list */
typedef void* Iterator;

/** @brief Reference to a created list */
typedef void* List_desc;

/**
 * @brief Prototype of the compare function.
 *
 * The function used to compare the elements of the liste when doing
 * operations on an ordered list.
 * @param data1 - First element to be compared
 * @param data2 - Second element to be compared
 * @return
 *  - -1 - If data1 < data2
 *  - 0 - If data1 == data2
 *  - 1 - If data1 > data2
 */
typedef int32_t (*f_cmp)(void *data1, void *data2);

/**
 * @name Generic functions
 * Each function interacting with the list have one of the following formats.\n
 * Aditionaly they may have one more parametere for specific functionalities.\n
 * In the Iterator functions, the list reference is replaced by the iterator's
 * one.
 * @{
 */

/**
 * @brief Add an element in the list.
 *
 * The element of the list is created and the data field is stored in it.
 * @param list_desc - Reference to the list. Created by \ref list_init
 * @param data - Data to store in a list element
 * @return
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 */
typedef int32_t	(*f_add)(List_desc list_desc, void *data);

/**
 * @brief Edit an element in the list. The content is replaced by new_data.
 * @param list_desc - Reference to the list. Created by \ref list_init .
 * @param new_data - New data to replace the old one
 * @return
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 */
typedef int32_t	(*f_edit)(List_desc list_desc, void *new_data);

/**
 * @brief Read an element from the list.
 * @param list_desc - Reference to the list. Created by \ref list_init
 * @param result - If not null, result is filled with:
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 * @return \n
 *  - Content of the list element
 *  - NULL : If some error occure
 * @note If the content of an element can be 0 then the result must be checked
 * to see if the functions has succeded
 */
typedef void*	(*f_read)(List_desc list_desc, int32_t *result);

/**
 * @brief Read and remove an element from the list.
 * @param list_desc - Reference to the list. Created by \ref list_init
 * @param result - If not null, result is filled with:
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 * @return
 *  - Content of the list element
 *  - NULL : If some error occure
 * @note If the content of an element can be 0 then the result must be checked
 * to see if the functions has succeded
 */
typedef void*	(*f_get)(List_desc list_desc, int32_t *result);

/** @} */

/**
 * @enum adapter_type
 * @brief Posibles adapters for the implemented list
 */
enum adapter_type {
	/** Default type. The created adapter will be like for LIST_STACK */
	LIST_DEFAULT,
	/**
	 * Adapter for a LIFO list (Last-in first-out). Elements are inserted in on
	 * end and extracted from the other end.
	 *  - \e Push: Insert element
	 *  - \e Pop: Get next element (Read and remove)
	 *  - \e Top_next: Read next element
	 *  - \e Back: Read first element
	 *  - \e Swap: Edit the content of the next element
	 */
	LIST_QUEUE,
	/**
	 * Adapter for a : FIFO list (First-in first-out). Elements are inserted
	 * and extracted only from the same end.
	 *  - \e Push: Insert element
	 *  - \e Pop: Get top element (Read and remove)
	 *  - \e Top_next: Read top element
	 *  - \e Back: Read bottom element
	 *  - \e Swap: Edit the content of the top element
	 */
	LIST_STACK,
	/**
	 * Adapter for ordered list. The order of element is determinated
	 * usinge the \ref f_cmp.
	 *  - \e Push: Insert element
	 *  - \e Pop: Get lowest element (Read and remove)
	 *  - \e Top_next: Read lowest element
	 *  - \e Back: Read the biggest element
	 *  - \e Swap: Edit the lowest element
	 */
	LIST_PRIORITY_LIST
};

/**
 * @struct list_adapter
 * @brief Adaptor of the list
 *
 * With this structure the funtionalities of specific list types
 * ( \ref adapter_type ) can be accesed with the member of this adapter.
 * For example: adapter->push(adapter->list, my_data);
 */
struct list_adapter {
	/** Refer to \ref adapter_type */
	f_add		push;
	/** Refer to \ref adapter_type */
	f_get		pop;
	/** Refer to \ref adapter_type */
	f_read		top_next;
	/** Refer to \ref adapter_type */
	f_read		back;
	/** Refer to \ref adapter_type */
	f_edit		swap;
	/** Reference of the list */
	List_desc	list;
};

/******************************************************************************/
/************************ Functions Declarations ******************************/
/******************************************************************************/

/**
 * @brief Create a new empty list
 * @param list_desc - Where to store the reference of the new created list
 * @param comparator - Used to compare item when using an ordered list or when
 * using the \em find functions.
 * @param adapter - Where to store the reference for the list adapter. Can be
 * NULL.
 * @param type - Type of adapter to use.
 * @return
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 */
int32_t list_init(List_desc *list_desc, f_cmp comparator,
		  struct list_adapter **adapter, enum adapter_type type);

/**
 * @brief Remove the created list.
 *
 * All its elements will be cleared and the data inside each will be lost. If
 * not all iterators have been removed, the list will not be removed.
 * @param list_desc - Reference to the list
 * @return
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 */
int32_t	list_remove(List_desc list_desc);

/**
 * @brief Get the number of elements inside the list
 * @param list_desc - List reference
 * @param out_size - Where to store the number of elements
 * @return
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 */
int32_t list_get_size(List_desc list_desc, uint32_t *out_size);

/**
 * @name Iterator functions
 * An iterator is used to iterate through the list. For a list can be created
 * any number of iterators. All must be removed before removing a list.
 * @{
 */

/**
 * @brief Create a new iterator
 * @param iter - Where to store the reference for the new iterator
 * @param list_desc - Reference of the list the iterator will be used for
 * @param start - If it is true the iterator will be positioned at the first
 * element of the list, else it will be positioned at the last.
 * @return
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 */
int32_t	iterator_init(Iterator *iter, List_desc list_desc, bool start);

/**
 * @brief Remove the created iterator
 * @param iter - Reference of the iterator
 * @return
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 */
int32_t iterator_remove(Iterator iter);

/**
 * @brief Move the position of the iteration through the list.
 *
 * If the required position is outside the list, the call will fail and the
 * iterator will keep its position.
 * @param iter - Reference of the iterator
 * @param idx - Number of positions to be move. If positive, it will be moved
 * forward, otherwise backwords.
 * @return
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 */
int32_t	iterator_move(Iterator iter, int32_t idx);

/**
 * @brief Place the iterator where cmp_data if found.
 * @param iter - Reference to the iterator
 * @param cmp_data - Data to be found
 * @return
 *  - \ref SUCCESS : On success
 *  - \ref FAILURE : Otherwise
 */
int32_t	iterator_find(Iterator iter, void *cmp_data);

/**
 * @brief Insert an item in the list. Refer to \ref f_add
 * @param iter
 * @param data
 * @param after - If true, the item will be inserted after the current position.
 * Otherwise it will be inserted before.
 */
int32_t	iterator_insert(Iterator iter, void *data, bool after);

/**
 * @brief Replace the data at the current position. Refer to \ref f_edit
 */
int32_t	iterator_edit(Iterator iter, void *new_data);

/**
 * @brief Read the data at the current position. Refer to \ref f_read
 */
void*	iterator_read(Iterator iter, int32_t *result);

/**
 * @brief Read and remove the data at the current position. Refer to \ref f_get.
 *
 * If the current item is the last one, the iterator will be moved to the
 * previous one.
 */
void*	iterator_get(Iterator iter, int32_t *result);
/** @}*/

/**
 * @name Operations on the ends of the list
 * These functions will operate on the first or last element of the list
 * @{
 */
/** @brief Add element at the begining of the list. Refer to \ref f_add */
int32_t list_add_first(List_desc list_desc, void *data);
/** @brief Edit the first element of the list. Refer to \ref f_edit */
int32_t list_edit_first(List_desc list_desc, void *new_data);
/** @brief Read the first element of the list. Refer to \ref f_read */
void*	list_read_first(List_desc list_desc, int32_t *result);
/** @brief Read and delete the first element of the list. Refer to \ref f_get */
void*	list_get_first(List_desc list_desc, int32_t *result);
/** @brief Add element at the end of the list. Refer to \ref f_add */
int32_t list_add_last(List_desc list_desc, void *data);
/** @brief Edit the last element of the list. Refer to \ref f_edit */
int32_t list_edit_last(List_desc list_desc, void *new_data);
/** @brief Read the last element of the list. Refer to \ref f_read */
void*	list_read_last(List_desc list_desc, int32_t *result);
/** @brief Read and delete the last element of the list. Refer to \ref f_get */
void*	list_get_last(List_desc list_desc, int32_t *result);
/** @}*/

/**
 * @name Operations by index
 * These functions use an index to identify the element in the list.
 * @{
 */
/** @brief Add element at the specified idx. Refer to \ref f_add */
int32_t list_add_idx(List_desc list_desc, void *data, uint32_t idx);
/** @brief Edit the element at the specified idx. Refer to \ref f_edit */
int32_t list_edit_idx(List_desc list_desc, void *new_data, uint32_t idx);
/** @brief Read the element at the specified idx. Refer to \ref f_read */
void*	list_read_idx(List_desc list_desc, int32_t *result, uint32_t idx);
/** @brief Read and delete the element at idx. Refer to \ref f_get */
void*	list_get_idx(List_desc list_desc, int32_t *result, uint32_t idx);
/** @}*/

/**
 * @name Operations by comparation
 * These functions the specified \ref f_cmp at \ref list_init to identify for
 * which element the operation is for
 * @{
 */
/** @brief Add element in ascending order. Refer to \ref f_add */
int32_t list_add_find(List_desc list_desc, void *data);
/** @brief Edit the element which match with cmp_data. Refer to \ref f_edit */
int32_t list_edit_find(List_desc list_desc, void *new_data, void *cmp_data);
/** @brief Read the element which match with cmp_data. Refer to \ref f_read */
void*	list_read_find(List_desc list_desc, int32_t *result, void *cmp_data);
/**
 * @brief Read and delete the element which match with cmp_data.
 * Refer to \ref f_get
 */
void*	list_get_find(List_desc list_desc, int32_t *result, void *cmp_data);
/** @}*/

#endif //LIST_H
