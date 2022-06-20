/**
 * Created by cottley on 4/17/2018.
 */

#ifndef ADS_DFU_
#define ADS_DFU_

#include <stdint.h>
#include <stdbool.h>
#include "ads_err.h"
#include "ads_hal.h"
#include "ads_util.h"

/*
 * Include firmware update images for ADS_ONE_AXIS versions 1 and 2. 
 * At least one version type should be set to (1) if firmware update capabilities are desired.
 * 
 * To indentify the one axis version you have, please refer to the physical one axis sensor:
 * - Sensor version 1 will have an "indentation" near pin 1.
 * - Sensor version 2 vill have a "protrusion" near pin 1.
 */
 #ifndef ADS_FW_INCLUDE_ADS1_V1
 #define ADS_FW_INCLUDE_ADS1_V1 1 // Set this to 1 to include version 1 firmware image
 #endif
 
 #ifndef ADS_FW_INCLUDE_ADS1_V2
 #define ADS_FW_INCLUDE_ADS1_V2 1 // Set this to 1 to include version 2 firmware image
 #endif

/**
 * @brief Checks if the firmware image in the driver is newer than 
 *			the firmware on the device.
 *
 * @param ads_dev_type	Get fw version command
 * @return	TRUE if update needed. FALSE if no updated needed
 */
bool ads_dfu_check(ADS_DEV_TYPE_T ads_dev_type);

/**
 * @brief Resets the ADS into bootloader
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_dfu_reset(void);

/**
 * @brief Writes firmware image, contained in ads_fw.h, to the ADS bootloader 
 *			  The ADS needs to be reset into bootloader mode prior to calling
 *				this function
 *
 * @return	ADS_OK if successful, ADS_ERR_DEV_ID if no device support or ADS_ERR_TIMEOUT if failed
 */
int ads_dfu_update(ADS_DEV_TYPE_T ads_dev_type);

#endif /* ADS_DFU_ */
