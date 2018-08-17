#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include "Arduino.h"
#include "ads.h"

#define ADS_RESET_PIN      (4)          // Pin number attached to ads reset line.
#define ADS_INTERRUPT_PIN   (3)         // Pin number attached to the ads data ready line. 

void ads_data_callback(float sample);
float deadzone_filter(float sample);
float signal_filter(float sample);
void parse_com_port(void);

float signal_filter(float sample)
{
    static float filter_samples[6];
    
    filter_samples[5] = filter_samples[4];
    filter_samples[4] = filter_samples[3];
    filter_samples[3] = (float)sample;
    filter_samples[2] = filter_samples[1];
    filter_samples[1] = filter_samples[0];

    // 20 Hz cutoff frequency @ 100 Hz Sample Rate
    filter_samples[0] = filter_samples[1]*(0.36952737735124147f) - 0.19581571265583314f*filter_samples[2] + \
      0.20657208382614792f*(filter_samples[3] + 2*filter_samples[4] + filter_samples[5]);
      
    return filter_samples[0];
}

float deadzone_filter(float sample)
{
  static float prev_sample = 0.0f;
  float dead_zone = 0.5f;

  if(fabs(sample-prev_sample) > dead_zone)
    prev_sample = sample;
  else
    sample = prev_sample;

  return sample;
}

/* Receives new samples from the ADS library */
void ads_data_callback(float sample)
{
  // Low pass IIR filter
  sample = (float)signal_filter(sample);

  // Deadzone filter
  sample = deadzone_filter(sample);

  Serial.println(sample);
  
  // Use this line to prevent serial plotter from autoscaling and comment out above print
  //Serial.print(sample);Serial.print(","); Serial.print(-300); Serial.print(",");Serial.println(300);
}

void setup() {
  Serial.begin(115200);

  Serial.println("Initializing One Axis sensor");
  
  ads_init_t init;                                // One Axis ADS initialization structure

  init.sps = ADS_100_HZ;                          // Set sample rate to 100 Hz
  init.ads_sample_callback = &ads_data_callback;  // Provide callback for new data
  init.reset_pin = ADS_RESET_PIN;                 // Pin connected to ADS reset line
  init.datardy_pin = ADS_INTERRUPT_PIN;           // Pin connected to ADS data ready interrupt

  // Initialize ADS hardware abstraction layer, and set the sample rate
  if(ads_init(&init) != ADS_OK)
  {
    Serial.println("One Axis ADS initialization failed");
  }

  // Start reading data!
  ads_run(true);
}

void loop() {
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
      ads_calibrate(ADS_CALIBRATE_FIRST, 0);
      break;
    case '9':
      ads_calibrate(ADS_CALIBRATE_SECOND, 90);
      break;
    case 'c':
      ads_calibrate(ADS_CALIBRATE_CLEAR, 0);
      break;
    case 'k':
      ads_calibrate((ADS_CALIBRATION_STEP_T)0x55, 0);
      break;
    case 'r':
      ads_run(true);
      break;
    case 's':
      ads_run(false);
      break;
    case 'f':
      ads_set_sample_rate(ADS_200_HZ);
      break;
    case 'u':
      ads_set_sample_rate(ADS_10_HZ);
      break;
    case 'n':
      ads_set_sample_rate(ADS_100_HZ);
      break;
    default:
      break;
  }
}

