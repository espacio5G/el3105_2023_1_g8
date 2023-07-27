#include <StarterKitNB.h>
#include <SparkFun_SHTC3.h>
#include <Arduino.h>
#define NO_OF_SAMPLES   32   

#define PIN_VBAT WB_A0

#define VBAT_MV_PER_LSB (0.73242188F) // 3.0V ADC range and 12 - bit ADC resolution = 3000mV / 4096
#define VBAT_DIVIDER_COMP (1.73)      // Compensation factor for the VBAT divider, depend on the board
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)

SHTC3 mySHTC3;  // Variable que guarda los datos del sensor
StarterKitNB sk;  // Variable para la funcionalidad del Starter Kit

// NB
String apn = "m2m.entel.cl";
String band = "B28 LTE";

// Thingsboard
String ClientIdTB = "grupo8";
String usernameTB = "88888";
String passwordTB = "88888";
String msg = "";
String resp = "";
String Operador = "";
String Banda = "";
String Red = "";



void errorDecoder(SHTC3_Status_TypeDef message)             // Función que facilita la detección de errores
{
  switch(message)
  {
    case SHTC3_Status_Nominal : Serial.print("Nominal"); break;
    case SHTC3_Status_Error : Serial.print("Error"); break;
    case SHTC3_Status_CRC_Fail : Serial.print("CRC Fail"); break;
    default : Serial.print("Unknown return code"); break;
  }
}


void setup()
{
  pinMode(WB_IO1, OUTPUT | PULLUP);
	digitalWrite(WB_IO1, HIGH);

	adcAttachPin(WB_A1);

	analogSetAttenuation(ADC_11db);
	analogReadResolution(12);

	// Initialize Serial for debug output
	time_t timeout = millis();
	Serial.begin(115200);
	while (!Serial)
	{
		if ((millis() - timeout) < 5000)
		{
            delay(100);
        }
        else
        {
            break;
        }
	}


  sk.Setup();
  Wire.begin();
  sk.Connect(apn, band);
  Serial.print("Beginning sensor. Result = ");            
  errorDecoder(mySHTC3.begin());                         // Detectar errores
  Serial.println("\n\n");
}

void loop()
{
  int i;
	int sensor_pin = WB_A1; // select the input pin for the potentiometer
	int mcu_ain_raw=0;
	int average_adc_raw;
	float voltage_mcu_ain;  //mv as unit
	float current_sensor; // variable to store the value coming from the sensor

	for (i = 0; i < NO_OF_SAMPLES; i++)
	{
		mcu_ain_raw += analogRead(sensor_pin);
	}
	average_adc_raw = mcu_ain_raw / NO_OF_SAMPLES;

	delay(1000);


  if (!sk.ConnectionStatus()) // Si no hay conexion a NB
  {
    sk.Reconnect(apn, band);  // Se intenta reconecta
    delay(2000);
  }

  sk.ConnectBroker(ClientIdTB, usernameTB, passwordTB);  // Se conecta a ThingsBoard
  delay(2000);

  resp = sk.bg77_at((char *)"AT+COPS?", 500, false);
  Operador = resp.substring(resp.indexOf("\""), resp.indexOf("\"",resp.indexOf("\"")+1)+1);     // ENTEL PCS
  delay(1000);

  resp = sk.bg77_at((char *)"AT+QNWINFO", 500, false);
  Red = resp.substring(resp.indexOf("\""), resp.indexOf("\"",resp.indexOf("\"")+1)+1);          // NB o eMTC
  Banda = resp.substring(resp.lastIndexOf("\"",resp.lastIndexOf("\"")-1), resp.lastIndexOf("\"")+1); 
  delay(1000);

  SHTC3_Status_TypeDef result = mySHTC3.update();             // resultado actual si se necesita
  if(mySHTC3.lastStatus == SHTC3_Status_Nominal)              // Actualiza y printea los datos de Humedad y temperatura en °C
  {
    msg = 
    "{\"humidity\":"+String(mySHTC3.toPercent())
    +",\"temperature\":"+String(mySHTC3.toDegC())
    +",\"Agua\":"+String(mcu_ain_raw)
    +",\"battery\":"+String(round(analogRead(PIN_VBAT) * REAL_VBAT_MV_PER_LSB)/37)
    +",\"Operador\":"+Operador
    +",\"Banda de frecuencia\":"+Banda
    +",\"Red IoT\":"+Red+"}";
    Serial.println(msg);  
    sk.SendMessage(msg);
  }
  else
  {
    msg = 
    "{\"humidity\":\"ERROR\", \"temperature\":\"ERROR\",\"battery\":"
    +String(round(analogRead(PIN_VBAT) * REAL_VBAT_MV_PER_LSB)/37)
    +",\"Operador\":"+Operador
    +",\"Banda de frecuencia\":"+Banda
    +",\"Red IoT\":"+Red+"}";
    Serial.println(msg);
    sk.SendMessage(msg);
  }

  sk.DisconnectBroker();
    delay(5000);                                                 // Tiempo de espera para cada entrega de datos
  }