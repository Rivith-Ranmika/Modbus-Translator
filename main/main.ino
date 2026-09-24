#include <Arduino.h>
#include "ModbusClientRTU.h"
#include "HardwareSerial.h"
#include "ModbusServerRTU.h"
#include <Logging.h>


// Meter
#define ME_R0     4
#define ME_D1     1
#define ME_DE_RE  2
#define ME_BAUDRATE   9600
#define ME_ID     1

// Huawei
#define IN_R0     7
#define IN_D1     5
#define IN_DE_RE  6
#define IN_BAUDRATE   9600
#define IN_ID     11 //0x0B

int i = 0;
bool data_ready = false;

//DDS238 meter register map
uint16_t registermap[3][3] = {
  { 0, 2, 0 },   // Row 0   address, qunatity, position to store in Meter_RegData
  { 8, 4, 1 },   // Row 1
  { 12, 6, 3 }   // Row 2
};

// 0-TOTAL 1-EXPORT  2-IMPORT 3-VOLTAGE 4-CURRENT 5-APOWER 6-RPOWER 7-PFACTOR 8-FREQUENCY 9-empty
float Meter_RegData[10];  // to store Meter registers
int Divider[9]={
  100, //total
  100, //export
  100, //import
  10, //voltage
  100, //current
  1000, //apower
  1000, //rpower
  1000, //pfactor
  100 //frequency
};

int HuaweiTranslate[80]={
  4, //0  Current A
  9,9, //1,2  Current B,C
  3, // 3  Average V
  3, //4  Voltage A 
  9,9,9,9,9,9, //5,6,7,8,9,10  Vb,Vc,AVabc,Vab,Vbc,Vca
  8, //11  Frequency
  5, //12 Total Active power
  5, //13 A Active power
  9,9, //14,15 B,C Active power
  6,  //16 Total reactive power
  6,  //17  A reactive power
  9,9,  //18,19 B,C reactive power
  9,9,9,9,  //20,21,22,23 tot,A,B,C apparent power
  7, //24  Total power factor
  7,  //25  A power factor
  9,9,9,9,9,9,  //25,26,27, tot,A,B,C pactor  28,29,30,31 imp,exp reactive energy sources conflict
  2, //32  import energy
  9,9,9,  //33,34,35  ??
  1,  //36  export energy
  9,9,9, //37,38,39  ??
};

//	Create a Modbus Master RE/DE control pin ME_DE_RE
ModbusClientRTU MeterMaster(ME_DE_RE);
//	Create a Modbus Slave with 20000ms timeout RE/DE control pin IN_DE_RE 
ModbusServerRTU HuaweiSlave(2000, IN_DE_RE);  

// Define an onError handler function to receive error responses
void handleError(Error error, uint32_t token){
  ModbusError me(error);
  Serial.printf("Modbus ERROR: %02X - %s\n", (int)me, (const char*)me);
  data_ready = false;
}

//For read meter
void handleData(ModbusMessage response, uint32_t token){
  Serial.println();
  Serial.println("===== Read meter =====");
  // Serial.print("Raw: ");
  // for (auto byte : response){
  //   Serial.printf("%02X ", byte);
  // }
  uint16_t offs = 3;//read start from 4th byte
  uint8_t qun;
  
  int position = 0;
  response.get(2, qun);//response quntity

  if (response.size() >= 5 && response.getFunctionCode() == 0x03 && qun>0){
    uint16_t pass;
    data_ready = true;
    Serial.printf("Response Quntity %d", qun);
    //set Meter_RegData positon
    switch (token) {
      case 0:
        position = registermap[0][2];
        break;
      case 8:
        position = registermap[1][2];
        break;
      case 12:
        position = registermap[2][2];
        break;
      default:
        Serial.printf("error read from Meter  HEX %x: ",qun);
        data_ready= false;
        while(Serial1.available()){
        Serial1.read();}
        Serial.println("reseted");
        break;
    }
    if(token==0 || token==8){//find 32-bit data
      for(uint8_t i = 0; i < qun/4; i++){
        uint32_t com = ((uint32_t)response[offs] << 24) | //convert 2 16-bit data to one 32-bit data
                       ((uint32_t)response[offs+1] << 16) | 
                       ((uint32_t)response[offs+2] << 8)  | 
                       (uint32_t)response[offs+3];
        Meter_RegData[position] = (float)com / Divider[position];//save as float
        position++;
        offs = offs+4;
      }
    }
    else{//16-bit data
      for(uint8_t i = 0; i < qun/2; i++){
        offs = response.get(offs, pass);//store as 16-bit
        if(i==2 || i ==3){//find the negative value
          int16_t x = (int16_t)pass;//covert to sined int
          Meter_RegData[position] = (float)x/Divider[position];//save as float
        }
        else {
          Meter_RegData[position] = (float)pass/Divider[position];//save as float
        }
        position++;
      }
    }
  }
  Serial.println();
  Serial.println("===========================");
}

