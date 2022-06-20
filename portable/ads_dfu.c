/**
 * Created by cottley on 4/17/2018.
 */

#include "ads_dfu.h"
#include <string.h>

#if ADS_FW_INCLUDE_ADS1_V1 == 1
	#include "ads_fw.h"
#endif

#if ADS_FW_INCLUDE_ADS1_V2 == 1
	#include "ads_fw_v2.h"
#endif

#define ADS_BOOTLOADER_ADDRESS (0x12)

/**
 * @brief Reads acknowledgment byte from the ADS bootloader
 *
 * @return	ADS_OK if successful ADS_ERR_TIMEOUT if failed
 */
static inline int ads_dfu_get_ack(void)
{
	uint8_t timeout = 254;
	uint8_t ack = 0;
	
	do
	{
		ads_hal_read_buffer(&ack, 1);
	} while(--timeout && ack != 's');
	
	if(timeout)
		return ADS_OK;
	else
		return ADS_ERR_TIMEOUT;
}

/**
 * @brief Checks if the firmware image in the driver is newer than 
 *			the firmware on the device.
 *
 * @param ads_get_fw_ver	Get fw version command
 * @return	TRUE if update needed. FALSE if no updated needed
 */
bool ads_dfu_check(ADS_DEV_TYPE_T ads_dev_type)
{
	uint8_t buffer[] = {ADS_GET_FW_VER, 0, 0};
	uint16_t fw_ver;
	
	ads_hal_pin_int_enable(false);
	
	ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
	ads_hal_delay(2);
	ads_hal_read_buffer(buffer, ADS_TRANSFER_SIZE);
	
	ads_hal_pin_int_enable(true);
	
	if(buffer[0] == ADS_FW_VER)
		fw_ver = ads_uint16_decode(&buffer[1]);
	else
		return false;
		
#if ADS_FW_INCLUDE_ADS1_V1 == 1
	if (ads_dev_type == ADS_DEV_ONE_AXIS_V1)
		return fw_ver < ads_fw_rev;
#endif
	
#if ADS_FW_INCLUDE_ADS1_V2 == 1
	if (ads_dev_type == ADS_DEV_ONE_AXIS_V2)
		return fw_ver < ads_fw_v2_rev;
#endif

	return false;
}

/**
 * @brief Resets the ADS into bootloader mode
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
 int ads_dfu_reset(void)
 {
	 uint8_t packet[ADS_TRANSFER_SIZE];
	 
	 packet[0] = ADS_DFU;
	 packet[1] = packet[2] = 0;
	 
	 return ads_hal_write_buffer(packet, ADS_TRANSFER_SIZE);
 }

/**
 * @brief Writes firmware image to ADS contained in ads_fw.h
 *
 * @return	ADS_OK if successful, ADS_ERR_DEV_ID if no device support or ADS_ERR_TIMEOUT if failed
 */
int ads_dfu_update(ADS_DEV_TYPE_T ads_dev_type)
{
	const uint8_t * fw = NULL;
	
#if ADS_FW_INCLUDE_ADS1_V1 == 1
	if (ads_dev_type == ADS_DEV_ONE_AXIS_V1)
		fw = ads_fw;
#endif
	
#if ADS_FW_INCLUDE_ADS1_V2 == 1
	if (ads_dev_type == ADS_DEV_ONE_AXIS_V2)
		fw = ads_fw_v2;
#endif

	if (fw == NULL)
		return ADS_ERR_DEV_ID;
		
	uint32_t len = sizeof(fw);
	
	uint8_t page_size = 64;
	uint8_t block_len = page_size;
	
	uint8_t packet[page_size];
	
	// Store a local copy of the current i2c address
	uint8_t address = ads_hal_get_address();
	
	// Set the i2c address to the booloader address
	ads_hal_set_address(ADS_BOOTLOADER_ADDRESS);
	
	// Transmit the length of the new firmware to the bootloader
	packet[0] = (uint8_t)(len & 0xff);
	packet[1] = (uint8_t)((len >> 8) & 0xff);
	packet[2] = (uint8_t)((len>>16) & 0xff);
	packet[3] = (uint8_t)((len>>24) & 0xff);
	
	ads_hal_write_buffer(packet, 4);
	
	// Get acknowledgement of the fw length
	if(ads_dfu_get_ack() != ADS_OK)
	{
		// restore i2c address
		ads_hal_set_address(address);		
		return ADS_ERR_TIMEOUT;
	}
	
	// Transfer the new firmware image
	uint32_t nb_blocks = len/block_len;
	uint32_t rem_data  = len % block_len;
	
	for(uint32_t i = 0; i < nb_blocks; i++)
	{
		// Copy the next page
		memcpy(packet, &fw[i*page_size], page_size);
		
		// Send the page
		ads_hal_write_buffer(packet, page_size/2);
		ads_hal_write_buffer(&packet[page_size/2], page_size/2);
		
		// Get acknowledgement of the recieved page
		if(ads_dfu_get_ack() != ADS_OK)
		{
			// restore i2c address
			ads_hal_set_address(address);
			return ADS_ERR_TIMEOUT;
		}
	}
	
	// Transfer the remainder and get acknowledgement
	memcpy(packet, &fw[nb_blocks*block_len], rem_data);
	
	if(rem_data > page_size/2)
	{
		ads_hal_write_buffer(packet, page_size/2);
		ads_hal_write_buffer(&packet[page_size/2], rem_data - page_size/2);
	}
	else
		ads_hal_write_buffer(packet, rem_data);
	
	if(ads_dfu_get_ack() != ADS_OK)
	{
		// restore i2c address
		ads_hal_set_address(address);		
		return ADS_ERR_TIMEOUT;
	}
	
	// restore i2c address
	ads_hal_set_address(address);
	
	return ADS_OK;
}
