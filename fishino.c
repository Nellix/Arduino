#ifndef _FISHINO32_
#include <Flash.h>
#endif
#include <Fishino.h>
#include <SPI.h>
#include <dht11.h>
#include <SoftwareSerial.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <avr/wdt.h>


#define MY_SSID  "InfostradaWiFi-010202" //Router's ssid
#define MY_PASS "PASS"  //Router's password
#define IPADDR  192, 168,   1, 251

#define INIT_LIGHT_HOUR 20
#define INIT_LIGHT_MINUTE 00
#define INIT_LIGHT_SECOND 00
#define END_LIGHT_HOUR 14
#define END_LIGHT_MINUTE 00
#define END_LIGHT_SECOND 00

#define fotoresistenza A0
#define DHT11PIN 6


#define FREQ_VENT  600//10 min
#define FREQ_DATA 1800 //30 min
#define FREQ_IP 30
#define FREQ_RESET 14440

#ifdef IPADDR
IPAddress ip(IPADDR);
#endif

dht11 DHT11;
SoftwareSerial NSS(0,6);
FishinoClient client;
tmElements_t tm;
char server1[] = "fishinoexample.altervista.org";
int ventilation=0;


//Prototipi
int getUm();
int getTemp();
int getLuce();
void changeLEDSTATUS(char*);
void(* Reset)(void) = 0;

FishinoServer server(80);

struct LED
{
    const char *name;
    uint8_t port;
    bool state;
};

//Array del LED(Sensori) collegati a Fishiino
LED leds[] =
{
    { "LED1", 3, false }, //LED1 -> ON/OFF Luce
    { "LED2", 5, false }, //LED2 -> ON/OFF Ventilatore
};

const int numLeds = sizeof(leds) / sizeof(LED);
char ledName[30];


char* processLine(const char *buf)
{
    
    // the line must start with 'GET /?'
    if(strncmp(buf, "GET /?", 6))
        // oops, an error occurred!
        return ledName;
    
    // ok, advance to led name
    buf += 6;
    
    if((strncmp(buf,"RESET",5))==0)
    {
        Serial.print("RESET");
        
        Reset();
    }if((strncmp(buf,"GetValue",7))==0)
    {
        httpRequest();
        
    }else
    {
        // copia il nome del led
        char *pName = ledName;
        while(*buf && *buf != '=' && pName - ledName < 29)
            *pName++ = *buf++;
        *pName = 0;
        
        Serial.println();
        Serial.print("NOME : ");
        Serial.println(ledName);
        
        // search for led by name inside available leds
        for(uint8_t iLed = 0; iLed < numLeds; iLed++)
        {
            if(!strcmp(ledName, leds[iLed].name))
            {
                LED & led = leds[iLed];
                
                // found!! toggle it
                // trovato!! ne inverte il valore
                led.state = !led.state;
                
                // modifica l'uscita corrispondente
                digitalWrite(led.port, led.state);
                
                
                // termina il ciclo
                return ledName;
            }
        }
    }
    return ledName;
}


// this method makes a HTTP connection to the server:  //Funzione che invia i dati dei sensori al server
void httpRequest()
{
    
    client.stop();
    if (client.connect(server1, 80))
    {
        Serial << F("Invio dati al server ...\n");
        
        // send the HTTP PUT request:
        client << F("GET /Fishino/add_data.php?");
        client << F("umidita=");
        client.print(getUm());
        client << F("&&");
        client << F("temperature=");
        client.print(getTemp());
        client << F("&&");
        client << F("luce=");
        client.print(getLuce());
        client << F(" HTTP/1.1\r\n");
        client << F("Host: fishinoexample.altervista.org\r\n");
        //client << F("User-Agent: FishinoWiFi/1.1\r\n");
        client << F("Connection: close\r\n");
        client.println();
        
        Serial.print(client);
        
    }
    else
    {
        Serial << F("connection failed\n");
    }
}




