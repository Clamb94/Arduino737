#include <LedControl.h>
#include <EEPROM.h>
#include <Encoder.h>
#include <SPI.h>
#include <Ethernet.h>

/*
Arduino737 by Axel Reinemuth
____________________________________

Latest Version:
https://github.com/Clamb94/Arduino737

User manual:
http://arduino737.reinemuth.de/Help/Arduino737.html
(will be transfered to Github eventually)

Questions/Issues: 
https://github.com/Clamb94/Arduino737/issues

---------------------------------------------
Published under GPL-3.0 Licence
---------------------------------------------

----------------------------------------------------------------------------------
------------------------------C O D E-------------------------------------------
----------------------------------------------------------------------------------
*/

//Advanced Settings:

boolean manualMode = false; //Disables EEPROM read function (beta)

boolean enableEthernet = false;
boolean intPullup = true; //Uses internal pullup resistors
boolean quadEncoder = false;
boolean disableSerial = false;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; //Device mac adress
//IPAddress ip(192, 168, 178, 177); //IP Adress of the Arduino
//IPAddress server(192, 168, 178, 40); //IP Adress of Prosim737 Server
int TCPPort = 8091;

int analogInSmoothen = 1; //Increase number when analog input values are jumping up/down.
const int baud = 9600;  //BAUD RATE

int Pins[70] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int DevID = 10; //Device ID

//                      0          1          2      3          4        5          6            7        8        9        0.        1.        2.        3.        4.        5.        6.        7.        8.        9.       EMPTY       .         -
int SegmentCode[23] = {B00000011, B10011111, B00100101, B00001101, B10011001, B01001001, B01000001, B00011111, B00000001, B00001001, B00000010, B10011110, B00100100, B00001100, B10011000, B01001000, B01000000, B00011110, B00000000, B00001000, B11111111, B11111110, B11111101};

//////////////////////////////////////////////////////////////////////////////////
//DO NOT CHANGE ANYTHING BELOW THIS LINE! ----------------------------------------
//////////////////////////////////////////////////////////////////////////////////

int port;
long sts;
long stsOld;
int LED;
int TX;
int TXOld[70];
int x;
int y;
long prevMillis = 0; //Heartbeat
int TXval;
int TXvalOld[72];
int RXram[72];
int segmentArray[60][5]; //Store all 7-Segment Displays
int segmentCache[60];
int displayLength[60];
int showDot[60];
int chip[60];
int bright[60];
boolean leadingZero[60];


String toSend = "";

const int unused = 0;
const int button = 1;
const int encoder = 2;
const int output = 3;
const int outputrev = 4;
const int servo = 5;
const int analog = 6;
const int analogrev = 7;
const int Segment = 8;

char* modes[] = {"-", "Button", "Encoder", "Output", "Output rev", "Servo", "Analog in", "Analog in rev" , "7Segment"};
char* chipName[] = {"74HC595", "MAX72xx"};
char* maxArray[] = {"max1", "max2", "max3", "max4", "max5"};

long enc1;
long enc2;
long enc3;
long enc1old;
long enc2old;
long enc3old;

String part1; //Temporary string holders
String part2;

Encoder Enc1(2, 3);
Encoder Enc2(18, 19);
Encoder Enc3(20, 21);

LedControl lc1 = LedControl(EEPROM.read(0 * 10 + 132), EEPROM.read(0 * 10 + 131), EEPROM.read(0 * 10 + 130), 1);
LedControl lc2 = LedControl(EEPROM.read(1 * 10 + 132), EEPROM.read(1 * 10 + 131), EEPROM.read(1 * 10 + 130), 1);
LedControl lc3 = LedControl(EEPROM.read(2 * 10 + 132), EEPROM.read(2 * 10 + 131), EEPROM.read(2 * 10 + 130), 1);
LedControl lc4 = LedControl(EEPROM.read(3 * 10 + 132), EEPROM.read(3 * 10 + 131), EEPROM.read(3 * 10 + 130), 1);
LedControl lc5 = LedControl(EEPROM.read(4 * 10 + 132), EEPROM.read(4 * 10 + 131), EEPROM.read(4 * 10 + 130), 1);

LedControl lcs[] = {lc1, lc2, lc3, lc4, lc5};

EthernetClient client;

IPAddress ip(EEPROM.read(104), EEPROM.read(105), EEPROM.read(106), EEPROM.read(107)); //Arduino IP
IPAddress server(EEPROM.read(108), EEPROM.read(109), EEPROM.read(110), EEPROM.read(111)); //IP Adress of Prosim737 Server

