/*
 Name:		BasementSensor.ino
 Created:	8/10/2016 10:48:42 PM
 Author:	tl1
*/

#include <AltSoftSerial.h>
#include <XBee.h>
#include <HTU21D.h>
#include <Wire.h>

// XBee variables
AltSoftSerial xbeeSerial;  // The software serial port for communicating with the Xbee (TX Pin 9, RX Pin 8)
XBee localRadio = XBee();  // The connection for the local coordinating radio

// XBee command codes
const uint8_t CMD_SENSOR_DATA = 4;

// XBee data codes
const uint8_t TEMPERATURE_CODE = 1;
const uint8_t LUMINOSITY_CODE = 2;
const uint8_t PRESSURE_CODE = 3;
const uint8_t HUMIDITY_CODE = 4;
const uint8_t POWER_CODE = 5;
const uint8_t LUX_CODE = 6;
const uint8_t HEATING_CODE = 7;
const uint8_t THERMOSTAT_CODE = 8;
const uint8_t TEMP_12BYTE_CODE = 9;
const uint8_t BATTERY_SOC_CODE = 10;

// Timing variables
const unsigned long SENSOR_DELAY = 600000;	// The sleep period (ms)

// Sensor objects
HTU21D airSensor;

// Union for conversion of numbers to byte arrays
union FloatConverter {
	float f;
	uint8_t b[sizeof(float)];
};

// Functions
void Message(String msg);

//=============================================================================
// SETUP
//=============================================================================
void setup() {
	// Start the I2C interface
	airSensor.begin();

	// Setup the serial communications
	Serial.begin(9600);
	Message("Starting Serial...");

	// Connect to the XBee
	Message("Starting XBee connection...");
	xbeeSerial.begin(9600);
	localRadio.setSerial(xbeeSerial);
	Message("FINISHED");
}

//=============================================================================
// MAIN LOOP
//=============================================================================
void loop() {
	//-------------------------------------------------------------------------
	// COLLECT SENSOR DATA
	//-------------------------------------------------------------------------
	// Read the temperature
	Message("Reading tempeature");
	FloatConverter Temperature;
	Temperature.f = airSensor.readTemperature();

	// Read the humidity
	Message("Reading humidity");
	FloatConverter Humidity;
	float raw_humidity = airSensor.readHumidity();
	//	Humidity.f = raw_humidity - 0.15*(25.0 - Temperature.f);	// Correction for HTU21D from spec sheet
	Humidity.f = raw_humidity;

	// Read Power
	Message("Reading Power");
	FloatConverter Power;
	Power.f = 5.0;

	//-------------------------------------------------------------------------
	// SEND DATA THROUGH XBEE
	//-------------------------------------------------------------------------
	// Create the byte array to pass to the XBee
	Message("Creating XBee data transmission");
	size_t floatBytes = sizeof(float);
	uint8_t package[1 + 3 * (floatBytes + 1)];
	package[0] = CMD_SENSOR_DATA;
	package[1] = TEMPERATURE_CODE;
	package[1 + (floatBytes + 1)] = HUMIDITY_CODE;
	package[1 + 2 * (floatBytes + 1)] = POWER_CODE;
	for(int i = 0; i < floatBytes; i++) {
		package[i + 2] = Temperature.b[i];
		package[i + 2 + (floatBytes + 1)] = Humidity.b[i];
		package[i + 2 + 2 * (floatBytes + 1)] = Power.b[i];
	}

	// Create the message text for debugging output
	String xbee_message = "Sent the following message(" + String(Temperature.f) + "," + Humidity.f + "," + Power.f + "): ";
	for(int i = 0; i < sizeof(package); i++) {
		if(i != 0) xbee_message += "-";
		xbee_message += String(package[i], HEX);
	}
	Message(xbee_message);

	// Send the data package to the coordinator through the XBee
	Message("Sending XBee transmission");
	XBeeAddress64 address = XBeeAddress64(0x00000000, 0x00000000);
	ZBTxRequest zbTX = ZBTxRequest(address, package, sizeof(package));
	localRadio.send(zbTX);

	// Transmission delay
	Message("XBee message sent");
	delay(SENSOR_DELAY);
}


//=============================================================================
// Message
//=============================================================================
void Message(String msg) {
	Serial.print(millis());
	Serial.print(": ");
	Serial.println(msg);
}