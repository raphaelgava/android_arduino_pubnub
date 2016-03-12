/*
Arduino communicates with both the W5100 and SD card using the SPI bus (through the 
ICSP header). This is on digital pins 10, 11, 12, and 13 on the Uno and pins 50, 51, 
and 52 on the Mega. On both boards, pin 10 is used to select the W5100 and pin 4 for 
the SD card. These pins cannot be used for general I/O. On the Mega, the hardware SS 
pin, 53, is not used to select either the W5100 or the SD card, but it must be kept 
as an output or the SPI interface won't work.

Note that because the W5100 and SD card share the SPI bus, only one can be active at 
a time. If you are using both peripherals in your program, this should be taken care 
of by the corresponding libraries. If you're not using one of the peripherals in your 
program, however, you'll need to explicitly deselect it. To do this with the SD card, 
set pin 4 as an output and write a high to it. For the W5100, set digital pin 10 as a
high output.
*/

#include <Ethernet.h>
#include <ArduinoJson.h>
#include "PubNub.h"

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192,168,1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

EthernetClient *client;
//const byte numClientes = 3;
// telnet defaults to port 23
//EthernetServer server(23);
//EthernetClient client;
//EthernetClient clients[numClientes];

// Pins configuration
const byte pinLedVerde = 5;
const byte pinBotaoVerde = 8;

const byte pinLedAmarelo = 6;
const byte pinBotaoAmarelo = 7;

const byte pinLedVermelho = 9;
const byte pinBotaoVermelho = 3;

const byte bitPos[] = {0x01, 0x02, 0x04};

byte estadoAtual;
byte estado;
bool valor;

long lastDebounceTime;  // the last time the output pin was toggled
const long debounceDelay = 1000;    // the debounce time; increase if the output flickers
String leitura;

//Configuração PubNub
const static char pubkey[] = "pub-c-f9671646-b308-4006-9ed1-ea7177b4e42e";//demo";//"pub-c-24a86643-ecbc-40b3-ad4f-18f4b24f568e";//
const static char subkey[] = "sub-c-f83d4e6a-e260-11e5-a25a-02ee2ddab7fe";//demo";//"sub-c-27383722-e188-11e5-ba64-0619f8945a4f";//
const static char channel[] = "led1";//"ANDRUINO";//"Arduino";//"hello_world"//;

//Configuração JSON
StaticJsonBuffer<200> jsonBuffer;