void startLED()   //DA SISTEMARE
{
    if (RTC.read(tm))
    {
        if((tm.Hour>=INIT_LIGHT_HOUR) || tm.Hour<=END_LIGHT_HOUR )
        {
            digitalWrite(3,LOW);
            Serial.println("Alarm: --------- LIGTHS ON--------");
            ventilation = 0;
            leds[0].state=true;
        }else
        {
            digitalWrite(3,HIGH);
            Serial.println("Alarm: --------- LIGTHS OFF--------");
            leds[0].state=false;
        }}
}


void setStatusLED()
{
    client.stop();
    if (client.connect(server1, 80))
    {
        Serial << F("connecting GET STATUS...\n");
        
        client << F("GET /Fishino/add_data.php?");
        client << F("setLEDOFF=1");
        client << F(" HTTP/1.1\r\n");
        client << F("Host: fishinoexample.altervista.org\r\n");
        //client << F("User-Agent: FishinoWiFi/1.1\r\n");
        client << F("Connection: close\r\n");
        client.println();
        
        Serial.print(client);
    }
    else
    {
        Serial << F("connection failed\n");
    }
}



void lastReset()
{
    client.stop();
    if (client.connect(server1, 80))
    {
        Serial << F("connecting GET STATUS...\n");
        if (RTC.read(tm))
        {
            client << F("GET /Fishino/add_data.php?");
            client << F("insertLR=1");
            client << F("&&");
            client << F("ora=");
            client.print(tm.Hour);
            client << F("&&");
            client << F("minuti=");
            client.print(tm.Minute);
            client << F("&&");
            client << F("secondi=");
            client.print(tm.Second);
            client << F("&&");
            client << F("giorno=");
            client.print(tm.Day);
            client << F("&&");
            client << F("mese=");
            client.print(tm.Month);
            client << F("&&");
            client << F("anno=");
            client.print(tm.Year);
            
            Serial.println(tm.Hour);
            Serial.println(tm.Minute);
            Serial.println(tm.Second);
            Serial.println(tm.Day);
            Serial.println(tm.Month);
            Serial.println(tm.Year);
            client << F(" HTTP/1.1\r\n");
            client << F("Host: fishinoexample.altervista.org\r\n");
            //client << F("User-Agent: FishinoWiFi/1.1\r\n");
            client << F("Connection: close\r\n");
            client.println();
            
            Serial.print(client);
        }
    }
    else
    {
        Serial << F("connection failed\n");
    }
}


void changeLEDSTATUS(char* led)
{
    client.stop();
    if (client.connect(server1, 80))
    {
        Serial << F("Aggiorno stato LED sul server \n");
        
        client << F("GET /Fishino/add_data.php?");
        client << F("changeLEDSTATUS=");
        client.print(led);
        //  Serial.print("LED DA CAMBIARE : ");
        //  Serial.print(led);
        client << F(" HTTP/1.1\r\n");
        client << F("Host: fishinoexample.altervista.org\r\n");
        //client << F("User-Agent: FishinoWiFi/1.1\r\n");
        client << F("Connection: close\r\n");
        client.println();
        
        Serial.print(client);
        
    }
    else
    {
        Serial << F("connection failed\n");
    }
    
}



void updateip()
{
    
    client.stop();
    
    // if there's a successful connection:
    // se la connessione è riuscita:
    if (client.connect("api.ipify.org", 80))
    {
        Serial << F("connecting...\n");
        
        // send the HTTP PUT request:
        // invia la richiesta HTTP:
        client << F("GET / HTTP/1.1\r\n");
        client << F("Host: api.ipify.org\r\n");
        client << F("User-Agent: FishinoWiFi/1.1\r\n");
        client << F("Connection: close\r\n");
        client.println();
    }
    else
    {
        // if you couldn't make a connection:
        // se la connessione non è riuscita:
        Serial << F("connection failed\n");
    }
    delay(1000);
    // buffer for GET line
    char buf[15];
    char b[15];
    uint8_t iBuf = 0;
    int n=0,s=0;
    int ip1,ip2,ip3,ip4;
    
    while (client.available())
    {
        char c = client.read();
        
        // if we're reading the first line, appena the char to it
        if(iBuf > 152 && n<15)
            buf[n++] = c;
        else
            iBuf++;
        
        
    }
    
    sscanf(buf, "%d.%d.%d.%d", &ip1,&ip2,&ip3,&ip4);
    
    
    client.stop();
    if (client.connect(server1, 80))
    {
        Serial << F("Invio dati al server ...\n");
        
        // send the HTTP PUT request:
        client << F("GET /Fishino/add_data.php?");
        client << F("ip1=");
        client.print(ip1);
        client << F("&&");
        client << F("ip2=");
        client.print(ip2);
        client << F("&&");
        client << F("ip3=");
        client.print(ip3);
        client << F("&&");
        client << F("ip4=");
        client.print(ip4);
        
        client << F(" HTTP/1.1\r\n");
        client << F("Host: fishinoexample.altervista.org\r\n");
        //client << F("User-Agent: FishinoWiFi/1.1\r\n");
        client << F("Connection: close\r\n");
        client.println();
        
        Serial.print(client);
        
    }
    else
    {
        Serial << F("connection failed\n");
    }
    
}



