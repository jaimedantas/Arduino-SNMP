/**
* DC Control - Jaime Danta <https://github.com/jaimedantas>
* SNMP Agent running in Arduino Uno 
* Version 1.0
* Sensor DHT22, LCD 20X4
* SNMP Agent Library based on Agentuino and modifies by Jaime Dantas
* Copyright 2010 Eric C. Gionet <lavco_eg@hotmail.com>
*
*/
#include <Streaming.h>         
#include <Ethernet.h>          
#include <SPI.h>
#include <Agentuino.h> 
#include <dht.h>
#include <math.h>
#include <String.h>
#include <LiquidCrystal.h>


#define DHT11_PIN 7
#define RELAY_1_AR 3
#define RELAY_2_PORTA 4
#define TERMISTOR_PIN A0
#define BOTAO_PIN 5


//**OBS: Cumminity e Porta sap mudadas na biblioteca

LiquidCrystal lcd(9, 8, A4, A3, A2, A1);//old configuration
//LiquidCrystal lcd(9, 8, A2, A1, 6, 2);


float Temp;
static uint32_t generalTime=0;
String IP = "192.168.70.90";
String mac_lcd = "DE:AD:BE:EF:EE:DE";
String Porta = "162";
String firmware = "08/12/2016";
static char TemperatiraDHT[10]        = "";
static char TemperaturraTermistor[10] = "";
static char Humidade[10]              = "";
static char Ar[10]                     = "";
static char isPorta[5]                = "";
static char mensagem[15]              = "";
String Auxiliar                       = "";

static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xEE, 0xDE };
static byte ip[] = { 192, 168, 70, 90 };
static byte gateway[] = { 192, 168, 70, 1 };
static byte subnet[] = { 255, 255, 255, 0 };


//comandos do Ar Condicionado
const static char ligaAR[] PROGMEM      = "1.3.6.1.2.1.9862.1.0"; //liga ar
const static char desligaAR[] PROGMEM   = "1.3.6.1.2.1.9862.0.0"; //desliga ar

//comando da Porta
const static char abrePORTA[] PROGMEM   = "1.3.6.1.2.1.3716.1.0";  //abre porta
const static char fechaPORTA[] PROGMEM  = "1.3.6.1.2.1.3716.0.0";  //fecha porta

//comando de consulta
const static char snmpTEMP1[] PROGMEM   = "1.3.6.1.2.1.6540.1.0";  //consulta temperatura 1
const static char snmpTEMP2[] PROGMEM   = "1.3.6.1.2.1.6540.2.0";  //consulta temperatura 2
const static char snmpHUMIDADE[] PROGMEM   = "1.3.6.1.2.1.6540.3.0";  //consulta humidade
const static char snmpOUTPUTS[] PROGMEM = "1.3.6.1.2.1.6540.4.0";  //consulta estado de saidas  | Porta | Ar |



//comandos nativos
const static char sysUpTime[] PROGMEM   = "1.3.6.1.2.1.1.3.0";  //tempo ligado
const static char sysName[] PROGMEM     = "1.3.6.1.2.1.1.5.0";  //nome da estacao

static uint32_t locUpTime           = 0;    // read-only (static) 
static char locName[20]             = "Estacao_Pop-RN";  // should be stored/read from EEPROM - read/write (not done for simplicity)
static char upTimeString[20]             = "";  // envia o tempo em string
byte termometru[8] = //icon for termometer
{
    B00100,
    B01010,
    B01010,
    B01110,
    B01110,
    B11111,
    B11111,
    B01110
};

byte picatura[8] = //icon for water droplet
{
    B00100,
    B00100,
    B01010,
    B01010,
    B10001,
    B10001,
    B10001,
    B01110,
};
byte customChar[8] = {
  B00000,
  B00100,
  B10101,
  B01110,
  B01010,
  B01110,
  B10101,
  B00100
};

byte customChar2[8] = {
  B11111,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111
};


int tela=1;
uint32_t prevMillis = millis();
char oid[SNMP_MAX_OID_LEN];
SNMP_API_STAT_CODES api_status;
SNMP_ERR_CODES status;

dht DHT;