String readString;

void setup() { //---------------------------------------------------------------------------------------------------------------------------------------------------------

  // SERIAL PORTS ----------------------------------------------------
  Serial.begin(baud);
  Serial.setTimeout(5);

  // READ EEPROM------------------------------------------------
  /* EEPROM usage
  0-76: Pin Modes
  100: Device ID
  101: use internal Pullup ( 0 / 1)
  102: 4x Encoder (Off: 0)
  103: Enable Ethernet
  104,105,106,107: Client IP Adress
  108,109,110,110: Server IP Adress
  111, 112: Empty
  113: Disable Serial (1: Disabled);
  114, 115: Baud rate (Unused)
  116,117,118: TCP Port

  130-134: LEDcontrol1
  140-144: LEDcontrol1
  150-154: LEDcontrol1

  200-250is: 7S: length
  300-360: 7S: Show Leading 0 for 7-Segement
  400-460: 7S:Show . at x
  500-560: Chip type: 0 = 74HC595; 1 = MAX
  600-660: Chip type: 0 = 74HC595; 1 = MAX

  */

  Serial.write("initializing...");
  if (manualMode == false) {
    DevID = EEPROM.read(100); //Read Device ID
    //DevID = 10;
    enableEthernet = EEPROM.read(103);
    intPullup = EEPROM.read(101);
    quadEncoder = EEPROM.read(102);
    disableSerial = EEPROM.read(113);

  }
  Serial.println();
  for (int i = 2; i <= 69; i++) {
    Pins[i - 2] = EEPROM.read(i);
    if (Pins[i - 2] > 8) {
      Pins[i - 2] = 0;
    }

    //Debug
    Serial.write("Pin:");
    Serial.print(i);
    Serial.write(" Mode:");
    Serial.println(modes[Pins[i - 2]]);
  }

  Serial.write("Device ID: ");
  Serial.println(DevID);
  Serial.write("Ethernet: ");
  Serial.println(enableEthernet);
  Serial.write("Use internal pullups: ");
  Serial.println(intPullup);
  Serial.write("4x encoder counting: ");
  Serial.println(quadEncoder);
  Serial.write("Disable Serial when using Ethernet: ");
  Serial.println(disableSerial);
  Serial.println();



  //INIT PINS ----------------------------------------------------
  for (int x = 0; x <= 67; x++) {
    if (Pins[x] == button) {  // Init buttons
      pinMode(x + 2, INPUT);
      if (intPullup == true) {
        digitalWrite(x + 2, HIGH);
      }
    }
    if (Pins[x] == output) { //Init Outputs
      pinMode(x + 2, OUTPUT);
      digitalWrite(x + 2, LOW);
    }
    if (Pins[x] == outputrev) { //Output rev
      pinMode(x + 2, OUTPUT);
      digitalWrite(x + 2, HIGH);
    }
  }

  int counterA = 0;
  int counterB = 0;
  //-----------------------------Read 7 Segment Displays
  for (int x = 2; x <= 69; x++) {
    if (Pins[x - 2] == Segment) {
      if (counterB == 0) {
        displayLength[x] = EEPROM.read(200 + x);
        leadingZero[x] = EEPROM.read(300 + x);
        showDot[x] = EEPROM.read(400 + x);
        chip[x] = EEPROM.read(500 + x);
        bright[x] = EEPROM.read(600 + x);
      }

      segmentArray[counterA][counterB] = x;
      counterB = counterB + 1;
      if (counterB >= 3) {
        counterA = counterA + 1;
        counterB = 0;
      }
    }
  }


  //Analyse 7-Segment
  Serial.println();
  Serial.println("Valid 7-Segment displays: ");

  for (int i = 0; i <= 50; i++) {
    if (segmentArray[i][2] != NULL) {
      Serial.print(i);
      Serial.write("Latch Pin:");
      Serial.print(segmentArray[i][0]);
      Serial.write(" | Clock Pin:");
      Serial.print(segmentArray[i][1]);
      Serial.write(" | Data Pin:");
      Serial.print(segmentArray[i][2]);
      Serial.write(" | Length:");
      Serial.print(displayLength[segmentArray[i][0]]);
      Serial.write(" | Leading Zero:");
      Serial.print(leadingZero[segmentArray[i][0]]);
      Serial.write(" | Show dot at:");
      Serial.print(showDot[segmentArray[i][0]]);
      Serial.write(" | Chip:");
      Serial.print(chipName[chip[segmentArray[i][0]]]);
      if (chip[segmentArray[i][0]] != 0){
      Serial.write(" | Brightness:");
      Serial.println(bright[segmentArray[i][0]]);}
      else { Serial.println(); }

      if (chip[segmentArray[i][0]] == 0) { //47xx chip setup
        pinMode(segmentArray[i][0], OUTPUT);
        pinMode(segmentArray[i][1], OUTPUT);
        pinMode(segmentArray[i][2], OUTPUT);

        digitalWrite(segmentArray[i][0], LOW);

        for (int k = 0; k <= displayLength[segmentArray[i][0]]; k++) {
          shiftOut(segmentArray[i][2], segmentArray[i][1], LSBFIRST, SegmentCode[k]); //Clear all on startup
        }
        digitalWrite(segmentArray[i][0], HIGH);
      }

      else if (chip[segmentArray[i][0]] == 1) { //MAX Chip setup
        if(EEPROM.read(i * 10 + 130) != segmentArray[i][0]) { EEPROM.write(i * 10 + 130, segmentArray[i][0]); }
        if(EEPROM.read(i * 10 + 131) != segmentArray[i][1]) { EEPROM.write(i * 10 + 131, segmentArray[i][1]); }
        if(EEPROM.read(i * 10 + 132) != segmentArray[i][2]) { EEPROM.write(i * 10 + 132, segmentArray[i][2]); }

        lcs[i].shutdown(0, false);
        lcs[i].setIntensity(0, bright[segmentArray[i][0]]);
        lcs[i].clearDisplay(0);

        for (int k = 1; k <= displayLength[segmentArray[i][0]]; k++) {
          lcs[i].setDigit(0, k - 1, (byte)k, false);
          //lc.setChar(0, k, '-', false);
        }
      }
    }
  }
  Serial.println();

  //Enable Heartbeat pin
  if (Pins[13 - 2] == 0) {
    pinMode(13, OUTPUT);
  }


  // ETHERNET------------------------------------------------
  if (enableEthernet == true) {
    Serial.write("Ethernet Enabled! Arduino IP:");
    Serial.println(ip);

    Ethernet.begin(mac, ip);
    delay(1000);
    Serial.write("connecting to: ");
    Serial.print(server);
    Serial.write(":");
    Serial.println(TCPPort);
    Serial.println();

    if (client.connect(server, TCPPort)) {
      Serial.println("connected!");
    }
    else {
      // if you didn't get a connection to the server:
      Serial.println("connection failed");
    }
  }

}
void loop() { //---------------------------------------------------------------------------------------------------------------------------------------------------------

  char c[20] = "";
  String str1;
  String str2;

  //Heartbeat
  if (Pins[13 - 2] == 0) {
    unsigned long currentMillis = millis();

    if (currentMillis - prevMillis > 700) {
      analogWrite(13, 200);
    }
    if (currentMillis - prevMillis > 800) {
      digitalWrite(13, LOW);
    }
    if (currentMillis - prevMillis > 1000) {
      analogWrite(13, 200);
    }
    if (currentMillis - prevMillis > 1100) {
      digitalWrite(13, LOW);
      prevMillis = currentMillis;
    }
  }
  //Read TCP------------------------

  if (enableEthernet == true) {
    if (client.available()) {

      c[0] = (char)0;

      int ca = client.available();
      if (ca > 11) {
        ca = 11;
      }

      client.readBytes(c, ca);

      while (client.available()) {
        client.read();
      }

      String str(c);

      str1 = str.substring(0, 4);
      str2 = str.substring(5, 8);

      port = str1.toInt();
      sts = str2.toInt();

      Serial.println(port);
      Serial.println(sts);

      processOutputs();
    }
  }
  //Read Serial---------------------------------------------------------------------

  if (Serial.available()) {
    port = Serial.parseInt();
    if (port == 999) {
      load(); //Load setup
    } else {
      sts = Serial.parseInt();
      processOutputs();
    }
  }
  //Read Digital pins and Send---------------------------------------------------------------------

  for (int x = 2; x <= 71; x++) {
    TXval = 1;
    if ((digitalRead(x) == HIGH) && (Pins[x - 2] == button)) {  //READ BUTTONS
      TX = DevID * 1000;
      TX = TX + (x * 10);
      TXval = 1;

    } else if ((digitalRead(x) == LOW) && (Pins[x - 2] == button)) {
      TX = DevID * 1000;
      TX = TX + (x * 10) + 1;
      TXval = 1;

    } else if (Pins[x - 2] == analog) {     //READ ANALOG
      TX = DevID * 100;
      TX = TX + x;
      TXval = analogRead(x - 54);
      TXval = TXval / 4;

    } else if (Pins[x - 2] == analogrev) {     //READ ANALOGreversed
      TX = DevID * 100;
      TX = TX + x;
      TXval = analogRead(x - 54);
      TXval = (TXval / 4) - 255;
      TXval = abs(TXval);

    } else {
      TX = 0;
      TXval = 1;
    }

    if (TXval - TXvalOld[x - 2] <= analogInSmoothen && TXvalOld[x - 2] - TXval <= analogInSmoothen ) {
      TXval = TXvalOld[x - 2]; //smoothen analog signal
    }

    if ((TX != TXOld[x - 2]) || (TXval != TXvalOld[x - 2])) {

      TXOld[x - 2] = TX;
      TXvalOld[x - 2] = TXval;

      toSend = "";     //Create String to be sended
      part1 = String(TX);
      part2 = String(TXval);
      toSend = part1 + "=" + part2 + ":";

      sendData();

      if ((Pins[x - 2] == button) && (digitalRead(x) == HIGH)) {
        reloadButtons();
      }
    }
  }
  //Encoders--------------------------------------------------
  int encStep = 1;
  if (quadEncoder == true) {
    encStep = 1;
  } else {
    encStep = 4;
  }

  for (int x = 2; x <= 71; x++) {
    if (Pins[x - 2] == 2) {
      enc1 = Enc1.read();
      if (abs(enc1) >=  encStep) {
        while (enc1 <= -encStep) {
          toSend = "1002=-1:";
          sendData();
          enc1 = enc1 + encStep;
        }
        while (enc1 >= encStep) {
          toSend = "1002=1:";
          sendData();
          enc1 = enc1 - encStep;
        }
        Enc1.write(0);
      }
    }

    if (Pins[x - 2] == 2) {
      enc2 = Enc2.read();
      if (abs(enc2) >=  encStep) {
        while (enc2 <= -encStep) {
          toSend = "1018=-1:";
          sendData();
          enc2 = enc2 + encStep;
        }
        while (enc2 >= encStep) {
          toSend = "1018=1:";
          sendData();
          enc2 = enc2 - encStep;
        }
        Enc2.write(0);
      }
    }

    if (Pins[x - 2] == 2) {
      enc3 = Enc3.read();
      if (abs(enc3) >=  encStep) {
        while (enc3 <= -encStep) {
          toSend = "1020=-1:";
          sendData();
          enc3 = enc3 + encStep;
        }
        while (enc3 >= encStep) {
          toSend = "1020=1:";
          sendData();
          enc3 = enc3 - encStep;
        }
        Enc3.write(0);
      }
    }
  }
}

