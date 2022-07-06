/* 
 *  Updating the one axis soft flex sensor from Bend Labs firmware
 *  By: Don Saxby @ Nitto Bend Technologies
 *  Date: July 6th, 2022
 *  
 *  This sktech configures the one axis soft flex sensor from Bendlabs
 *  and updates sensor firmware if out of date.
 *  
 *  The firmware update image is include is source code. It is advised to
 *  run this sketch on device that has sufficient memory.
 *  
 *  Minimum sample interval is 2ms (500 Hz sample rate)
 *  
 *  Sensor is not 5V tolerant use only with 3.3V boards
 *  
 *  Refer to one_axis_quick_start_guide.pdf for wiring instructions
 */

#include "Arduino.h"
#include "ads.h"

/*
 * Include firmware update images for ADS_ONE_AXIS versions 1 and 2. 
 * At least one version type should be set to (1) if firmware update capabilities are desired.
 * 
 * To indentify the one axis version you have, please refer to the physical one axis sensor:
 * - Sensor version 1 will have an "indentation" near pin 1.
 * - Sensor version 2 vill have a "protrusion" near pin 1.
 */
 
#define ADS_FW_INCLUDE_ADS1_V1 0 // Set this to 1 to include version 1 firmware image
#define ADS_FW_INCLUDE_ADS1_V2 0 // Set this to 1 to include version 2 firmware image

#include "ads_dfu.h"

#define ADS_RESET_PIN      (3)           // Pin number attached to ads reset line.
#define ADS_INTERRUPT_PIN  (4)           // Not needed in polled mode.  

/* Not used in polled mode. Stub function necessary for library compilation */
void ads_data_callback(float * sample, uint8_t sample_type)
{
  
}

void setup() {
  Serial.begin(115200);

  Serial.println("Initializing One Axis sensor");

  ads_init_t init{};                              // One Axis ADS initialization structure

  init.sps = ADS_100_HZ;                          // Set sample rate to 100 Hz (Interrupt mode)
  init.ads_sample_callback = &ads_data_callback;  // Provide callback for new data
  init.reset_pin = ADS_RESET_PIN;                 // Pin connected to ADS reset line
  init.datardy_pin = ADS_INTERRUPT_PIN;           // Pin connected to ADS data ready interrupt
  init.addr = 0;                                  // Update value if non default I2C address is assinged to sensor

  // Initialize ADS hardware abstraction layer, and set the sample rate
  int ret_val = ads_init(&init);  
  if(ret_val != ADS_OK)
  {
    Serial.print("One Axis ADS initialization failed with reason: ");
    Serial.println(ret_val);
    return;
  }
  else
  {
    Serial.println("One Axis ADS initialization succeeded...");
  }

  ADS_DEV_TYPE_T dev_type;
  
  ret_val = ads_get_dev_type(&dev_type);
  if(ret_val != ADS_OK)
  {
    Serial.print("One Axis ADS get device type failed with reason: ");
    Serial.println(ret_val);
    return;
  }

  if (!ads_dfu_check(dev_type))
  {
    Serial.println("One Axis ADS firmware is up to date.");
    return;
  }

  // Firmware needs updating. Update device now.
  Serial.println("Updating One Axis ADS firmware...");
  
  ads_dfu_reset();
  ads_hal_delay(50); // Give ADS time to reset
  ads_dfu_update(dev_type);
  ads_hal_delay(2000); // Let it reinitialize

  Serial.println("Update complete.");
}

void loop() {
}
