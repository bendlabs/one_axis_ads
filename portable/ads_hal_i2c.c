/**
 * ads_hal_i2c.c
 *
 * Created by cottley on 4/13/2018.
 */

#include "ads_hal.h"

/* Hardware Specific Includes */
#include "Arduino.h"
#include "Wire.h"

static void (*ads_read_callback)(uint8_t *);

static uint8_t read_buffer[ADS_TRANSFER_SIZE];

#define ADS_DEFAULT_ADDR		(0x12)			// Default I2C address of the ADS one axis sensor

static uint32_t ADS_RESET_PIN = 0;
static uint32_t ADS_INTERRUPT_PIN = 0;


static uint8_t _address = ADS_DEFAULT_ADDR;

volatile bool _ads_int_enabled = false;


/************************************************************************/
/*                        HAL Stub Functions                            */
/************************************************************************/
static inline void ads_hal_gpio_pin_write(uint8_t pin, uint8_t val);
static void ads_hal_pin_int_init(void);
static void ads_hal_i2c_init(void);


/**
 * @brief ADS data ready interrupt. Reads out packet from ADS and fires 
 *  		callback in ads.c
 */
void ads_hal_interrupt(void)
{
	if(ads_hal_read_buffer(read_buffer, ADS_TRANSFER_SIZE) == ADS_OK)
	{
		ads_read_callback(read_buffer);
	}
}

/**
 * @brief Initializes the pin ADS_INTERRUPT_PIN as a falling edge pin change interrupt.
 *			Assign the interrupt service routine as ads_hal_interrupt. Enable pullup
 *			Enable interrupt	
 */
static void ads_hal_pin_int_init(void)
{
 	pinMode(ADS_INTERRUPT_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN), ads_hal_interrupt, FALLING);
}

/**
 * @brief Write pin to level of val	
 */
static inline void ads_hal_gpio_pin_write(uint8_t pin, uint8_t val)
{
	digitalWrite(pin, val);
}

/**
 * @brief Millisecond delay routine.
 */
void ads_hal_delay(uint16_t delay_ms)
{
	delay(delay_ms);
}

/**
 * @brief Enable/Disable the pin change data ready interrupt
 *
 * @param enable		true = enable, false = disable
 */
void ads_hal_pin_int_enable(bool enable)
{
	// Copy enable to local variable to store enabled state of pin change interrupt
	_ads_int_enabled = enable;
	
	// Enable/Disable the pin change data ready interrupt
	if(enable)
	{
		attachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN), ads_hal_interrupt, FALLING);
	}
	else
	{
		detachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN));
	}
}

/**
 * @brief Configure I2C bus, 7 bit address, 400kHz frequency enable clock stretching
 *			if available.
 */
static void ads_hal_i2c_init(void)
{
	Wire.begin();
	Wire.setClock(400000);	
}

/**
 * @brief Write buffer of data to the Angular Displacement Sensor
 *
 * @param buffer[in]	Write buffer
 * @param len			Length of buffer.
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_write_buffer(uint8_t * buffer, uint8_t len)
{
	// Disable the interrupt, if interrupt is enabled
	if(_ads_int_enabled)
		detachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN));
	
	// Write the the buffer to the ADS sensor
	Wire.beginTransmission(_address);
	Wire.write(buffer, len);
	int ret_val = Wire.endTransmission();
	
	// Re-enable the interrupt, if the interrupt was enabled
	if(_ads_int_enabled)
	{
		attachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN), ads_hal_interrupt, FALLING);
		
		// Read data packet if interrupt was missed
		if(digitalRead(ADS_INTERRUPT_PIN) == 0)
		{
			if(ads_hal_read_buffer(read_buffer, ADS_TRANSFER_SIZE) == ADS_OK)
			{
				ads_read_callback(read_buffer);
			}
		}
	}
	
	if(!ret_val)
		return ADS_OK;
	else
		return ADS_ERR_IO;
}

/**
 * @brief Read buffer of data from the Angular Displacement Sensor
 *
 * @param buffer[out]	Read buffer
 * @param len			Length of data to read in number of bytes.
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_read_buffer(uint8_t * buffer, uint8_t len)
{
	Wire.requestFrom(_address, len);
	
	uint8_t i = 0; 
	
	while(Wire.available())
	{
		buffer[i] = Wire.read();
		i++;
	}
	
	if(i == len)
		return ADS_OK;
	else
		return ADS_ERR_IO;
}

/**
 * @brief Reset the Angular Displacement Sensor
 */
void ads_hal_reset(void)
{
	// Configure reset line as an output
	pinMode(ADS_RESET_PIN, OUTPUT);
	
	// Bring reset low for 10ms then release
	ads_hal_gpio_pin_write(ADS_RESET_PIN, 0);
	ads_hal_delay(10);
	ads_hal_gpio_pin_write(ADS_RESET_PIN, 1);
}

/**
 * @brief Initializes the hardware abstraction layer 
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_init(void (*callback)(uint8_t*), uint32_t reset_pin, uint32_t datardy_pin)
{
	// Copy pin numbers for reset and data ready to local variables
	ADS_RESET_PIN     = reset_pin;
	ADS_INTERRUPT_PIN = datardy_pin;
	
	// Set callback pointer
	ads_read_callback = callback;
	
	// Reset the ads
	ads_hal_reset();
	
	// Wait for ads to initialize
	ads_hal_delay(2000);
	
	// Configure and enable interrupt pin
	ads_hal_pin_int_init();
	
	// Initialize the I2C bus
	ads_hal_i2c_init();

	return ADS_OK;
}

/**
 * @brief Gets the current i2c address that the hal layer is addressing. 	
 *				Used by device firmware update (dfu)
 * @return	uint8_t _address
 */
uint8_t ads_hal_get_address(void)
{
	return _address;
}

/**
 * @brief Sets the i2c address that the hal layer is addressing *	
 *				Used by device firmware update (dfu)
 *
 * @param address		i2c address hal to communicate with
 */
void ads_hal_set_address(uint8_t address)
{
	_address = address;
}