void pduReceived()
{
  SNMP_PDU pdu;
  api_status = Agentuino.requestPdu(&pdu);
  if ( pdu.type == SNMP_PDU_GET && pdu.error == SNMP_ERR_NO_ERROR && api_status == SNMP_API_STAT_SUCCESS ) {
    
    pdu.OID.toString(oid);

    if ( strcmp_P(oid, ligaAR ) == 0 ) {//------------LIGAR AR
        digitalWrite(RELAY_1_AR, HIGH);
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, Ar);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    else if ( strcmp_P(oid, desligaAR ) == 0 ) {//------------DESLIGAR AR
        digitalWrite(RELAY_1_AR, LOW);
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, Ar);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    else if ( strcmp_P(oid, abrePORTA ) == 0 ) {//------------ABRE PORTA
        digitalWrite(RELAY_2_PORTA, HIGH);
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, isPorta);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    else if ( strcmp_P(oid, fechaPORTA ) == 0 ) {//------------FECHA PORTA
        digitalWrite(RELAY_2_PORTA, LOW);
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, isPorta);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    else if ( strcmp_P(oid, snmpTEMP1 ) == 0 ) {//------------TEMP 1
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, TemperatiraDHT);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    else if ( strcmp_P(oid, snmpTEMP2 ) == 0 ) {//------------TEMP 2
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, TemperaturraTermistor);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    else if ( strcmp_P(oid, snmpHUMIDADE ) == 0 ) {//------------HUMIDADE
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, Humidade);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    else if ( strcmp_P(oid, snmpOUTPUTS ) == 0 ) {//------------SAIDAS 
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, mensagem);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    else if ( strcmp_P(oid, sysUpTime ) == 0 ) {//------------TEMPO LIGADO
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, upTimeString);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    else if ( strcmp_P(oid, sysName ) == 0 ) {//------------NOME
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locName);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    else {
      // oid does not exist
      // response packet - object not found
      pdu.type = SNMP_PDU_RESPONSE;
      pdu.error = SNMP_ERR_NO_SUCH_NAME;
    }
    Agentuino.responsePdu(&pdu);
  }
  
  Agentuino.freePdu(&pdu);
  
}
//----------------------------------------LCD
void MostrarNoLCD(int valorTela){ 
  switch(valorTela){
    case 1:
      lcd.createChar(1,termometru);
      lcd.clear();//remover clean mode
      lcd.begin(20, 4);
      lcd.setCursor(0,0); 
      lcd.print("Temp. do Sensor:");
      lcd.setCursor(1,1);
      lcd.write((uint8_t)1);//mudar depois pra print
      lcd.setCursor(5, 1);
      lcd.print(TemperatiraDHT);
      lcd.print((char)223);
      lcd.print("C");
      lcd.setCursor(1,2); 
      lcd.print("Temp. do Equip.:");
      lcd.setCursor(1,3);
      lcd.write((uint8_t)1);
      lcd.setCursor(5, 3);
      lcd.print(TemperaturraTermistor);
      lcd.print((char)223);
      lcd.print("C");
      break;
    case 2: 
      lcd.createChar(2,picatura); 
      lcd.clear();//remover clean mode
      //delay(10);
      lcd.begin(20, 4);
      lcd.setCursor(0,0); 
      lcd.print("                ");
      lcd.setCursor(0,1); 
      lcd.print("                ");
      lcd.setCursor(0,0); 
      lcd.print("Humidade:       ");
      lcd.setCursor(1,1);
      lcd.write((uint8_t)2);
      lcd.setCursor(5, 1);
      lcd.print(Humidade);
      lcd.print("%");
      break;
    case 3:
      lcd.createChar(3,customChar);//AR
      lcd.createChar(4,customChar2);
      lcd.clear();//remover clean mode
      //delay(10);
      lcd.begin(20, 4);
      lcd.setCursor(0,0);
      lcd.print("Ar Cand. Aux:   ");
      lcd.setCursor(0,1);
      if(bitRead(PORTD,3) == 1){
          lcd.setCursor(1,1);
          lcd.write((uint8_t)3);
          lcd.print("   Ligado");
      }
      else if(bitRead(PORTD,3) == 0){
          lcd.setCursor(1,1);
          lcd.write((uint8_t)3);
          lcd.print("  Desligado");
      }
      lcd.setCursor(0,2);
      lcd.print("Porta do Rack:  ");
      lcd.setCursor(0,3);
      if(bitRead(PORTD,4) == 1){
          lcd.setCursor(1,3);
          lcd.write((uint8_t)4);
          lcd.print("   Aberta");
      }
      else if(bitRead(PORTD,4) == 0){
          lcd.setCursor(1,3);
          lcd.write((uint8_t)4);
          lcd.print("  Fechada");
      }
      break;
     case 4:
      lcd.clear();//remover clean mode
      lcd.begin(20, 4);
      lcd.setCursor(0,0);
      lcd.print("IP:");
      lcd.print(IP);
      lcd.setCursor(0,1);
      lcd.print("Porta: ");
      lcd.print(Porta);
      lcd.setCursor(0,2);
      lcd.print("MAC: ");
      lcd.setCursor(0,3);
      lcd.print(mac_lcd);  
      break;
     case 5:
      lcd.clear();//remover clean mode
      //delay(10);
      lcd.begin(20, 4);
      lcd.setCursor(0,0); 
      lcd.print("                ");
      lcd.setCursor(0,1); 
      lcd.print("                ");
      lcd.setCursor(0,0);
      lcd.print("Ultimo Boot:    ");
      lcd.setCursor(0,1);
      lcd.print(locUpTime);
      lcd.print(" seg.");
      lcd.setCursor(0,2);
      lcd.print("Firmware:       ");
      lcd.setCursor(0,3);
      lcd.print(firmware);
      break;
     case 6:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Nome:           ");
      lcd.setCursor(0,1);
      lcd.print(locName);
      break;
     case 7:
      lcd.clear();//remover clean mode
      //delay(10);
      lcd.begin(20, 4);
      lcd.setCursor(0,1);
      lcd.print("   DC Control   ");
      lcd.setCursor(0,2);
      lcd.print("    PoP - RN    ");
      break;
  }
}

