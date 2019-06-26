/* 
 *  Updating the I2C address on the one axis soft flex sensor 
 *  from Bend Labs
 *  By: Colton Ottley @ Bend Labs
 *  Date: June 19th, 2019
 *  
 *  Updates the I2C address on the one axis sensor from current address stored 
 *  in init.addr to new address stored in new_address. Open Serial Monitor and
 *  send any key to begin updating.
 *  
 *  Sensor is not 5V tolerant use only with 3.3V boards
 *  
 *  Refer to one_axis_quick_start_guide.pdf for wiring instructions
 */

#include "Arduino.h"
#include "ads.h"

#define ADS_RESET_PIN      (3)           // Pin number attached to ads reset line.
#define ADS_INTERRUPT_PIN  (4)           // Pin number attached to the ads data ready line. 

void ads_data_callback(float * sample);

/* Stub function here for initialization code to run */
void ads_data_callback(float * sample, uint8_t sample_type)
{
  
}

// New I2C address being written to the attached ads one axis sensor
uint8_t new_address = 0x15;

void setup() {
  Serial.begin(115200);

  // Wait for keypress from user
  while(!Serial.available());

  Serial.read();

  Serial.println("Initializing One Axis sensor");
  
  ads_init_t init;                                // One Axis ADS initialization structure

  init.sps = ADS_100_HZ;                          // Set sample rate to 100 Hz
  init.ads_sample_callback = &ads_data_callback;  // Provide callback for new data
  init.reset_pin = ADS_RESET_PIN;                 // Pin connected to ADS reset line
  init.datardy_pin = ADS_INTERRUPT_PIN;           // Pin connected to ADS data ready interrupt
  init.addr = 0x12;                               // Update value if non default I2C address is assinged to sensor

  // Initialize ADS hardware abstraction layer, and set the sample rate
  int ret_val = ads_init(&init);
  
  if(ret_val != ADS_OK)
  {
    Serial.print("One Axis ADS initialization failed with reason: ");
    Serial.println(ret_val);

    while(1) 
    {
      // Do not proceed with address update if initialization failed 
    }
  }
  else
  {
    Serial.println("One Axis ADS initialization succeeded...\n");
  }

  Serial.println("Updating I2C address...\n");

  // Updating I2C address of sensor with new address stored in new_address
  ret_val = ads_update_device_address(new_address);

  if(ret_val == ADS_OK)
  {
    Serial.println("I2C Device Address updated successfully");
    Serial.print("New device address: 0x");
    Serial.println(new_address, HEX);
  }
  else
  {
    Serial.print("Updating I2C Device Address failed with reason: ");
    Serial.println(ret_val);
  }
}

void loop() {
  // All tasks carried out in setup. Do nothing
}
