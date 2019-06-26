/**
 * Created by cottley on 4/13/2018.
 */

#ifndef ADS_UTIL_H_
#define ADS_UTIL_H_

#include <stdint.h>

typedef enum {
	ADS_RUN = 0,			// Place ADS in freerun interrupt mode or standby
	ADS_SPS,				// Update SPS on ADS in freerun interrupt mode
	ADS_RESET,				// Software reset command
	ADS_DFU,				// Reset ADS into bootloader for firmware update
	ADS_SET_ADDRESS,		// Update the I2C address on the ADS
	ADS_POLLED_MODE,		// Place ADS in polled mode or standby
	ADS_GET_FW_VER,			// Get firwmare version on the ADS
	ADS_CALIBRATE,			// Calibration command, see ADS_CALIBRATION_STEP_T
	ADS_READ_STRETCH,		// Enable simultaneous bend and stretch measurements
	ADS_SHUTDOWN,			// Shuts ADS down, lowest power mode, requires reset to wake
	ADS_GET_DEV_ID			// Gets unique device ID for ADS sensor, see ADS_DEV_IDS_T
} ADS_COMMAND_T;			// ADS command control enumeration

typedef enum {
	ADS_SAMPLE = 0,			// Packet read is a bend sample
	ADS_FW_VER,				// Packet read is the firmware version on the sensor
	ADS_DEV_ID,				// Packet read is the device id of the ADS sensor
	ADS_STRETCH_SAMPLE		// Packet read is a stretch sample
} ADS_PACKET_T;

/* Device IDS */
typedef enum {
	ADS_ONE_AXIS = 1,		// Device ID for the one axis ADS
	ADS_TWO_AXIS = 2,		// Device ID for the two axis ADS
} ADS_DEV_IDS_T;

/**@brief Function for decoding a int16 value.
 *
 * @param[in]   p_encoded_data   Buffer where the encoded data is stored.
 * @return      Decoded value.
 */
inline int16_t ads_int16_decode(const uint8_t * p_encoded_data)
{
        return ( (((uint16_t)(p_encoded_data)[0])) |
                 (((int16_t)(p_encoded_data)[1]) << 8 ));
}

/**@brief Function for decoding a uint16 value.
 *
 * @param[in]   p_encoded_data   Buffer where the encoded data is stored.
 * @return      Decoded value.
 */
inline uint16_t ads_uint16_decode(const uint8_t * p_encoded_data)
{
        return ( (((uint16_t)(p_encoded_data)[0])) |
                 (((uint16_t)(p_encoded_data)[1]) << 8 ));
}

/**@brief Function for encoding a uint16 value.
 *
 * @param[in]   value            Value to be encoded.
 * @param[out]  p_encoded_data   Buffer where the encoded data is to be written.
 *
 * @return      Number of bytes written.
 */
inline uint8_t ads_uint16_encode(uint16_t value, uint8_t * p_encoded_data)
{
    p_encoded_data[0] = (uint8_t) ((value & 0x00FF) >> 0);
    p_encoded_data[1] = (uint8_t) ((value & 0xFF00) >> 8);
    return sizeof(uint16_t);
}

#endif /* ADS_UTIL_H_ */
