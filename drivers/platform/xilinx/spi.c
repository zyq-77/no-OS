/***************************************************************************//**
 *   @file   xilinx/spi.c
 *   @brief  Implementation of Xilinx SPI Generic Driver.
 *   @author Antoniu Miclaus (antoniu.miclaus@analog.com)
********************************************************************************
 * Copyright 2019(c) Analog Devices, Inc.
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

#include <stdlib.h>

#include <xparameters.h>
#ifdef XPAR_XSPI_NUM_INSTANCES
#include <xspi.h>
#endif
#ifdef XPAR_XSPIPS_NUM_INSTANCES
#include <xspips.h>
#endif

#include "error.h"
#include "spi.h"
#include "spi_extra.h"
#ifdef XPAR_SPI_ADC_AXI_REGMAP_BASEADDR
#include "spi_engine.h"
#endif

#if defined(PLATFORM_ZYNQ)
#define SPI_CLK_FREQ_HZ(dev)    (XPAR_PS7_SPI_ ## dev ## _SPI_CLK_FREQ_HZ)
#define SPI_NUM_INSTANCES       XPAR_XSPIPS_NUM_INSTANCES
#elif defined(PLATFORM_ZYNQMP)
#define SPI_CLK_FREQ_HZ(dev)    (XPAR_PSU_SPI_ ## dev ## _SPI_CLK_FREQ_HZ)
#define SPI_NUM_INSTANCES       XPAR_XSPIPS_NUM_INSTANCES
#endif

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/**
 * @brief Initialize the hardware SPI peripherial
 *
 * @param xdesc Platform specific SPI descriptor
 * @param xinit Platform specific SPI init param
 * @return int32_t FAILURE if the device couldn't be found or configured
 */
static int32_t spi_init_pl(struct spi_desc *desc,
			   const struct spi_init_param *param)
{
#ifdef XSPI_H
	int32_t				ret;
	struct xil_spi_desc 		*xdesc;
	struct xil_spi_init_param	*xinit;

	xdesc = (xil_spi_desc*)malloc(sizeof(xil_spi_desc));
	if(!xdesc) {
		free(xdesc);
		return FAILURE;
	}

	desc->extra = xdesc;
	xinit = param->extra;
	xdesc->type = xinit->type;
	xdesc->flags = xinit->flags;

	xdesc->instance = (XSpi*)malloc(sizeof(XSpi));
	if(!xdesc->instance)
		goto pl_error;

	xdesc->config = XSpi_LookupConfig(xinit->device_id);
	if(xdesc->config == NULL)
		goto pl_error;

	ret = XSpi_CfgInitialize(xdesc->instance,
				 xdesc->config,
				 ((XSpi_Config*)xdesc->config)
				 ->BaseAddress);
	if(ret != SUCCESS)
		goto pl_error;

	ret = XSpi_Initialize(xdesc->instance, xinit->device_id);
	if (ret != SUCCESS)
		goto pl_error;

	ret = XSpi_SetOptions(xdesc->instance,
			      XSP_MASTER_OPTION |
			      ((desc->mode & SPI_CPOL) ?
			       XSP_CLK_ACTIVE_LOW_OPTION : 0) |
			      ((desc->mode & SPI_CPHA) ?
			       XSP_CLK_PHASE_1_OPTION : 0));
	if (ret != SUCCESS)
		goto pl_error;

	ret = XSpi_Start(xdesc->instance);
	if (ret != SUCCESS)
		goto pl_error;

	XSpi_IntrGlobalDisable((XSpi *)(xdesc->instance));

	return SUCCESS;

pl_error:
	free(xdesc->instance);
	free(xdesc);
#endif
	return FAILURE;
}

/**
 * @brief Initialize the hardware SPI peripherial
 *
 * @param xdesc Platform specific SPI descriptor
 * @param xinit Platform specific SPI init param
 * @return int32_t FAILURE if the device couldn't be found or configured
 */