void printWifiStatus()
{
    Serial.print("SSID: ");
    Serial.println(Fishino.SSID());
    
    IPAddress ip = Fishino.localIP();
    Serial << F("IP Address: ");
    Serial.println(ip);
    
    long rssi = Fishino.RSSI();
    Serial << F("signal strength (RSSI):");
    Serial.print(rssi);
    Serial << F(" dBm\n");
    
}

int getLuce()
{
    int val = analogRead(fotoresistenza);
    return val;
}

int getTemp()
{
    uint8_t chk = DHT11.read(DHT11PIN);
    return DHT11.temperature;
}

int getUm()
{
    uint8_t chk = DHT11.read(DHT11PIN);
    return DHT11.humidity;
}




void timeReset()
{
    // lastReset();
    delay(1000);
    Reset();
}


void setup()
{
    
    Serial.begin(115200);
    
    Serial << F("Resetting Fishino...");
    while(!Fishino.reset())
    {
        Serial << ".";
        delay(500);
    }
    Serial << F("OK\r\n");
    
    Fishino.setPhyMode(PHY_MODE_11G);
    
    Serial.begin(115200);
    
    
    // initialize SPI
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    
    
    
    
    Serial.println("Sketch di test per sensore di umidita' e temperatura tipo DHT11");
    Fishino.setMode(STATION_MODE);
    
    Serial << F("Connecting to AP...");
    while(!Fishino.begin(MY_SSID, MY_PASS))
    {
        Serial << ".";
        delay(2000);
    }
    Serial << "OK\n";
    
    
#ifdef IPADDR
    Fishino.config(ip);
#else
    Fishino.staStartDHCP();
#endif
    
    // wait till connection is established
    Serial << F("Waiting for IP...");
    while(Fishino.status() != STATION_GOT_IP)
    {
        Serial << ".";
        delay(500);
    }
    Serial << "OK\n";
    
    printWifiStatus();
    
    
    // init leds I/O ports
    for(uint8_t iLed = 0; iLed < numLeds; iLed++)
    {
        uint8_t port = leds[iLed].port;
        pinMode(port, OUTPUT);
        digitalWrite(port, HIGH);
    }
    
    pinMode(fotoresistenza,INPUT); //Inizializzo sensore Luce-Fotocellula
    
    // start listening for clients
    server.begin();
    
    updateip();
    
    if (RTC.read(tm))
    {
        setTime(tm.Hour,tm.Minute,tm.Second,tm.Day,tm.Month,tm.Year); // set time to Saturday 8:29:00am Jan 1 2011
        // Serial.print("Ora settata");
    }
    
    
    
    Alarm.alarmRepeat(INIT_LIGHT_HOUR,INIT_LIGHT_MINUTE,INIT_LIGHT_SECOND, On);  // 8:30am every day
    Alarm.alarmRepeat(END_LIGHT_HOUR,END_LIGHT_MINUTE,END_LIGHT_SECOND, Off);  // 8:30am every day
    
    Alarm.alarmRepeat(15,00,00, timeReset);  // 8:30am every day
    Alarm.alarmRepeat(03,00,00, timeReset);  // 8:30am every day
    
    Alarm.timerRepeat(FREQ_VENT, Repeats);  //Ofni 5 minuti richiamo la funzione
    Alarm.timerRepeat(FREQ_DATA, httpRequest);  //Ofni 5 minuti richiamo la funzione
    Alarm.timerRepeat(FREQ_IP, updateip);  //Ofni 5 minuti richiamo la funzione
    // Alarm.timerRepeat(FREQ_RESET, Reset);  //Ofni 5 minuti richiamo la funzione
    
    lastReset();
    delay(500);
    
    setStatusLED();
    delay(500);
    startLED() ;
    
    //wdt_enable(WDTO_8S);  //abilito il watchdog
}