void load() { //-----------------------------------------Arduino737 Communication

  int ID = 0;
  char serialData[1];
  int modeTemp1;
  int modeTemp2;
  int modeTemp3;

  ID = Serial.parseInt();
  EEPROM.write(100, ID);
  Serial.println();
  for (int i = 2; i <= 100; i++) {

    int Pin = Serial.parseInt();
    int mode = Serial.parseInt();

    if (Pin == 101) {
      EEPROM.write(101, mode);
    }
    else if (Pin == 102) {
      EEPROM.write(102, mode);
    }
    else if (Pin == 103) {
      EEPROM.write(103, mode);
    }
    else if (Pin == 113) {
      EEPROM.write(113, mode);
    }
    else if (Pin == 104) {
      EEPROM.write(104, mode);
      mode = Serial.parseInt();
      EEPROM.write(105, mode);
      mode = Serial.parseInt();
      EEPROM.write(106, mode);
      mode = Serial.parseInt();
      EEPROM.write(107, mode);
    }
    else if (Pin == 108) {
      EEPROM.write(108, mode);
      mode = Serial.parseInt();
      EEPROM.write(109, mode);
      mode = Serial.parseInt();
      EEPROM.write(110, mode);
      mode = Serial.parseInt();
      EEPROM.write(111, mode);
    } else if (Pin <= 299 && Pin >= 200) {
      EEPROM.write(Pin, mode); //Length of 7S
      mode = Serial.parseInt();
      EEPROM.write(Pin + 100, mode); //show 0
      mode = Serial.parseInt();
      EEPROM.write(Pin + 200, mode); //show Dot At
      mode = Serial.parseInt();
      EEPROM.write(Pin + 300, mode); //Chip type
      mode = Serial.parseInt();
      EEPROM.write(Pin + 400, mode); //brightness

    } else {
      RXram[Pin] = mode;
    }

  }
  for (int i = 2; i <= 69; i++) {
    EEPROM.write(i, RXram[i]);

    Serial.print(i) ;
    Serial.write("=")  ;
    Serial.println(RXram[i]);

  }
}
void reloadButtons() {

  for (int x = 2; x <= 71; x++) {
    if ((digitalRead(x) == HIGH) && (Pins[x - 2] == button)) {  //READ BUTTONS
      TX = DevID * 1000;
      TX = TX + (x * 10);

      part1 = String(TX);
      toSend = part1 + ":";
      sendData();
    }
  }
}