static int32_t spi_init_ps(struct spi_desc *desc,
			   const struct spi_init_param *param)
{
#ifdef XSPIPS_H
	int32_t				ret;
	struct xil_spi_desc 		*xdesc;
	struct xil_spi_init_param	*xinit;
#ifdef XSPIPS_H
	const uint32_t			prescaler_default = XSPIPS_CLK_PRESCALE_64;
	const uint32_t			prescaler_min = XSPIPS_CLK_PRESCALE_4;
	const uint32_t			prescaler_max = XSPIPS_CLK_PRESCALE_256;
#endif
	uint32_t			prescaler = 0u;
	uint32_t			input_clock[SPI_NUM_INSTANCES];

// There may only be 2 XSPI PS instances on PS7 and PSU architectures.
#if (SPI_NUM_INSTANCES >= 1)
	input_clock[0] = SPI_CLK_FREQ_HZ(0);
#endif
#if (SPI_NUM_INSTANCES >= 2)
	input_clock[1] = SPI_CLK_FREQ_HZ(1);
#endif

	xdesc = (xil_spi_desc*)malloc(sizeof(xil_spi_desc));
	if(!xdesc) {
		free(xdesc);
		return FAILURE;
	}

	desc->extra = xdesc;
	xinit = param->extra;
	xdesc->type = xinit->type;
	xdesc->flags = xinit->flags;

	xdesc->instance = (XSpiPs*)malloc(sizeof(XSpiPs));
	if(!xdesc->instance)
		goto ps_error;

	xdesc->config = XSpiPs_LookupConfig(xinit->device_id);
	if(xdesc->config == NULL)
		goto ps_error;

	ret = XSpiPs_CfgInitialize(xdesc->instance,
				   xdesc->config,
				   ((XSpiPs_Config*)xdesc->config)
				   ->BaseAddress);
	if(ret != SUCCESS)
		goto ps_error;

	if (desc->max_speed_hz != 0u) {
		uint32_t div = input_clock[xinit->device_id] / desc->max_speed_hz;
		uint32_t rem = input_clock[xinit->device_id] % desc->max_speed_hz;
		uint32_t po2 = !(div & (div - 1)) && !rem;

		// find the power of two just higher than div and
		// store the exponent in prescaler
		while(div) {
			prescaler += 1;
			div >>= 1u;
		}

		// this exponent - 1 is needed because of the way
		// xilinx stores it into registers
		if (prescaler)
			prescaler -= 1;

		// this exponent - 1 is needed when initial div was
		// precisely a power of two
		if (prescaler && po2)
			prescaler -= 1;

		if (prescaler < prescaler_min)
			prescaler = prescaler_min;

		if (prescaler > prescaler_max)
			prescaler = prescaler_max;
	} else
		prescaler = prescaler_default;

	ret = XSpiPs_SetClkPrescaler(xdesc->instance, prescaler);

	if(ret != SUCCESS)
		goto ps_error;

	return SUCCESS;

ps_error:
	free(xdesc->instance);
	free(xdesc);
#endif
	return FAILURE;
}

/**
 * @brief Initialize the SPI communication peripheral.
 * @param desc - The SPI descriptor.
 * @param param - The structure that contains the SPI parameters.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t spi_init(struct spi_desc **desc,
		 const struct spi_init_param *param)
{
	int32_t				ret;
	enum xil_spi_type		*spi_type;

	if (!param) {
		return FAILURE;
	}

	*desc = malloc(sizeof(**desc));
	if(! *desc) {
		free(*desc);
		return FAILURE;
	}

	/* Get only the first member of the sctructure
	 * Both structures (spi_engine_init_param and spi_init_param)
	 * have the fisrt member enum xil_spi_type so in this case
	 * we can read the first member without casting the pointer to
	 * a certain structure type.
	*/
	spi_type = param->extra;

	if(!spi_type)
		goto init_error;

	(*desc)->max_speed_hz = param->max_speed_hz;
	(*desc)->mode = param->mode;
	(*desc)->chip_select = param->chip_select;

	switch (*spi_type) {
	case SPI_PL:
		ret = spi_init_pl(*desc, param);
		break;
	case SPI_PS:
		ret = spi_init_ps(*desc, param);
		break;
	case SPI_ENGINE:
#ifdef SPI_ENGINE_H
		ret = spi_engine_init(*desc, param);
		break;
#endif

	default:
		goto init_error;
		break;
	}

	if (ret != SUCCESS) {
init_error:
		free(*desc);

		return FAILURE;
	}

	return ret;
}

