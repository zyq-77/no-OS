/***************************************************************************//**
 *   @file   circular_buffer.h
 *   @brief  Circular buffer implementation
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
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include "circular_buffer.h"
#include "error.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/******************************************************************************/
/*************************** Types Declarations *******************************/
/******************************************************************************/

/**
 * @struct circular_buffer
 * @brief Circular buffer descriptor
 */
struct circular_buffer {
	/** Size of the buffer in bytes */
	uint32_t	size;
	/** Number of user elements in the buffer */
	uint32_t	nb_elem;
	/** Size of an user element */
	uint32_t	elem_size;
	/** Address to the buffer */
	int8_t		*buff;
	/** Pointer to reading location */
	int8_t		*read_ptr;
	/** Pointer to writing location */
	int8_t		*write_ptr;
	/** Set when the buffer is full */
	bool		full;
};

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

int32_t circular_buffer_init(circular_buffer_t *desc, uint32_t nb_elements,
			     uint32_t element_size)
{
	struct circular_buffer	*ldesc;
	uint32_t		size;

	if (!desc || !element_size || !nb_elements)
		return FAILURE;

	size = nb_elements * element_size;
	if (size < nb_elements || size < element_size)
		return FAILURE; //Integer overflow

	ldesc = (struct circular_buffer*)calloc(1, sizeof(*ldesc));
	if (!ldesc)
		return FAILURE;

	ldesc->buff = calloc(nb_elements, element_size);
	if (!ldesc->buff) {
		free(ldesc);
		return FAILURE;
	}

	ldesc->size = size;
	ldesc->elem_size = element_size;
	ldesc->nb_elem = nb_elements;
	ldesc->read_ptr = ldesc->buff;
	ldesc->write_ptr = ldesc->buff;

	*desc = ldesc;

	return SUCCESS;
}

int32_t circular_buffer_remove(circular_buffer_t *desc)
{
	struct circular_buffer *ldesc = (struct circular_buffer *)desc;

	if (!ldesc)
		return FAILURE;

	if (ldesc->buff)
		free(ldesc->buff);
	free(ldesc);

	return SUCCESS;
}

/* Convert pointer in buffer to offset from the beginning */
static inline uint32_t ptr_to_offset(struct circular_buffer *desc, int8_t *ptr)
{
	return ptr - desc->buff;
}

/* Convert number of elements to size */
static inline uint32_t elements_to_size(struct circular_buffer *desc,
					uint32_t elems)
{
	return elems * desc->elem_size;
}

/* Convert size to number of elements */
static inline uint32_t size_to_elements(struct circular_buffer *desc,
					uint32_t size)
{
	return size / desc->elem_size;
}

/* Get size of the datt written to the buffer */
static inline uint32_t get_unread_size(struct circular_buffer *desc)
{
	uint32_t		size;

	if (desc->full)
		return desc->size;

	if (desc->write_ptr < desc->read_ptr) {
		size = desc->size - ptr_to_offset(desc, desc->read_ptr);
		size += desc->write_ptr - desc->buff;
	} else {
		size = desc->write_ptr - desc->read_ptr;
	}

	return size;
}

int32_t circular_buffer_size(circular_buffer_t *desc, uint32_t *nb_elements)
{
	struct circular_buffer	*ldesc = (struct circular_buffer *)desc;

	if (!ldesc)
		return FAILURE;

	*nb_elements =  size_to_elements(ldesc, get_unread_size(ldesc));

	return SUCCESS;
}

int32_t circular_buffer_write(circular_buffer_t *desc, void *data,
			      uint32_t nb_elements)
{
	struct circular_buffer	*ldesc = (struct circular_buffer *)desc;
	uint32_t		to_write_size;
	uint32_t		size_to_end;

	if (!ldesc)
		return FAILURE;

	if (ldesc->full)
		return FAILURE;

	to_write_size = elements_to_size(ldesc, nb_elements);
	/* Check for overflow */
	if (to_write_size > ldesc->size - get_unread_size(ldesc))
		return FAILURE;


	size_to_end = ldesc->size - ptr_to_offset(ldesc, ldesc->write_ptr);
	if (to_write_size > size_to_end) {
		memcpy(ldesc->write_ptr, data, size_to_end);
		to_write_size -= size_to_end;
		memcpy(ldesc->buff, (uint8_t *)data + size_to_end, to_write_size);
		ldesc->write_ptr = ldesc->buff + to_write_size;
	} else {
		memcpy(ldesc->write_ptr, data, to_write_size);
		ldesc->write_ptr += to_write_size;
	}

	if (ldesc->write_ptr == ldesc->read_ptr)
		ldesc->full = true;

	return SUCCESS;
}


int32_t circular_buffer_read(circular_buffer_t *desc, void *data,
			     uint32_t nb_elements)
{
	struct circular_buffer	*ldesc = (struct circular_buffer *)desc;
	uint32_t		to_read_size;
	uint32_t		size_to_end;

	if (!ldesc)
		return FAILURE;

	to_read_size = elements_to_size(ldesc, nb_elements);
	/* Check for overflow */
	if (to_read_size > get_unread_size(ldesc))
		return FAILURE;

	size_to_end = ldesc->size - ptr_to_offset(ldesc, ldesc->read_ptr);
	if (to_read_size > size_to_end) {
		memcpy(data, ldesc->read_ptr, size_to_end);
		to_read_size -= size_to_end;
		memcpy((int8_t *)data + size_to_end, ldesc->buff, to_read_size);
		ldesc->read_ptr = ldesc->buff + to_read_size;
	} else {
		memcpy(data, ldesc->read_ptr, to_read_size);
		ldesc->read_ptr += to_read_size;
	}

	if (ldesc->full)
		ldesc->full = false;

	return SUCCESS;
}