void On(){
    digitalWrite(3,LOW);
    Serial.println("Alarm: --------- LIGTHS ON--------");
    ventilation = 0;
    //  changeLEDSTATUS("LED1");
}

void Off(){
    digitalWrite(3,HIGH);
    Serial.println("Alarm: --------- LIGTHS OF--------");
    //  changeLEDSTATUS("LED1");
}


void Repeats(){
    if(ventilation==0){
        Serial.println("------ventilation on-----------");
        digitalWrite(5,LOW);
        ventilation=6;
        changeLEDSTATUS("LED2");
        //  Serial.println("Inviata  richiesta get");
    }
    else if(ventilation==6){
        Serial.println("------ventilation off-----------");
        digitalWrite(5,HIGH);
        ventilation--;
        changeLEDSTATUS("LED2");
    }else
    {
        ventilation--;
    }
}



void loop()
{
    
    // wait for a new client:
    
    delay(200);
    FishinoClient client1 = server.available();
    
    if (client1)
    {
        Serial.println("new client");
        // an http request ends with a blank line
        
        bool currentLineIsBlank = true;
        
        // flag for first line (the GET one) read
        bool gotGetLine = false;
        
        // buffer for GET line
        char buf[100];
        uint8_t iBuf = 0;
        
        while (client1.connected())
        {
            if (client1.available())
            {
                char c = client1.read();
                Serial.write(c);
                
                // if we're reading the first line, appena the char to it
                if(!gotGetLine && c != '\r' && c != '\n' && iBuf < 99)
                    buf[iBuf++] = c;
                
                // if we've got an end of line character, first line is terminated
                if((c == '\r' || c == '\n') && !gotGetLine)
                {
                    // mark first line as read
                    
                    buf[iBuf] = 0;
                    gotGetLine = true;
                    
                    // process the line to toggle the led
                    processLine(buf);
                    
                    Serial.print(ledName);
                    if(ledName[3]!='1')
                    {
                        Serial.print("STO QUI");
                        changeLEDSTATUS(ledName);  
                        
                    }
                    
                    
                    
                }
                
                // if you've gotten to the end of the line (received a newline
                // character) and the line is blank, the http request has ended,
                // so you can send a reply
                
                if (c == '\n' && currentLineIsBlank)
                {
                    // send a standard http response header
                    client1 << F("HTTP/1.1 200 OK\r\n");
                    client1 << F("Content-Type: text/html\r\n");
                    
                    // the connection will be closed after completion of the response
                    client1 << F("Connection: close\r\n\r\n");
                    client1 << F(
                                 "<!DOCTYPE HTML>\r\n"
                                 "<html>\r\n"
                                 "<head>\r\n"
                                 //         "<title>Fishino Web Button Demo</title>\r\n"
                                 "<script>  window.location.href = \"http://fishinoexample.altervista.org/Fishino/contatti.php/\" </script>"
                                 "</head>\r\n"
                                 "<body>\r\n"
                                 
                                 );
                    client1 <<
                    "</body>\r\n"
                    "</html>\r\n"
                    ;
                    break;
                }
                if (c == '\n')
                {
                    // you're starting a new line
                    currentLineIsBlank = true;
                }
                else if (c != '\r')
                {
                    // you've gotten a character on the current line
                    currentLineIsBlank = false;
                }
            }
        }
        // give the web browser time to receive the data
        // lascia tempo al browser per ricevere i dati
        delay(500);
        
        // close the connection:
        // chiudi la connessione
        client1.stop();
        Serial.println("client disonnected");
        
    }
    
    Alarm.delay(0);
    //wdt_reset();
}