/**
 * @brief Free the resources allocated by spi_init().
 * @param desc - The SPI descriptor.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t spi_remove(struct spi_desc *desc)
{
#ifdef XSPI_H
	int32_t				ret;
#endif
	struct xil_spi_desc	*xdesc = NULL;
	enum xil_spi_type	*spi_type;

	/* Get only the first member of the sctructure
	 * Both structures (spi_engine_init_param and spi_init_param)
	 * have the fisrt member enum xil_spi_type so in this case
	 * we can read the first member without casting the pointer to
	 * a certain structure type.
	*/
	spi_type = desc->extra;

	if(!spi_type)
		return FAILURE;

	switch (*spi_type ) {
	case SPI_PL:
#ifdef XSPI_H
		xdesc = desc->extra;

		if(!xdesc)
			return FAILURE;

		ret = XSpi_Stop((XSpi *)(xdesc->instance));
		if(ret != SUCCESS)
			goto error;
#endif
		break;
	case SPI_PS:
#ifdef XSPIPS_H
		xdesc = desc->extra;

		if(!xdesc)
			return FAILURE;
#endif
		break;
	case SPI_ENGINE:
#ifdef SPI_ENGINE_H
		spi_engine_remove(desc);
#endif
		/* Intended fallthrough */
#ifdef XSPI_H
error:
#endif
	default:
		return FAILURE;
		break;
	}

	if(xdesc)
		free(xdesc->instance);
	free(desc->extra);
	free(desc);

	return SUCCESS;
}

/**
 * @brief Write and read data to/from SPI.
 * @param desc - The SPI descriptor.
 * @param data - The buffer with the transmitted/received data.
 * @param bytes_number - Number of bytes to write/read.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */

int32_t spi_write_and_read(struct spi_desc *desc,
			   uint8_t *data,
			   uint16_t bytes_number)
{
	int32_t			ret;
	struct xil_spi_desc	*xdesc;
	enum xil_spi_type	*spi_type;

	ret = FAILURE;

	/* Get only the first member of the sctructure
	 * Both structures (spi_engine_init_param and spi_init_param)
	 * have the fisrt member enum xil_spi_type so in this case
	 * we can read the first member without casting the pointer to
	 * a certain structure type.
	*/
	spi_type = desc->extra;

	if(!spi_type)
		return FAILURE;

	if (*spi_type != SPI_ENGINE)
		xdesc = desc->extra;

	switch (*spi_type) {
	case SPI_PL:
#ifdef XSPI_H
		ret = XSpi_SetOptions(xdesc->instance,
				      XSP_MASTER_OPTION |
				      ((desc->mode & SPI_CPOL) ?
				       XSP_CLK_ACTIVE_LOW_OPTION : 0) |
				      ((desc->mode & SPI_CPHA) ?
				       XSP_CLK_PHASE_1_OPTION : 0));
		if (ret != SUCCESS)
			goto error;

		ret = XSpi_SetSlaveSelect(xdesc->instance,
					  0x01 << desc->chip_select);
		if (ret != SUCCESS)
			goto error;

		ret = XSpi_Transfer(xdesc->instance,
				    data,
				    data,
				    bytes_number);
		if (ret != SUCCESS)
			goto error;
#endif
		break;
	case SPI_PS:
#ifdef XSPIPS_H
		ret = XSpiPs_SetOptions(xdesc->instance,
					XSPIPS_MASTER_OPTION |
					((xdesc->flags & SPI_CS_DECODE) ?
					 XSPIPS_DECODE_SSELECT_OPTION : 0) |
					XSPIPS_FORCE_SSELECT_OPTION |
					((desc->mode & SPI_CPOL) ?
					 XSPIPS_CLK_ACTIVE_LOW_OPTION : 0) |
					((desc->mode & SPI_CPHA) ?
					 XSPIPS_CLK_PHASE_1_OPTION : 0));
		if (ret != SUCCESS)
			goto error;

		ret = XSpiPs_SetSlaveSelect(xdesc->instance,
					    desc->chip_select);
		if (ret != SUCCESS)
			goto error;
		ret = XSpiPs_PolledTransfer(xdesc->instance,
					    data,
					    data,
					    bytes_number);
		if (ret != SUCCESS)
			goto error;
		ret = XSpiPs_SetSlaveSelect(xdesc->instance, SPI_DEASSERT_CURRENT_SS);
		if (ret != SUCCESS)
			goto error;
#endif
		break;
	case SPI_ENGINE:
#ifdef SPI_ENGINE_H
		ret = spi_engine_write_and_read(desc, data, bytes_number);
#endif
		break;
error:
	default:
		return FAILURE;
		break;
	}

	return ret;
}
