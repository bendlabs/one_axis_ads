/* 
 *  Reading the one axis soft flex sensor from Bend Labs
 *  By: Colton Ottley @ Bend Labs
 *  Date: June 18th, 2019
 *  
 *  This sktech configures the one axis soft flex sensor from Bendlabs
 *  to simultaneously supply bend (angular displacement) and stretch
 *  (linear displacement) data via an interrupt driven callback.
 *  
 *  FUNCTIONALITY ADDED IN FW VERSION 44 TO MEASURE STRETCH
 *  
 *  Sensor is not 5V tolerant use only with 3.3V boards
 *  
 *  Refer to one_axis_quick_start_guide.pdf for wiring instructions
 */
#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include "Arduino.h"
#include "ads.h"

#define ADS_RESET_PIN      (4)           // Pin number attached to ads reset line.
#define ADS_INTERRUPT_PIN  (3)           // Pin number attached to the ads data ready line. 

void ads_data_callback(float * sample);
void deadzone_filter(float * sample);
void signal_filter(float * sample);
void parse_com_port(void);

/* Receives new samples from the ADS library */
void ads_data_callback(float * sample, uint8_t sample_type)
{
  if(sample_type == ADS_SAMPLE)
  {
    // Low pass IIR filter
    signal_filter(sample);
  
    // Deadzone filter
    deadzone_filter(sample);
  
    Serial.print(sample[0]);    // Contains bend data
    Serial.print(",");
    Serial.println(sample[1]);  // Contains stretch data if enabled
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println("Initializing One Axis sensor");
  
  ads_init_t init;                                // One Axis ADS initialization structure

  init.sps = ADS_200_HZ;                          // Set sample rate to 200 Hz
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
  }
  
  // Enable stretch sensor data
  ads_stretch_en(true);

  // Start reading data in interrupt mode
  ads_run(true);
}

void loop() {

  // New data received through the callback function ads_data_callback
  
  // Check for received hot keys on the com port
  if(Serial.available())
  {
    parse_com_port();
  }
}

/* Function parses received characters from the COM port for commands */
void parse_com_port(void)
{
  char key = Serial.read();

  switch(key)
  {
    case '0':
      // Take first calibration point at zero degrees
      ads_calibrate(ADS_CALIBRATE_FIRST, 0);
      break;
    case '9':
      // Take second calibration point at ninety degrees
      ads_calibrate(ADS_CALIBRATE_SECOND, 90);
      break;
    case 'c':
      // Restore factory calibration coefficients
      ads_calibrate(ADS_CALIBRATE_CLEAR, 0);
      break;
    case 'r':
      // Start sampling in interrupt mode
      ads_run(true);
      break;
    case 's':
      // Place ADS in suspend mode
      ads_run(false);
      break;
    case 'f':
      // Set ADS sample rate to 200 Hz (interrupt mode)
      ads_set_sample_rate(ADS_200_HZ);
      break;
    case 'u':
      // Set ADS sample to rate to 10 Hz (interrupt mode)
      ads_set_sample_rate(ADS_10_HZ);
      break;
    case 'n':
      // Set ADS sample rate to 100 Hz (interrupt mode)
      ads_set_sample_rate(ADS_100_HZ);
      break;
    case 'b':
      // Calibrate the zero millimeter linear displacement
      ads_calibrate(ADS_CALIBRATE_STRETCH_ZERO, 0);
      break;
    case 'e':
      // Calibrate the 30 millimeter linear displacement (stretch), Make certain the sensor is at 0 degrees angular displacement (flat)
      ads_calibrate(ADS_CALIBRATE_STRETCH_SECOND, 30);
      break;
    default:
      break;
  }
}

/* 
 *  Second order Infinite impulse response low pass filter. Sample freqency 100 Hz.
 *  Cutoff freqency 20 Hz. 
 */
void signal_filter(float * sample)
{
    static float filter_samples[2][6];

    for(uint8_t i=0; i<2; i++)
    {
      filter_samples[i][5] = filter_samples[i][4];
      filter_samples[i][4] = filter_samples[i][3];
      filter_samples[i][3] = (float)sample[i];
      filter_samples[i][2] = filter_samples[i][1];
      filter_samples[i][1] = filter_samples[i][0];
  
      // 20 Hz cutoff frequency @ 100 Hz Sample Rate
      filter_samples[i][0] = filter_samples[i][1]*(0.36952737735124147f) - 0.19581571265583314f*filter_samples[i][2] + \
        0.20657208382614792f*(filter_samples[i][3] + 2*filter_samples[i][4] + filter_samples[i][5]);   

      sample[i] = filter_samples[i][0];
    }
}

/* 
 *  If the current sample is less that 0.5 degrees different from the previous sample
 *  the function returns the previous sample. Removes jitter from the signal. 
 */
void deadzone_filter(float * sample)
{
  static float prev_sample[2];
  float dead_zone = 0.5f;

  for(uint8_t i=0; i<2; i++)
  {
    if(fabs(sample[i]-prev_sample[i]) > dead_zone)
      prev_sample[i] = sample[i];
    else
      sample[i] = prev_sample[i];
  }
}
