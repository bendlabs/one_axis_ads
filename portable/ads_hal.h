/**
 * Created by cottley on 4/13/2018.
 */

#ifndef ADS_HAL_
#define ADS_HAL_

#include <stdint.h>
#include "ads_err.h"

#define ADS_TRANSFER_SIZE		(3)


/**
 * @brief Millisecond delay routine.
 */
void ads_hal_delay(uint16_t delay_ms);

/**
 * @brief Initializes the pin ADS_INTERRUPT_PIN as a falling edge pin change interrupt.
 *			Assign the interrupt service routine as ads_hal_interrupt. Enable pullup
 *			Enable interrupt	
 */
void ads_hal_pin_int_enable(bool enable);

/**
 * @brief Write buffer of data to the Angular Displacement Sensor
 *
 * @param buffer[in]	Write buffer
 * @param len			Length of buffer.
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_write_buffer(uint8_t * buffer, uint8_t len);

/**
 * @brief Read buffer of data from the Angular Displacement Sensor
 *
 * @param buffer[out]	Read buffer
 * @param len			Length of buffer.
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_read_buffer(uint8_t * buffer, uint8_t len);

/**
 * @brief Reset the Angular Displacement Sensor
 */
void ads_hal_reset(void);

/**
 * @brief Initializes the hardware abstraction layer 
 *
 * @param callback to ads.cpp, 
 * @param reset_pin pin number for reset line of ads
 * @param datardy_pin pin number for data ready interrupt 
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_init(void (*callback)(uint8_t*), uint32_t reset_pin, uint32_t datardy_pin);

/**
 * @brief Gets the current i2c address that the hal layer is addressing. 	
 *				Used by device firmware update (dfu)
 * @return	uint8_t _address
 */
uint8_t ads_hal_get_address(void);

/**
 * @brief Sets the current i2c address that the hal layer is addressing. 	
 *				Used by device firmware update (dfu) 
 */
void ads_hal_set_address(uint8_t address);

#endif /* ADS_HAL_ */