//For sent data
ModbusMessage handleReadHolding(ModbusMessage request){
  uint16_t address;
  uint16_t words;
  ModbusMessage response;
  // Get requested register and number of registers
  request.get(2, address);
  request.get(4, words);
  
  if (data_ready==true){//check data avalable
    Serial.println("==========DATA SENT==========");
    // Create Modbus response
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));

    if ((address == 0x07D1)&& (words==0x01)){//  meter status
      response.add(uint16_t(0x3B11)); 
      return response;
    }
    else if((address==0x0836)&& (words=0x50)) { //  first request
      for (uint16_t i = 0; i < words/2; i++) {
        response.add(Meter_RegData[HuaweiTranslate[i]]);  // translate meter adresses on to Huawei addresses
      }
      return response;
    }
    else if ((address == 0x08A6)&& (words==0x0A)) {   //  read meter status data
      for (uint16_t i = 0; i <  words/2; i++) {
        response.add(0x0000);  // all registers set to zero
      }
    }
    else{ //Set up error response
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      Serial.printf("ERROR bad address= %X \n", address);
      return response;
    }
  }
  else{
    //send meter error register
    Serial.println("Data not ready");
    response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
    return response;
  }
}

void setup() {
  Serial1.setRxBufferSize(512);
  Serial.begin(115200);
  delay(1000);

  //Setup the Master Serial Port (UART1)
  RTUutils::prepareHardwareSerial(Serial1);
  Serial1.begin(9600, SERIAL_8N1, ME_R0, ME_D1);
  while (!Serial1) {Serial.println("Meter port is not ready");}
	Serial.print("Serial1 OK __  ");
  MeterMaster.onDataHandler(&handleData);   // Set up ModbusRTU client - provide onData handler function	
  MeterMaster.onErrorHandler(&handleError);   // provide onError handler function
  MeterMaster.setTimeout(2000);   // Set message timeout to 2000ms
  MeterMaster.begin(Serial1,0);   // Start ModbusRTU background task on CPU 0


  //Setup the Slave Serial Port (UART2)
  RTUutils::prepareHardwareSerial(Serial2);
  Serial2.begin(9600, SERIAL_8N1, IN_R0, IN_D1);
  while (!Serial2) {Serial.println("Inverter port is not ready");}
	Serial.println("Serial2 OK __");
  HuaweiSlave.registerWorker(IN_ID,READ_HOLD_REGISTER,&handleReadHolding); // Register served function code worker for server 11, FC 0x03
  HuaweiSlave.begin(Serial2,1); // Start ModbusRTU background task on CPU 1

}
unsigned long previousMillis = 0;
//cyclically request the data from Meter
void loop() {
  static uint32_t token = 1;
  Meter_RegData[9] = 0/10;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 400) {
    previousMillis = currentMillis;
    if(i>2){i=0;}
    uint16_t addr = registermap[i][0];
    uint16_t len  = registermap[i][1];
    token = addr;
    Error err = MeterMaster.addRequest(token, ME_ID,READ_HOLD_REGISTER, addr, len);
    if (err != SUCCESS)
    {
      ModbusError e(err);
      Serial.printf("Request error: %02X - %s\n", (int)e, (const char *)e);
      data_ready = false;
    }
    i++;
  }

}