void setup() {
  // make the pins outputs:
  pinMode(pinLedVerde, OUTPUT);
  pinMode(pinLedAmarelo, OUTPUT);
  pinMode(pinLedVermelho, OUTPUT);
  
  // make the pins inputs:
  //pinMode(pinBotaoVerde, INPUT);
  //digitalWrite(pinBotaoVerde, HIGH);
  pinMode(pinBotaoVerde, INPUT_PULLUP);
  
  //pinMode(pinBotaoAmarelo, INPUT);
  //digitalWrite(pinBotaoAmarelo, HIGH);
  pinMode(pinBotaoAmarelo, INPUT_PULLUP);
  
  //pinMode(pinBotaoVermelho, INPUT);
  //digitalWrite(pinBotaoVermelho, HIGH);
  pinMode(pinBotaoVermelho, INPUT_PULLUP);

  valor = 0;

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  // this check is only needed on the Leonardo:
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // start the Ethernet connection:
  Serial.println("Trying to get an IP address using DHCP");
  while (!Ethernet.begin(mac)) {
    Serial.println("Failed to configure Ethernet using DHCP");
    delay(1000);
  }
//  if (Ethernet.begin(mac) == 0) {
//    Serial.println("Failed to configure Ethernet using DHCP");
//    // initialize the Ethernet device not using DHCP:
//    Ethernet.begin(mac, ip, myDns, gateway, subnet);
//  }
  // print your local IP address:
  Serial.print("My IP address: ");
  ip = Ethernet.localIP();
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(ip[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
  // start listening for clients
//  server.begin();

  // start comunication
  PubNub.begin(pubkey, subkey);

  lastDebounceTime = millis();
  estado = 0;
  estadoAtual = 0;

  leitura = "";
}

String ImprimirValor(byte valor)
{
  byte teste;
  String texto;

  texto = "";
  if (valor > 100)
  {
    teste =  valor / 100;
    valor = valor % 100;
    texto.concat(teste);
    Serial.print(teste);
  }

  if (valor > 10)
  {
    teste =  valor / 10;
    valor = valor % 10;
    texto.concat(teste);
    Serial.print(teste);
  }

  texto.concat(valor);  
  Serial.print(valor);
  return texto;
}

void AtualizarValorTCP(byte led, byte valor)
{
  bool enviar = true;
  
  jsonBuffer = StaticJsonBuffer<200>();
  JsonObject& root = jsonBuffer.createObject();
  
  // Add values in the object
  //
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do root.set<long>("time", 1351824120);
  switch (led)
  {
    case 0:
      root["bt1"] = ImprimirValor(valor);
    break;
    
    case 1:
      root["bt2"] = ImprimirValor(valor);
    break;
    
    case 2:
      root["bt3"] = ImprimirValor(valor);
    break;
    
    default:
      enviar = false;
  }

  if (enviar)
  {
   char ra[255];
   Serial.println("publishing a message");
   root.printTo(ra, sizeof(ra));
   root.printTo(Serial);
   Serial.println();
   client = PubNub.publish(channel, ra);  
   if (!client) {
      Serial.println("publishing error");
      delay(1000);
      return;
   }
   client->stop();
  }
}

void Debounce()
{
  valor = digitalRead(pinBotaoVerde);
  delay(1);

  estado = valor;
  estado <<= 1;
  
  valor = digitalRead(pinBotaoAmarelo);
  delay(1);

  estado |= valor;
  estado <<= 1;
  
  valor = digitalRead(pinBotaoVermelho);
  delay(1);

  estado |= valor;

  if ((millis() - lastDebounceTime) > debounceDelay) {
    Serial.println("CCCC!");
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (estado != estadoAtual) {  
      Serial.println("BLAAAAAA!");
      for (byte i = 0; i < sizeof(bitPos); i++)
      {
        if ((estado & bitPos[i]) != (estadoAtual & bitPos[i]))
        {
          valor = !(estado & bitPos[i]); 
//          if (valor > 0)
//          {
//            valor = 255;
//          }
          switch(i)
          {
            case 0:            
//                digitalWrite(pinLedVerde, valor);                
                Serial.print("Verde: ");
            break;
  
            case 1:
//              digitalWrite(pinLedAmarelo, valor);
              Serial.print("Amarelo: ");
            break;
  
            case 2:
              digitalWrite(pinLedVermelho, valor);
              Serial.print("Vermelho: ");
            break;
          }
          Serial.println(valor); // somente para debug
          AtualizarValorTCP(i, valor);
        }
      }

      estadoAtual = estado;
    }
    
    lastDebounceTime = millis();
  }
}

void ParserLed(String led, byte porta)
{
  if (led != "")
  {
    byte ledVal= led.toInt();
    analogWrite(porta, ledVal);
    ImprimirValor(ledVal);
  }
}

//void Parser(char thisChar)
void Parser()
{
  Serial.println("1");
  PubSubClient *pclient = PubNub.subscribe(channel);
  if (pclient)
  {
    Serial.println("2");
    jsonBuffer = StaticJsonBuffer<200>();
    Serial.println("3");
    leitura = "";
    while (pclient->wait_for_data(1)) {
      Serial.println("4");
      char c = pclient->read();
      leitura += c;
      Serial.println("5");
    }
    Serial.println("6");
    Serial.println(leitura);
//  
//    JsonArray& root1 = jsonBuffer.parseArray(leitura);
//    if (root1.success())
//    {
//////      Serial.println("tamalho");
//////      Serial.println(sizeof(root1));String json1 = root1[0];
//////      Serial.println(json1);
////      int i = 0;
//////      for (int i = 0; i < (sizeof(root1)); i++)
////      bool ok = true;
////      while(ok)
////      {
////        String json = root1[i]; 
////        if (json == "")
////          break;   
////
////        Serial.println("a");
////        Serial.println(json);
////        jsonBuffer = StaticJsonBuffer<200>();
////        JsonObject& root2 = jsonBuffer.parseObject(json);
////        Serial.println("b");
////        if (root2.success())
////        {
////          Serial.println("c");
////          ParserLed(root2["led1"], pinLedVerde);
////          ParserLed(root2["led2"], pinLedAmarelo);
////          ParserLed(root2["led3"], pinLedVermelho);
////          Serial.println("d");
////        }
////        else
////        {
////          Serial.println("Erro ao parsear o objeto!");
////        }
////        i++;
////      }
//    }
//    else
//    {
//      Serial.println("Erro ao parsear o array!");
//    }
  Serial.println("teste!");
    pclient->stop();
  }
  Serial.println("fim");
}

void loop() {  
  Ethernet.maintain();
  
  Debounce();

  Parser();
  
  delay(100);
}
