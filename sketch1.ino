/* Web client programm for Arduino UNO to switch a "Junghanns.net" ip controled Power Distributor (verteilersteckdose)
 * written by : Su Gao
 * Last Edite : 2016-7-13
 */

#include <EEPROM.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <SPI.h> // SPI.h & SD.h are not include for Ethernet Shield as default.
#include <SD.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0A, 0x00, 0xBA }; // Mac address on this shield.
IPAddress server(192, 168, 2, 112); // Target ip address
IPAddress ip(192, 168, 2, 5); // Own device's ip address
EthernetClient client;

// Try to build a small webserver interface in order to let user easieer to set parameters.
// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer webServer(80);

//------------------------------------------------------------------------------
// constants
//------------------------------------------------------------------------------
#define DELAY   (5)
#define BAUD    (115200)
#define MAX_BUF (64)

//------------------------------------------------------------------------------
// global variables
//------------------------------------------------------------------------------
char buffer[MAX_BUF];
int sofar;
unsigned long t = 11, r = 22, p = 33, q = 44;
int valAdr[4] = {100, 110, 120, 130};
//------------------------------------------------------------------------------
//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to address + 3.
void EEPROMWritelong(int address, unsigned long value) {
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

//This function will return a 4 byte (32bit) long from the eeprom
//at the specified address to address + 3.
unsigned long EEPROMReadlong(long address) {
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void processCommand() {
  if (!strncmp(buffer, "help", 4)) {
    Serial.println(F(
                     "commands:\nls; List the status of all switch.\nsX=1; Turn on switch 'X'.\nsX=0;Turn off switch 'X'.\nconnect; Try connect to the switcher.\ndisconnect; Try disconnect to the switcher."));
  } else if (!strncmp(buffer, "s", 1)) {
    Serial.println(F("got port setting."));
    char *ptr = buffer;
    ptr = strchr(ptr, 's'); int portNum = atoi(ptr + 1);
    ptr = strchr(ptr, '='); int portStat = atoi(ptr + 1);
    String qry = "GET /SWOV.CGI?s";
    qry += portNum;
    qry += '=';
    qry += portStat;
    Serial.println(qry);
    client.println(qry);
  } else if (!strncmp(buffer, "connect", 7)) {
    Serial.println(F("Try to connect to server."));
    if (client.connect(server, 80)) {
      Serial.println(F("connected"));
    } else {
      Serial.println(F("connection failed"));
    }
  } else if (!strncmp(buffer, "disconnect", 10)) {
    Serial.println(F("Try to disconnect to server."));
    client.stop();
    if (!client.connected()) {
      Serial.println(F("disconnected."));
      //      client.stop();
    } else {
      Serial.println(F("Server is still connected."));
    }
  }
}

void listenForEthernetClients() {
  // listen for incoming clients
  EthernetClient client = webServer.available();
  if (client) {
    Serial.println(F("Got a client"));

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    boolean currentLineIsGet = true;

    int tCount = 0;
    char tBuf[256];
    char *pch;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply

        if (currentLineIsGet && tCount < 63) {
          tBuf[tCount] = c;
          tCount++;
          tBuf[tCount] = 0;
        }

        if (c == '\n' && currentLineIsBlank) {

          // send a standard http response
          while (client.available()) client.read();
          Serial.println(tBuf);
          pch = strtok(tBuf, "?");

          while (pch != NULL)
          {
            if (strncmp(pch, "t=", 2) == 0)
            {
              t = atol(pch + 2);
              EEPROMWritelong(valAdr[0], t);
              Serial.print("t=");
              Serial.println(t, DEC);
            }

            if (strncmp(pch, "r=", 2) == 0)
            {
              r = atol(pch + 2);
              EEPROMWritelong(valAdr[1], r);
              Serial.print("r=");
              Serial.println(r, DEC);
            }

            if (strncmp(pch, "p=", 2) == 0)
            {
              p = atol(pch + 2);
              EEPROMWritelong(valAdr[2], p);
              Serial.print("p=");
              Serial.println(p, DEC);
            }

            if (strncmp(pch, "q=", 2) == 0)
            {
              q = atol(pch + 2);
              EEPROMWritelong(valAdr[3], q);
              Serial.print("q=");
              Serial.println(q, DEC);
            }


            pch = strtok(NULL, "& ");
          }
          Serial.println(F("Sending response"));

          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          // print the current readings, in HTML format:
          client.print(F("<!DOCTYPE html>"));
          client.print(F("<style type='text/css'>BODY { margin: 9px 9px; background: #F0F0F0; font: 16px Courier;text-align: center;}</style>"));
          client.print(F("<html><body>"));
          client.println(F("<H1><span style='color:#AA3939'>(Awesome)</span> Power Distributor for Jo</H1>"));
          client.print(F("<form method=GET>"));
          client.print(F("T: <input type=text name=t value="));
          client.print(t);
          client.print(F("><br>"));
          client.println(F("R: <input type=text name=r value="));
          client.print(r);
          client.print(F("><br>"));
          client.println(F("P: <input type=text name=p value="));
          client.print(p);
          client.print(F("><br>"));
          client.println(F("Q: <input type=text name=q value="));
          client.print(q);
          client.print(F("><br>"));
          client.println(F("<input type=submit value='Set Value'></form>"));
          client.println(F("</body></html>"));
          client.print(F("<p>Click the 'Set Value' button and the form-data will be sent to a page on the server called 'demo_form.asp'.</p>"));
          break;
        } else if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          currentLineIsGet = false;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);       // give the web browser time to receive the data
    client.stop();  // close the connection:
  }
}


void setup() {
  Serial.begin(BAUD);
  Serial.println(F("Network Steckerverteiler is running."));
  Serial.println(F("send 'help;' to get command list"));

  Ethernet.begin(mac, ip);
  webServer.begin();

  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.println(F("connecting..."));

  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.println(F("connected"));
    //    Make a HTTP request:
    // client.println("GET /SWOV.CGI?s5=0");
    // client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println(F("connection failed"));
    Serial.println(F("Retry connect to Web Server of Power Distributor"));
    delay(500);
    while (!client.connect(server, 80)) {
      Serial.println(F("Retry connect to Web Server of Power Distributor"));
      delay(250);
    }
    Serial.println(F("connected"));
  }
  // Load the saved value of each value.
  t = EEPROMReadlong(valAdr[0]);
  r = EEPROMReadlong(valAdr[1]);
  p = EEPROMReadlong(valAdr[2]);
  q = EEPROMReadlong(valAdr[3]);

}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  while (Serial.available() > 0) {
    buffer[sofar++] = Serial.read();
    if (buffer[sofar - 1] == ';') break; // in case there are multiple instructions
  }

  // if we hit a semi-colon, assume end of instruction.
  if (sofar > 0 && buffer[sofar - 1] == ';') { // what if message fails/garbled?
    buffer[sofar] = 0;
    Serial.println(buffer);   // echo confirmation

    processCommand();         // do something with the command
    sofar = 0;                // reset the buffer
    Serial.println("Done.");  // echo completion
  }

  listenForEthernetClients(); // listen for incoming Ethernet connections:
}
