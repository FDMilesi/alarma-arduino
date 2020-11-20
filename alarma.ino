#include <Keypad.h>

//Alarma
//CONSTANTES
const int ALARM_DELAY = 10000; //10 segundos
const int ACTIVATE_DELAY = 10000; //10 segundos
const int SECRET_SIZE = 4;
const char SECRET[SECRET_SIZE] = {'1', '4', '7', '8'};
const char RESET_SECRET_CHAR = '*';
const int PIN_SIRENA = 12;
const int PIN_SPEAKER = 10;
const int PIN_LED = 11;
const int PIN_SENSOR_ENTRADA = A1;
const int PIN_SENSOR_LAVADERO = A2;
//ESTADOS
const int ACTIVA_OK = 0;
const int ACTIVA_SONANDO = 1;
const int INACTIVA = 2;
const int NOTIFICANDO = 3;
const int POR_SONAR = 8;
const int POR_ACTIVARSE = 9;
//TIPOS NOTIFICACION
const int BEEPS_3 = 4;
const int LEDS_3 = 5;
const int BEEPS_2 = 6;
const int BEEPS_1 = 7;
//VARIABLES
char keys_pressed[SECRET_SIZE] = {'-', '-', '-', '-'};
int count_key = 0;
int estado;
boolean codigo_correcto = false;
int tipoNotificacion;
int estadoAnterior;
int countNotificacion;
boolean notificado;
unsigned long previousMillis = 0;
//End alarma

//Keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad. 2 va al medio
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad. 9 va al medio
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
//End keypad

void setup() {
  Serial.begin(9600);
  pinMode(PIN_SIRENA, OUTPUT);
  pinMode(PIN_SPEAKER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SENSOR_ENTRADA, INPUT_PULLUP);
  pinMode(PIN_SENSOR_LAVADERO, INPUT_PULLUP);
  estado = INACTIVA;
  countNotificacion = 0;
  notificado = false;
}

void loop() {
  //Chequeo sensores
  boolean sensores_ok = false;
  
  //Chequeo codigo
  char key = keypad.getKey();
  if (key != NO_KEY){
    Serial.print("Key pressed: ");
    Serial.println(key);
    if (key == RESET_SECRET_CHAR){
      count_key = 0;
      notificado = false;
      codigo_correcto = false;
    } else {
      keys_pressed[count_key] = key;
      count_key++;
      //Serial.print("Count key: ");
      //Serial.println(count_key);
      if (count_key == (SECRET_SIZE)){
        count_key=0;
        codigo_correcto = true;
        for (int i = 0; i < SECRET_SIZE; i++){
            if (keys_pressed[i] != SECRET[i]){
              Serial.print(keys_pressed[i]);
              Serial.print("-");
              Serial.println(SECRET[i]);
              codigo_correcto = false;
              break;
            }
        }
        if (!codigo_correcto){
          if (!notificado){
            Serial.println("Notificando codigo incorrecto");
            notificar(BEEPS_2, estado); 
          }
        } else {
          Serial.println("Codigo correcto");
        }
      }
    }
  }
  //Fin Chequeo codigo

  //Inicio chequeo sensores
  if (digitalRead(PIN_SENSOR_ENTRADA) == LOW && digitalRead(PIN_SENSOR_LAVADERO) == LOW){
    //Serial.println("Sensores bien");
    sensores_ok = true;
  } else {
    //Serial.println("Sensores mal");
    sensores_ok = false;
  }
  //Fin chequeo sensores
  
  switch (estado) {
    case POR_ACTIVARSE:
      //Si paso el tiempo
      if (millis() - previousMillis >= ACTIVATE_DELAY) {
        //Y los sensores estan OK
        if (sensores_ok){
          //Activo la alarma
          estado = ACTIVA_OK;
          digitalWrite(PIN_LED, HIGH);        
          Serial.println("Alarma activada");
        } else {
          //Sino, la vuelvo a inactivar
          Serial.println("Alarma desactivada");
          codigo_correcto = false;
          estado = INACTIVA;
          digitalWrite(PIN_LED, LOW);
        }
      }
    break;
    case ACTIVA_OK:
      if (!sensores_ok){
        codigo_correcto = false;
        Serial.println("Estoy por sonaaaar");
        previousMillis = millis();
        estado = POR_SONAR;
      }
      if (codigo_correcto){
        Serial.println("Alarma desactivada");
        codigo_correcto = false;
        estado = INACTIVA;
        digitalWrite(PIN_LED, LOW);
      }
    break;
    case POR_SONAR:
      //no blocking counter
      if (millis() - previousMillis >= ALARM_DELAY) {
        estado = ACTIVA_SONANDO;
      }
      if (codigo_correcto){
        Serial.println("Alarma desactivada");
        codigo_correcto = false;
        estado = INACTIVA;
        digitalWrite(PIN_LED, LOW);
      }
    break;
    case ACTIVA_SONANDO:
      Serial.println("Estoy sonandoooo");  
      //beep();
      digitalWrite(PIN_SIRENA, HIGH);
      if (codigo_correcto){
        Serial.println("Alarma desactivada");
        codigo_correcto = false;
        estado = INACTIVA;
        digitalWrite(PIN_SIRENA, LOW);
        digitalWrite(PIN_LED, LOW);
      }
    break;
    case INACTIVA:
    if (codigo_correcto){
      codigo_correcto = false;
      if (sensores_ok){
        Serial.println("Estoy por activarme");
        estado = POR_ACTIVARSE;
        previousMillis = millis();
        if (!notificado){
          notificar(BEEPS_1, POR_ACTIVARSE);  
        }
      } else {
        //Serial.println("Codigo correcto - Sensores incorrecto");
        if (!notificado){
          Serial.println("Notificando sensores incorrectos");
          notificar(BEEPS_3, INACTIVA);  
        }
      }
    } else {
      //no hago nada
    }
    break;
    case NOTIFICANDO:
      notificar(tipoNotificacion, estadoAnterior);
    break;
  }
}

void notificar(int tipo, int _estadoAnterior){
  estado = NOTIFICANDO;
  estadoAnterior = _estadoAnterior;
  tipoNotificacion = tipo;
  switch (tipo) {
    case BEEPS_3:
      if (countNotificacion >= 0){
        beep();
        countNotificacion++;
        if (countNotificacion == 3){
          estado = estadoAnterior;
          estadoAnterior = NOTIFICANDO;
          countNotificacion = 0;
          notificado = true;
        }
      }
      break;
    case BEEPS_2:
      if (countNotificacion >= 0){
        beep();
        countNotificacion++;
        if (countNotificacion == 2){
          estado = estadoAnterior;
          estadoAnterior = NOTIFICANDO;
          countNotificacion = 0;
          notificado = true;
        }
      }
      break;
    case BEEPS_1:
      if (countNotificacion >= 0){
        beep();
        countNotificacion++;
        if (countNotificacion == 1){
          estado = estadoAnterior;
          estadoAnterior = NOTIFICANDO;
          countNotificacion = 0;
          notificado = true;
        }
      }
      break;
  }
  delay(100);
}

void beep(){
  for (int i = 0; i < 150; i++) {
    digitalWrite(PIN_SPEAKER, HIGH);
    delayMicroseconds(500);
    digitalWrite(PIN_SPEAKER, LOW);
    delayMicroseconds(500);
  }
}

void blinkLed(){
  digitalWrite(13, HIGH);
  delay(1000);
  digitalWrite(13, LOW);
  delay(1000);
}
