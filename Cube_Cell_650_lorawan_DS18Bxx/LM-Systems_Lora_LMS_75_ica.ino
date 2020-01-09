#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <OneWire.h>
#include <DallasTemperature.h>

//LMS
//GPIOx defines wich port on the board, dont forget to put a 4,7k resistor, between Vcc and datapin for pullup, 
//this might be neccesary to adjust if several DS18xx devices coexist on the bus
//Device is always start 0 zero on the bus like (sensors.getTempCByIndex(0))= sensor 1

#define ONE_WIRE_BUS GPIO5 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/*
 * set LoraWan_RGB to Active,the RGB active in loraWan
 * RGB red means sending;
 * RGB purple means joined done;
 * RGB blue means RxWindow1;
 * RGB yellow means RxWindow2;
 * RGB green means received done;
 */

//*** Dont forget to edit the commissioning.h file for EUI codes *** 



/*LoraWan Class*/
DeviceClass_t  CLASS=LORAWAN_CLASS;
/*OTAA or ABP*/
bool OVER_THE_AIR_ACTIVATION = LORAWAN_NETMODE;
/*ADR enable*/
bool LORAWAN_ADR_ON = LORAWAN_ADR;
/* set LORAWAN_Net_Reserve ON, the node could save the network info to flash, when node reset not need to join again */
bool KeepNet = LORAWAN_Net_Reserve;
/*LoraWan REGION*/
LoRaMacRegion_t REGION = ACTIVE_REGION;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool IsTxConfirmed = true;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t ConfirmedNbTrials = 8;

/* Application port */
uint8_t AppPort = 2;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t APP_TX_DUTYCYCLE = 15000;

/* Prepares the payload of the frame and request the temperature data from DS18xxx */
static void PrepareTxFrame( uint8_t port )
{
  
  // Read temperatures from DS18xxx
  sensors.begin();
  float temp = (sensors.getTempCByIndex(0));
  Serial.print("Content of temp float value from Sensor: = ");
  Serial.print(temp);
  Serial.println (" Degrees Celsius");
  Serial.println ("Preparing to send data via Semtex Radio ");
  Serial.println();
  delay(5000); // Wait for a while before proceeding
 
  unsigned char *tempout;
  tempout = (unsigned char *)(&temp);
  AppDataSize = 4;//AppDataSize max value is 64
  
 AppData[0] = tempout[0]; 
 AppData[1] = tempout[1]; 
 AppData[2] = tempout[2]; 
 AppData[3] = tempout[3];  
 //AppData[x] = 0x4c will fx. also work 

    }


void setup() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  delay(100);
  BoardInitMcu();
  
  //******** Sensorinits and print examples below ******
  Serial.begin(115200);
  Serial.println("LORAWAN (LM) Dallas Temperature IC Control single sensor demo");
  sensors.begin();
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  delay(2000);
  Serial.println("DONE");
  delay(2000);
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.print(sensors.getTempCByIndex(0)); 
  Serial.println (" Celsius");
  delay(2000);

    
#if(AT_SUPPORT)
    Enable_AT();
#endif
    DeviceState = DEVICE_STATE_INIT;
    LoRaWAN.Ifskipjoin();
}

void loop()
{
 
	switch( DeviceState )
	{
		case DEVICE_STATE_INIT:
		{
#if(AT_SUPPORT)
      getDevParam();
#endif
			printDevParam();
			Serial.printf("LoRaWan Class%X  start! \r\n",CLASS+10);   
			LoRaWAN.Init(CLASS,REGION);
			DeviceState = DEVICE_STATE_JOIN;
			break;
		}
		case DEVICE_STATE_JOIN:
		{
			LoRaWAN.Join();
			break;
		}
		case DEVICE_STATE_SEND:
		{
    
			PrepareTxFrame( AppPort );
			LoRaWAN.Send();
			DeviceState = DEVICE_STATE_CYCLE;
			break;
		}
		case DEVICE_STATE_CYCLE:
		{
			// Schedule next packet transmission
			TxDutyCycleTime = APP_TX_DUTYCYCLE + randr( 0, APP_TX_DUTYCYCLE_RND );
			LoRaWAN.Cycle(TxDutyCycleTime);
			DeviceState = DEVICE_STATE_SLEEP;
			break;
		}
		case DEVICE_STATE_SLEEP:
		{
			LoRaWAN.Sleep();
			break;
		}
		default:
		{
			DeviceState = DEVICE_STATE_INIT;
			break;
		}
	}
}
