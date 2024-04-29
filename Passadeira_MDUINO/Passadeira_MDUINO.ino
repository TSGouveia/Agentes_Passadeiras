#include <Ethernet.h>

////////////////////////////////////////////
#define ATUADOR_1 Q1_7  //Conv Esquerda
#define ATUADOR_2 Q1_6  //Conv direita
#define ATUADOR_3 Q1_5  //punch cima
#define ATUADOR_4 Q1_4  //punch baixo

#define SENSOR_1 I0_4  //Conv Entrada
#define SENSOR_2 I0_3  //Conv Punch
#define SENSOR_3 I0_2  //Punch UP
#define SENSOR_4 I0_1  //Punch Down

#define MACHINEDELAY 1000

#define SSID "Kit FMS"
#define PASSWORD "demo01FMS"

#define SKILLEXIT "exit"
#define SKILLENTRY "entry"
#define SKILLPUNCH "punch"

#define METHODEXIT "start_exit"
#define METHODENTRY "start_entry"
#define METHODSTOP "stop_conveyor"
////////////////////////////////////////////

byte mac[] = { 0x00, 0xAD, 0xCE, 0xEB, 0xFE, 0xED };
IPAddress ip(192, 168, 2, 20);

EthernetServer server(8888);

int skillRequest = 0;

// portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

//////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Setup and Loop ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);  //initialize serial communication at a 9600 baud rate
  Serial.println("Comecar Setup\n");
  startPorts();
  Serial.println("Portos Setup\n");
  //setupFreeRTOS();
  setupEthernet();
  Serial.println("Setup Acabado\n");
}

void loop() {

  handleRequests();
  delay(1);
}

//////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// Setup /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void startPorts() {
  ///////////////////////////////////////////////
  //Inicializar os portos
  //Input
  pinMode(ATUADOR_1, OUTPUT);
  pinMode(ATUADOR_2, OUTPUT);
  pinMode(ATUADOR_3, OUTPUT);
  pinMode(ATUADOR_4, OUTPUT);

  //Output
  pinMode(SENSOR_1, INPUT);
  pinMode(SENSOR_2, INPUT);
  pinMode(SENSOR_3, INPUT);
  pinMode(SENSOR_4, INPUT);
}

void setupFreeRTOS() {
  Serial.println("");
}

void setupEthernet() {

  ///////////////////////////////////////////////
  // start the Ethernet
  Ethernet.begin(mac, ip);

  //////////////////////////////////////////////
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1);  // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  Serial.println("Starting Server");

  server.begin();
  Serial.print("Server at ");
  Serial.println(Ethernet.localIP());
}

//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// Sensor and Atuador /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
int sensor(int portIndex) {
  switch (portIndex) {
    case 1:
      return digitalRead(SENSOR_1);
      break;
    case 2:
      return digitalRead(SENSOR_2);
      break;
    case 3:
      return digitalRead(SENSOR_3);
      break;
    case 4:
      return digitalRead(SENSOR_4);
      break;
  }
}