//----------------------------------------
float Termistor() {
  Temp = log(9060.0*((1024.0/analogRead(TERMISTOR_PIN)-1))); //deu problema no resistor, o real eh 4,7k
  //         =log(10000.0/(1024.0/RawADC-1)) // for pull-up configuration
  Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
  Temp = Temp - 273.15;            // Convert Kelvin to Celcius         // Convert Kelvin to Celcius
  double multiplier = pow(10, 1 || 0);
  Temp = round(Temp * multiplier) / multiplier;
  return Temp;
}

void setup()
{
  Ethernet.begin(mac, ip);
  api_status = Agentuino.begin();
  delay(10);
  pinMode(RELAY_1_AR, OUTPUT);
  pinMode(RELAY_2_PORTA, OUTPUT);
  pinMode(TERMISTOR_PIN, INPUT);
  pinMode(BOTAO_PIN, INPUT);
  DDRC |= B00000110;
  DDRB |= B00000011;
  digitalWrite(RELAY_1_AR, LOW);// off
  digitalWrite(RELAY_2_PORTA, HIGH);// open
  digitalWrite(BOTAO_PIN, HIGH);// pull-up
  lcd.createChar(1,termometru);
  lcd.createChar(2,picatura); 
  lcd.createChar(3,customChar);
  lcd.createChar(4,customChar2);
  lcd.begin(20, 4);
  lcd.setCursor(0,0);
  lcd.print("DC Control v1.0");
  if ( api_status == SNMP_API_STAT_SUCCESS ) {
    Agentuino.onPduReceive(pduReceived);
    delay(10);
    return;
  }

}


void loop()
{
  //digitalWrite(RELAY_2_PORTA, HIGH);
  //tem q fazer a leitura dos dados
  if(locUpTime - generalTime > 3){
    int chk = DHT.read22(DHT11_PIN);
    dtostrf(DHT.temperature, 4,1,TemperatiraDHT);
    dtostrf(DHT.humidity, 4,1,Humidade);
    generalTime=locUpTime;
  }
  dtostrf(Termistor(), 4,1,TemperaturraTermistor);
  dtostrf(bitRead(PORTD,3), 2,0, Ar);
  dtostrf(bitRead(PORTD,4), 2,0,isPorta);
  //Auxiliar = Ar + isPorta;
  sprintf(mensagem, "AR %s | PORTA %s" ,Ar, isPorta);
  Agentuino.listen();
  // sysUpTime - The time (in hundredths of a second) since
  // the network management portion of the system was last
  // re-initialized.
  //if ( millis() - prevMillis > 1000 ) {
    locUpTime = millis()/1000;
    dtostrf(locUpTime, 10,0,upTimeString);
  //}
  if(!digitalRead(BOTAO_PIN)){
    lcd.clear();
    MostrarNoLCD(tela);
    delay(500);
    tela++;
    if(tela>7) tela=1;
  }
}


