/**
 * Created by cottley on 4/16/2018.
 */

#include "ads.h"

static ads_callback ads_data_callback;


/**
 * @brief Parses sample buffer from one axis ADS. Scales to degrees and
 *				executes callback registered in ads_init. 
 *				This function is called from ads_hal. Application should never call this function.
 */	
void ads_parse_read_buffer(uint8_t * buffer)
{
	if(buffer[0] == ADS_SAMPLE)
	{
		int16_t temp = ads_int16_decode(&buffer[1]);
		float sample = (float)temp/64.0f;
		
		ads_data_callback(sample);
	}
}

/**
 * @brief Places ADS in free run or sleep mode
 *
 * @param	run	true if activating ADS, false is putting in suspend mode
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_run(bool run)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
		
	buffer[0] = ADS_RUN;
	buffer[1] = run;
		
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Sets the sample rate of the ADS in free run mode
 *
 * @param	sps ADS_SPS_T sample rate
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_set_sample_rate(ADS_SPS_T sps)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_SPS;
	ads_uint16_encode(sps, &buffer[1]);
	
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Enables the ADS data ready interrupt line
 *				NOT CURRENTLY IMPLEMENTED 
 * @param	enable	true if activating Data Ready Interrupt, false is disabling.
 *			default - enabled.
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_enable_interrupt(bool enable)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_INTERRUPT_ENABLE;
	buffer[1] = enable;
	
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Updates the I2C address of the selected ADS. The default address 
 *		  is 0x12. Use this function to program an ADS to allow multiple
 *		  devices on the same I2C bus.
 *
 * @param	device	device number of the device that is being updated
 * @param	address	new address of the ADS
 * @return	ADS_OK if successful ADS_ERR_IO or ADS_ERR_BAD_PARAM if failed
 */
int ads_update_device_address(uint8_t device, uint8_t address)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_SET_ADDRESS;
	buffer[1] = address;
	
	if(ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE) != ADS_OK)
		return ADS_ERR_IO;
	
	return ads_hal_update_device_addr(device, address);	
}

/**
 * @brief Updates selected device number the HAL is communicating with
 *
 * @param	device number 0-9 
 * @return	ADS_OK if successful ADS_ERR_BAD_PARAM if failed
 */
int ads_select_device(uint8_t device)
{
	return ads_hal_select_device(device);
}

/**
 * @brief Initializes the hardware abstraction layer and sample rate of the ADS
 *
 * @param	ads_init_t	initialization structure of the ADS
 * @return	ADS_OK if successful ADS_ERR if failed
 */
int ads_init(ads_init_t * ads_init)
{
	ads_hal_init(&ads_parse_read_buffer, ads_init->reset_pin, ads_init->datardy_pin);	
	
	ads_data_callback = ads_init->ads_sample_callback;
	
	// Check that the device id matched ADS_ONE_AXIS
	if(ads_get_dev_id() != ADS_OK)
		return ADS_ERR_DEV_ID;
	
	ads_hal_delay(2);
 	
 	if(ads_set_sample_rate(ads_init->sps))
		return ADS_ERR;
	
	ads_hal_delay(2);
	
#if ADS_DFU_CHECK
 	if(ads_dfu_check((uint8_t)ADS_GET_FW_VER))
	{
		ads_dfu_reset();
		ads_hal_delay(50);		// Give ADS time to reset
 		ads_dfu_update();
		ads_hal_delay(2000);	// Let it reinitialize
	}
#endif
	
	return ADS_OK;
}

/**
 * @brief Calibrates one axis ADS. ADS_CALIBRATE_FIRST should be at 0 degrees on
 *				ADS_CALIBRATE_SECOND can be at 45 - 255 degrees, recommended 90 degrees.
 *
 * @param	ads_calibration_step 	ADS_CALIBRATE_STEP_T to perform
 * @param degrees uint8_t angle at which sensor is bent when performing 
 *				ADS_CALIBRATE_FIRST, and ADS_CALIBRATE_SECOND
 * @return	ADS_OK if successful ADS_ERR_IO or ADS_BAD_PARAM if failed
 */
int ads_calibrate(ADS_CALIBRATION_STEP_T ads_calibration_step, uint8_t degrees)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_CALIBRATE;
	buffer[1] = ads_calibration_step;
	buffer[2] = degrees;
	
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Shutdown ADS. Requires reset to wake up from Shutdown. ~50nA in shutdwon
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_shutdown(void)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_SHUTDOWN;
	
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Wakes up ADS from shutdown. Delay is necessary for ADS to reinitialize 
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_wake(void)
{
	// Reset ADS to wake from shutdown
	ads_hal_reset();
	
	// Allow time for ADS to reinitialize 
	ads_hal_delay(100);	
	
	return ADS_OK;
}

/**
 * @brief Checks that the device id is ADS_ONE_AXIS. ADS should not be in free run
 * 			when this function is called.
 *
 * @return	ADS_OK if dev_id is ADS_ONE_AXIS, ADS_ERR_DEV_ID if not
 */
int ads_get_dev_id(void)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_GET_DEV_ID;
	
	// Disable interrupt to prevent callback from reading out device id
	ads_hal_pin_int_enable(false);
	
	ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
	ads_hal_delay(2);
	ads_hal_read_buffer(buffer, ADS_TRANSFER_SIZE);
	
	ads_hal_pin_int_enable(true);
	
	/* Check that packet read is a device id packet and that
	 * and that the device id is a two axis sensor */
	if(buffer[0] == ADS_DEV_ID && buffer[1] == ADS_ONE_AXIS)
	{
		return ADS_OK;
	}
	else
	{
		return ADS_ERR_DEV_ID;
	}
}
