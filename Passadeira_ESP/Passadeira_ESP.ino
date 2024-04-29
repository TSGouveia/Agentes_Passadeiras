#include <WiFi.h>
#include <WebServer.h>

////////////////////////////////////////////
#define ATUADOR_1 23  //Conv Esquerda
#define ATUADOR_2 22  //Conv direita
#define ATUADOR_3 21  //punch cima
#define ATUADOR_4 19  //punch baixo
#define ATUADOR_5 18
#define ATUADOR_6 5
#define ATUADOR_7 17

#define SENSOR_1 35  //Conv Entrada
#define SENSOR_2 25  //Conv Punch
#define SENSOR_3 26  //Punch UP
#define SENSOR_4 27  //Punch Down
#define SENSOR_5 2
#define SENSOR_6 4
#define SENSOR_7 16

#define MACHINEDELAY 1000

#define SSID "Kit FMS"
#define PASSWORD "demo01FMS"

#define SKILLENTRY "entry"
#define SKILLEXIT "exit"
#define SKILLPUNCH "punch"

#define MODULESTARTENTRY "start_entry"
#define MODULESTARTEXIT "start_exit"
#define MODULESTOPCONVEYOR "stop_conveyor"
////////////////////////////////////////////

IPAddress local_IP(192, 168, 2, 21);  // Set your Static IP address
IPAddress gateway(192, 168, 1, 1);    // Set your Gateway IP address
IPAddress subnet(255, 255, 0, 0);

WebServer server(80);

int skillRequest = 0;

portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

//////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Setup and Loop ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);  //initialize serial communication at a 9600 baud rate
  startPorts();
  setupFreeRTOS();
  setupWiFi();
}
void loop() {
  server.handleClient();
  delay(1);
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
    case 5:
      return digitalRead(SENSOR_5);
      break;
    case 6:
      return digitalRead(SENSOR_6);
      break;
    case 7:
      return digitalRead(SENSOR_7);
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
    case 5:
      digitalWrite(ATUADOR_5, high);
      break;
    case 6:
      digitalWrite(ATUADOR_6, high);
      break;
    case 7:
      digitalWrite(ATUADOR_7, high);
      break;
  }
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
  pinMode(ATUADOR_5, OUTPUT);
  pinMode(ATUADOR_6, OUTPUT);
  pinMode(ATUADOR_7, OUTPUT);

  //Output
  pinMode(SENSOR_1, INPUT);
  pinMode(SENSOR_2, INPUT);
  pinMode(SENSOR_3, INPUT);
  pinMode(SENSOR_4, INPUT);
  pinMode(SENSOR_5, INPUT);
  pinMode(SENSOR_6, INPUT);
  pinMode(SENSOR_7, INPUT);
}

void setupFreeRTOS() {
  Serial.println("");
}

void setupWiFi() {
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Erro a configurar IP estático");
  }

  //WiFi.setSleep(WIFI_PS_NONE);

  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/skills/", HTTP_POST, handlePostSkills);
  server.on("/methods/", HTTP_POST, handlePostMethods);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

//////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// HTTP Requests /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void handlePostSkills() {
  Serial.println("Recebi skill");
  String message = "";

  if (server.hasArg("skill")) {
    message = server.arg("skill");
  }

  if (message == SKILLENTRY) {
    gotoConveyor(0);
    server.send(200, "text/plain", "Skill " + message + " finished");
  } else if (message == SKILLEXIT) {
    gotoConveyor(1);
    server.send(200, "text/plain", "Skill " + message + " finished");
  } else if (message == SKILLPUNCH) {
    skillPunch();
    server.send(200, "text/plain", "Skill " + message + " finished");
  } else {
    server.send(200, "text/plain", "Skill " + message + " not found");
  }
}

void handlePostMethods() {
  Serial.println("Recebi metodo");
  String message = "";

  if (server.hasArg("skill")) {
    message = server.arg("skill");
  }

  if (message == MODULESTARTENTRY) {
    moveConveyorBack();
    while (getConveyorPos() != 1) {
      continue;
    }
    stopConveyor();
    server.send(200, "text/plain", "Method " + message + " started");
  } else if (message == MODULESTARTEXIT) {
    moveConveyorFront();
    server.send(200, "text/plain", "Method " + message + " started");
  } else if (message == MODULESTOPCONVEYOR) {
    stopConveyor();
    server.send(200, "text/plain", "Method " + message + " started");
  } else {
    server.send(200, "text/plain", "Method " + message + " not found");
  }
  
}

void handleNotFound() {
  Serial.println("Recebi erro");
  String message = "Erro ao executar skill\n\n";
  message += "Obtido:\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMetodo: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArgumentos: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
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
