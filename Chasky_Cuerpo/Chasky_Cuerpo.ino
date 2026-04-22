#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

// --- CONFIGURACIÓN HARDWARE ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Servo cuello;
const int PIN_SERVO = 9;

// --- ESTADOS DEL ROBOT ---
String comando = "REPOSO";
unsigned long tiempoAnterior = 0;
bool bocaAbierta = false;

void setup() {
  Serial.begin(9600);
  
  // Inicializar OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Bloqueo si no hay pantalla
  }
  
  // Inicializar Servo
  cuello.attach(PIN_SERVO);
  cuello.write(90); // Posición central
  
  mostrarCaraNormal();
}

void loop() {
  // 1. Escuchar al Cerebro (Python)
  if (Serial.available() > 0) {
    comando = Serial.readStringUntil('\n');
    comando.trim();
  }

  // 2. Ejecutar acciones según el comando
  if (comando == "HABLAR") {
    animacionHablar();
  } 
  else if (comando == "PENSANDO") {
    mostrarCaraPensando();
  } 
  else {
    mostrarCaraNormal();
    cuello.write(90); // Vuelve al centro
  }
}

// --- FUNCIONES DE EXPRESIONES ---

void mostrarCaraNormal() {
  display.clearDisplay();
  // Ojos (rectángulos redondeados)
  display.fillRoundRect(30, 20, 20, 25, 5, WHITE);
  display.fillRoundRect(78, 20, 20, 25, 5, WHITE);
  // Boca simple
  display.drawLine(44, 55, 84, 55, WHITE);
  display.display();
}

void mostrarCaraPensando() {
  display.clearDisplay();
  // Ojos achinados o mirando arriba
  display.fillRoundRect(30, 15, 20, 15, 5, WHITE);
  display.fillRoundRect(78, 15, 20, 15, 5, WHITE);
  // Boca de duda
  display.drawCircle(64, 50, 5, WHITE);
  display.display();
  // Mover el cuello un poquito de lado a lado
  if (millis() % 1000 < 500) cuello.write(85);
  else cuello.write(95);
}

void animacionHablar() {
  // Animación simple de boca y movimiento de servo
  if (millis() - tiempoAnterior > 200) {
    tiempoAnterior = millis();
    bocaAbierta = !bocaAbierta;
    
    display.clearDisplay();
    // Ojos
    display.fillRoundRect(30, 20, 20, 25, 5, WHITE);
    display.fillRoundRect(78, 20, 20, 25, 5, WHITE);
    
    if (bocaAbierta) {
      display.fillCircle(64, 52, 8, WHITE); // Boca abierta
      cuello.write(80);
    } else {
      display.drawLine(44, 55, 84, 55, WHITE); // Boca cerrada
      cuello.write(100);
    }
    display.display();
  }
}