void atuador(int portIndex, int high) {
  switch (portIndex) {
    case 1:
      digitalWrite(ATUADOR_1, high);
      break;
    case 2:
      digitalWrite(ATUADOR_2, high);
      break;
    case 3:
      digitalWrite(ATUADOR_3, high);
      break;
    case 4:
      digitalWrite(ATUADOR_4, high);
      break;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// HTTP Requests /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
String handlePostSkill(String message) {

  if (message == SKILLEXIT) {
    gotoConveyor(1);
    return "Skill " + message + " finished";
  } else if (message == SKILLENTRY) {
    gotoConveyor(0);
    return "Skill " + message + " finished";
  } else if (message == SKILLPUNCH) {
    skillPunch();
    return "Skill " + message + " finished";
  }
  {
    return "Skill " + message + " not found";
  }
}

String handlePostMethod(String message) {
  if (message == METHODEXIT) {
    moveConveyorFront();
    return "Method " + message + " finished";
  } else if (message == METHODENTRY) {
    moveConveyorBack();
    while (getConveyorPos() != 1) {
      continue;
    }
    stopConveyor();
    return "Method " + message + " finished";
  } else if (message == METHODSTOP) {
    stopConveyor();
    return "Method " + message + " finished";
  }
  {
    return "Method " + message + " not found";
  }
  /*
  if (message == SKILL3) {
    gotoPunch(1);  
    return "Skill " + message + " started";
  } else if (message == SKILL4) {
    gotoPunch(0);
    return "Skill " + message + " started";
  } else if (message == SKILL5) {
    skillPunch();
    return "Skill " + message + " started";
  } else {
    return "Skill " + message + " not found";
  }
  */
}

String handleNotFound() {
  String message = "Erro ao executar skill\n\n";
  message += "\nEsperado:\nURI: /transporte/\nMetodo: POST\nArgumentos: 1\n skill: C1SL1\n";
  /*message += "\nVerifica se o pedido Ã© um POST";
  message += "\nVerifica se o pedido tem o seguinte formato:\n";
  message += "192.168.2.69:80/transporte/?skill=C1SL1\n";
  message += "\nSkills disponiveis:\n";
  message += " /transporte/\n  ";
  message += SKILL1;
  message += "\n  ";
  message += SKILL2;
  message += "\n  ";

  message += "/estacao/\n  ";
  message += SKILL3;
  message += "\n  ";
  message += SKILL4;
  message += "\n  ";
  message += SKILL5;
  message += "\n  ";
*/
  return message;
}

//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// Receber Posts //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void handleRequests() {
  String readString;
  String response;
  String skill;
  Serial.println("A Receber\n");
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        Serial.println("A Ler\n");
        char c = client.read();
        readString += c;
        Serial.println(readString);
        //if HTTP request has ended
        if (c == '\n') {

          if (readString.indexOf("/skills") > 0) {
            if (readString.indexOf("?skill=") > 0) {
              int pos1 = readString.indexOf("=");
              int pos2 = readString.indexOf("HTTP");
              skill = readString.substring(pos1 + 1, pos2 - 1);

              response = handlePostSkill(skill);
            }
          } else if (readString.indexOf("/methods") > 0) {
            Serial.println("Entrei Estacao");
            if (readString.indexOf("?skill=") > 0) {
              Serial.println("Entrei Skill\n");
              int pos1 = readString.indexOf("=");
              int pos2 = readString.indexOf("HTTP");
              skill = readString.substring(pos1 + 1, pos2 - 1);

              Serial.println(skill);

              response = handlePostMethod(skill);
            }
          } else {
            response = handleNotFound();
          }

          Serial.println(response);
          client.println("HTTP/1.1 200 OK\nContent-Type: text/html\nConnection: close\nRefresh: 5\n\n" + response);
          client.stop();

          Serial.println("Boom");
          //clearing string for next read
          readString = "";
        }
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Conveyor 1 //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void stopConveyor() {

  atuador(1, 0);
  atuador(2, 0);
}

void moveConveyorFront() {

  atuador(1, 0);
  atuador(2, 1);
}

void moveConveyorBack() {

  atuador(2, 0);
  atuador(1, 1);
}

int getConveyorPos() {

  int I1 = sensor(1);
  int I2 = sensor(2);

  if (I1 == 0)
    return 1;
  else if (I2 == 0)
    return 0;
  else return -1;  //if none are high level it's not on either ends
}

void gotoConveyor(int pos) {
  if (pos == 1) {
    moveConveyorFront();

    while (getConveyorPos() != 1) {
      continue;
    }
  } else {
    moveConveyorBack();

    while (getConveyorPos() != 0) {
      continue;
    }
  }

  stopConveyor();  //when pos reached stop
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////// Punch ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void stopPunch() {

  atuador(3, 0);
  atuador(4, 0);
}

void movePunchUp() {

  atuador(4, 0);
  atuador(3, 1);
}

void movePunchDown() {

  atuador(3, 0);
  atuador(4, 1);
}

int getPunchPos() {

  int I1 = sensor(3);
  int I2 = sensor(4);

  if (I2 == 1)
    return 0;  //if high level it's retracted
  else if (I1 == 1)
    return 1;      //if high level it's extended
  else return -1;  //if none are high level it's not on either ends
}

void gotoPunch(int pos) {
  if (pos == 1) {
    movePunchUp();

    while (getPunchPos() != 1) {
      continue;
    }
  } else {
    movePunchDown();

    while (getPunchPos() != 0) {
      continue;
    }
  }

  stopPunch();
}

void skillPunch() {
  gotoPunch(0);
  delay(MACHINEDELAY);
  gotoPunch(1);
}