void sendData() {
  if ((enableEthernet == true) && (disableSerial == true)) {

  } else {
    Serial.print(toSend);
  }

  if ((enableEthernet == true) && (client.connected())) {
    client.println(toSend);
  }
}

void processOutputs() {
  if (port == 0) {
    sts = stsOld;
  } else if ((port != 0) && (Pins[port - (DevID * 100) - 2] == output)) //Outputs
  {
    stsOld = sts;
    LED = port - (DevID * 100);

    if (sts == 1) {
      digitalWrite(LED, HIGH);
    }
    if (sts == 0) {
      digitalWrite(LED, LOW);
    }

  } else if ((port != 0) && (Pins[port - (DevID * 100) - 2] == outputrev)) //Outputs rev
  {
    stsOld = sts;
    LED = port - (DevID * 100);

    if (sts == 1) {
      digitalWrite(LED, LOW);
    }
    if (sts == 0) {
      digitalWrite(LED, HIGH);
    }
  } else if ((port != 0) && (Pins[port - (DevID * 100) - 2] == Segment)) //7-Segments
  {
    int Pin = port - (DevID * 100);
    boolean zeroTemp = leadingZero[Pin];
    int counter = 0;
    boolean negative = false;
    boolean off = false;
    String str;
    int segNo;

    segmentCache[0] = 0;
    segmentCache[1] = 0;
    segmentCache[2] = 0;
    segmentCache[3] = 0;
    segmentCache[4] = 0;
    segmentCache[5] = 0;
    segmentCache[6] = 0;
    segmentCache[7] = 0;



    if (sts == -1) {
      off = true;
     // Serial.print("Off");
    }
    if (sts < 0) {
      negative = true;
      sts = abs(sts);
    }

    str = String(sts, DEC);
    int strLength = str.length();
    Serial.println(str);

    for ( int z = 1; z <= strLength; z++) {
      String sString;
      int temp5;
      sString = str.substring(strLength - z, strLength - z + 1);
      segmentCache[z] = sString.toInt();
      //Serial.print(segmentCache[z]);
    }

/*
    for (int i = 1; i <= 8; i++) {
      if (segmentCache[i] == 0 &&  zeroTemp == false) {
        segmentCache[i] = 20;
      } else {
        zeroTemp = true;
      }
    }
    */
    
    for (int i = 0; i <= 50; i++) {
      if (segmentArray[i][0] == Pin) {
        segNo = i;
        break;
      }
    }


    if (chip[segmentArray[segNo][0]] == 0) {  //-------------------------------47xxx CHIP

      if (showDot[Pin] != 0) {
        segmentCache[showDot[Pin]] = segmentCache[showDot[Pin]] + 10;
      }

      digitalWrite(segmentArray[segNo][0], LOW);

      for (int i = 1; i <= displayLength[Pin]; i++) {
        if (off == true) {
          segmentCache[i] = 20;
        }
        shiftOut(segmentArray[segNo][2], segmentArray[segNo][1], LSBFIRST, SegmentCode[segmentCache[i]]);
        
      }
      
      digitalWrite(segmentArray[segNo][0], HIGH);

    } else if (chip[segmentArray[segNo][0]] == 1) { //-------------------------------MAXxxx CHIP

      for ( int i = 1; i <= displayLength[Pin]; i++) {
        lcs[segNo].setDigit(0, i - 1, (byte)segmentCache[i], false);
      }

      if (showDot[Pin] != 0) {
        lcs[segNo].setDigit(0, showDot[Pin] - 1, segmentCache[showDot[Pin]] , true);
        
      }
      if (negative == true) {
        lcs[segNo].setChar(0, displayLength[Pin] - 1 , '-' , false);
      }
      if (off == true) {
        lcs[segNo].clearDisplay(0);
      }
    }
  }
}


