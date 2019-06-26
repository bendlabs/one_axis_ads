#include "Arduino.h"
#include "ads.h"

#include <bluefruit.h>
#include <string.h>

#define ADS_RESET_PIN       (27)        // Pin number attached to ads reset line.
#define ADS_INTERRUPT_PIN   (30)        // Pin number attached to the ads data ready line.  


BLEService        angms = BLEService(0x1820);
BLECharacteristic angmc = BLECharacteristic(0x2A70);


BLEDis bledis;    // DIS (Device Information Service) helper class instance
BLEBas blebas;    // BAS (Battery Service) helper class instance


// function prototypes
void startAdv(void);
void setupANGM(void);
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
void ads_data_callback(float sample);
float deadzone_filter(float sample);
float signal_filter(float sample);
void parse_serial_port(void);
 

float ang = 0.0f;
volatile bool newData = false;

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

void ads_data_callback(float sample)
{
  // Low pass IIR filter
  sample = (float)signal_filter(sample);

  // Deadzone filter
  sample = deadzone_filter(sample);
  
  ang = sample;
  newData = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("One Axis ADS BLE Example");
  Serial.println("-----------------------\n");

  Bluefruit.begin();
  
  // Set the advertised device name
  Serial.println("Setting Device Name to 'one_axis_ads'");
  Bluefruit.setName("one_axis_ads");


  // Set the connect/disconnect callback handlers
  Bluefruit.setConnectCallback(connect_callback);
  Bluefruit.setDisconnectCallback(disconnect_callback);

  Bluefruit.setConnIntervalMS(7.5,200);

  // Configure and Start the Device Information Service
  //Serial.println("Configuring the Device Information Service");
  bledis.setManufacturer("Bend Labs");
  bledis.setModel("One Axis Demo");
  bledis.setHardwareRev("REV: 0.0.0");
  bledis.setSoftwareRev("SD: 132.2.0.0");
  bledis.begin();

  // Start the BLE Battery Service and set it to 100%
  //Serial.println("Configuring the Battery Service");
  blebas.begin();
  blebas.write(100);

  // Setup the Angle Measurement service using
  // BLEService and BLECharacteristic classes
  //Serial.println("Configuring the Heart Rate Monitor Service");
  setupANGM();

  // Setup the advertising packet(s)
  //Serial.println("Setting up the advertising payload(s)");
  startAdv();
  
  Serial.println("\nAdvertising"); 

  Serial.println("Initializing the One Axis Angular Displacement Sensor");
  ads_init_t init;

  init.sps = ADS_100_HZ;
  init.ads_sample_callback = &ads_data_callback;
  init.reset_pin = ADS_RESET_PIN;                 // Pin connected to ADS reset line
  init.datardy_pin = ADS_INTERRUPT_PIN;           // Pin connected to ADS data ready interrupt
  init.addr = 0;

  delay(100);

  if(ads_init(&init) != ADS_OK)
    Serial.println("One Axis ADS initialization failed");

  //delay(100);
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  //Bluefruit.Advertising.addTxPower();

  // Include Angle Measurement Service UUID
  Bluefruit.Advertising.addService(angms);

  // Include Name
  Bluefruit.Advertising.addName();

  Bluefruit.Advertising.addAppearance(113);
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(500,800);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

void setupANGM(void)
{
  // Configure the Angle Measurement service
  // Supported Characteristics:
  // Name                         UUID    Requirement Properties
  // ---------------------------- ------  ----------- ----------
  // Angle Measurement            0x2A70  Mandatory   Notify/Write    
  angms.begin();

  // Configure the Angle Measurement characteristic
  // Properties = Notify
  // Min Len    = 4
  // Max Len    = 4
  // Little Endian Float
  angmc.setProperties(CHR_PROPS_NOTIFY|CHR_PROPS_WRITE);
  angmc.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  angmc.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  angmc.setWriteCallback(write_callback);
  angmc.begin();
  uint8_t ang_initial[] = {0, 0, 0, 0};
  angmc.notify(ang_initial, 4);                   // Use .notify instead of .write!
}

void connect_callback(uint16_t conn_handle)
{
    Serial.print("Connected");
    ads_polled(true);
  //ads_run(true);
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{

  Serial.println(reason);
  (void) conn_handle;
  (void) reason;

  Serial.println("Disconnected");
  Serial.println("Advertising!");
  
  ads_polled(false);
  //ads_run(false);
}

void write_callback(BLECharacteristic& chr, unsigned char * rx, short unsigned len, short unsigned dah)
{
  if(len == 1)
  {
    if(rx[0] == 0)
    {
      ads_calibrate(ADS_CALIBRATE_FIRST, 0);
    }
    else if(rx[0] == 1)
    {
      ads_calibrate(ADS_CALIBRATE_SECOND, 90);
    }
    else if(rx[0] == 2)
    {
      //Not implemented for one axis sensor
    }
    else if(rx[0] == 3)
    {
      ads_calibrate(ADS_CALIBRATE_CLEAR, 0);
    }
    else if(rx[0] == 0x07)
    {
      NVIC_SystemReset();
    }
  }
  else if(len == 2)
  {
    uint16_t sps = ads_uint16_decode(rx);
    
    ads_set_sample_rate((ADS_SPS_T)sps);
  }
}

void cccd_callback(BLECharacteristic& chr, uint16_t cccd_value)
{
    // Display the raw request packet
    //Serial.print("CCCD Updated: ");
    //Serial.printBuffer(request->data, request->len);
    //Serial.print(cccd_value);
    //Serial.println("");

    // Check the characteristic this CCCD update is associated with in case
    // this handler is used for multiple CCCD records.
    if (chr.uuid == angmc.uuid) {
        if (chr.notifyEnabled()) {
            Serial.println("Angle Measurement 'Notify' enabled");
        } else {
            Serial.println("Angle Measurement 'Notify' disabled");
        }
    }
}

void parse_serial_port(void)
{
    char key = Serial.read();
    
    if(key == '0')
      ads_calibrate(ADS_CALIBRATE_FIRST, 0);
    else if(key == '9')
      ads_calibrate(ADS_CALIBRATE_SECOND, 90);
    else if(key == 'c')
      ads_calibrate(ADS_CALIBRATE_CLEAR, 0);
    else if(key == 'r')
      ads_run(true);
    else if(key == 's')
      ads_run(false);
    else if(key == 'f')
      ads_set_sample_rate(ADS_200_HZ);
    else if(key == 'u')
      ads_set_sample_rate(ADS_10_HZ);
    else if(key == 'n')
      ads_set_sample_rate(ADS_100_HZ);
}

void loop() {
  // put your main code here, to run repeatedly:

  uint8_t read_buffer[3];

  float ang = 0.0f;

  if( ads_hal_read_buffer(read_buffer, 3) == ADS_OK)
  {
    int16_t temp = ads_int16_decode(&read_buffer[1]);
    ang = (float)temp/64.0f;
    Serial.println(ang);
  }
  
  
  //if ( newData ) 
  {
    if(Bluefruit.connected())
    {
      uint8_t ang_encoded[4];
      memcpy(ang_encoded, &ang, sizeof(float));
      angmc.notify(ang_encoded, sizeof(ang_encoded));
    }
    newData = false;
    Serial.println(ang);
  }

  if(Serial.available())
  {
    parse_serial_port();
  } 

  delay(1);
  
  //delay(5);
}

void rtos_idle_callback(void)
{
  // Don't call any other FreeRTOS blocking API()
  // Perform background task(s) here
}
