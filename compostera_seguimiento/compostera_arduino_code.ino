
/*
PROGRAMACION DE PROYECTO DE MONITOREO DE UNA COMPOSTERA:

funcionalidad de los módulos usados:
- reloj (RTC DS1307)
- lectorSD
- sensor de temperatura DS18B20
actuadores:
- leds (3 colores por separado)
- buzzer
cpu:
- arduino UNO


Here is the link to my project at Wokwi: https://wokwi.com/projects/414748846045857793 
*/

// LIBRERIAS
#include <OneWire.h>            // Comunicación con sensores OneWire
#include <DallasTemperature.h>  // Lectura de sensores de temperatura
#include <Wire.h>               // Comunicación I2C entre dispositivos
#include <RTClib.h>             // Manejo de tiempo con reloj RTC
#include <SD.h>                 // Control de tarjetas SD

// CONEXIONES
#define sensor_temperatura 2    // Pin para DS18B20 (temperatura)
#define init_btn 4              // Pin para el botón
#define reset_btn 5             // Pin para el botón reset
#define buzzer 6                // Pin para el buzzer
#define led_red 7               // Pin para LEDrojo
#define led_green 8             // Pin para LEDverde
#define led_yellow 9            // Pin para LEDamarillo
#define SDpin 10                // Pin CS para tarjeta SD

OneWire oneWire(sensor_temperatura); // Configuración de comunicacion en el pin 2
DallasTemperature sensors(&oneWire); // Instaciacion de la clase DallasTemperature

RTC_DS1307 rtc; // Configuración del RTC DS1307

File dataFile; // Instanciacion de la clase File

// Variables de tiempo necesarias para la lógica
unsigned long lastTime = 0;             // Última vez que se registraron datos
unsigned long lastTimeaire = 0;         // Última vez que se registraron datos
const unsigned long intervalreg = 1000; // Intervalo de 1 hora (3.600.000 ms)
const unsigned long intervalaire =8000; // Intervalo de 1 hora (309.600.000 ms)
bool timerStarted = false;              // Bandera para saber si el temporizador debe comenzar
int contadorBoton=0;

// Banderas globales para indicar el paso por los estados
bool mesofila=false;
bool termofila=false;
bool maduracion=false;
bool sinaire = false; 

float temperatura = 0; // Variable global de lectura de temperatura


void setup() {
  Serial.begin(9600); //Iniciamos la comunicacion con el puerto serial
  
  // Configuración de pines
  pinMode(init_btn, INPUT_PULLUP);
  pinMode(reset_btn, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  pinMode(led_red, OUTPUT);
  pinMode(led_green, OUTPUT);
  pinMode(led_yellow, OUTPUT);

  sensors.begin();  // Inicializar sensor de temperatura

  if (!rtc.begin()) {                                                   // Inicializar RTC (Real Time Clock)
    Serial.println("Error al inicializar el RTC");
    while (1);
  }
  if (!rtc.isrunning()) {
    Serial.println("RTC no está en marcha, configurando la hora...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!SD.begin(SDpin)) {                                               // Inicializar tarjeta SD
    Serial.println("Error al inicializar la tarjeta SD");
    while (1);
  }
  Serial.println("Tarjeta SD inicializada correctamente");
}

void loop() {

  if (digitalRead(init_btn) == LOW){         // Emulacion de la tapa abriendose y cerrandose con el contador del boton en 2
    contadorBoton= contadorBoton+1;
    delay(500);
  }

  sensors.requestTemperatures();              // Leemos la temperatura del sensor y la almacenamos en la variable global
  temperatura = sensors.getTempCByIndex(0);

  if (temperatura>=50){                       // ETAPA TERMOFILA ALCANZADA
    termofila=true;
    digitalWrite(led_red, HIGH);
  }

  if (termofila==true && temperatura<50){     // ETAPA MESOFILA ALCANZADA
    mesofila=true;
  }

  if (mesofila==true && temperatura<20){
    maduracion=true;                          // ETAPA DE MADURACION ALCANZADA
    digitalWrite(led_green, HIGH);
  }

  if (contadorBoton==2 && !timerStarted) {
    timerStarted = true;                      // Iniciar el primer estado cuando se cerro la tapa por primera vez (carga finalizada)
    lastTime = millis();                      // Establecer el tiempo inicial para el registro de datos
    lastTimeaire = millis();                  // Establecer el tiempo inicial para el registro del aire
    registrarDatos();                         // Registrar datos inmediatamente al pulsar el botón
    delay(1000);                              // Esperar un segundo para evitar múltiples registros con el rebote del boton
  }

  if (timerStarted) {

    unsigned long currentMillis = millis();         // Temporizacion para el registro de datos

    if (currentMillis - lastTime >= intervalreg) {
      lastTime = currentMillis; 
      registrarDatos();
    }
  }

  if (timerStarted) {                                                         // Monitoreo del estado "Lleno sin aire" - se revuelve cada 86 horas

    unsigned long currentMillis2 = millis();

    if (currentMillis2 - lastTimeaire >= intervalaire && sinaire==false) {
      sinaire=true;
      digitalWrite(led_yellow, HIGH);
    }

    if(sinaire=true && digitalRead(init_btn) == LOW){
      lastTimeaire = currentMillis2;
      sinaire=false;
      digitalWrite(led_yellow, LOW);
    }
  }

// FUNCION DE RESET
if (maduracion==true && digitalRead(reset_btn) == LOW){     // Solo se puede resetear si paso la maduracion
  unsigned long lastTime = 0;        
  unsigned long lastTimeaire = 0;         
  bool timerStarted = false;          
  int contadorBoton=0;
  bool mesofila=false;
  bool termofila=false;
  bool maduracion=false;
  bool sinaire = false; 
  float temperatura = 0;
  digitalWrite(led_red, LOW);
  digitalWrite(led_green, LOW);
  digitalWrite(led_yellow, LOW);
  asm volatile ("jmp 0");          // Utilizamos Assembly para volver a la posicion 0 de memoria donde comienza el codigo nuevamente (con restricciones)
  }
}

void registrarDatos() {
  
  sensors.requestTemperatures();
  temperatura = sensors.getTempCByIndex(0);   // Leer temperatura para registrar en la SD

  DateTime now = rtc.now();   // Leer hora del RTC para el registro en la SD

  Serial.print("Fecha y hora: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);                            // Mostrar en el monitor serial la lectura que se va a registrar
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.print(" | Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");

  dataFile = SD.open("data.txt", FILE_WRITE);               // Escribe en el archivo 'data.txt' o lo crea si no existe
  if (dataFile) {
    dataFile.print(now.year(), DEC);
    dataFile.print('/');
    dataFile.print(now.month(), DEC);
    dataFile.print('/');
    dataFile.print(now.day(), DEC);
    dataFile.print(" ");
    dataFile.print(now.hour(), DEC);
    dataFile.print(':');
    dataFile.print(now.minute(), DEC);
    dataFile.print(':');
    dataFile.print(now.second(), DEC);
    dataFile.print(" | ");
    dataFile.print("Temp: ");
    dataFile.print(temperatura);
    dataFile.println(" °C");
    dataFile.close();
    Serial.println("Datos guardados en la tarjeta SD");

  } else {
    Serial.println("Error al abrir el archivo en la tarjeta SD");
  }

  tone(buzzer, 1000); // Encender LEDs y sonar buzzer como indicación de registro
  delay(500);
  noTone(buzzer);
}