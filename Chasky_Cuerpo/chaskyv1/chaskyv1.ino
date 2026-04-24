/**
 * @brief   Control del cuerpo del robot Chasky.
 *          - Servo barre 0°→180°→0° en loop hasta detectar persona.
 *          - Al detectar persona el servo se detiene y mantiene posición.
 *          - Recibe comandos por Serial: HABLAR / PENSANDO / NORMAL.
 * @hardware  Servo → PIN 9 | HC-SR04 TRIG → PIN 8 | ECHO → PIN 7 | 
 *            OLED SSD1306 I2C 128x64 SCK ->	A5 , SDA -> A4
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

// ─── HARDWARE ────────────────────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define PIN_SERVO     9
#define PIN_TRIG      8   // HC-SR04: disparo ultrasónico
#define PIN_ECHO      7   // HC-SR04: recepción del eco

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // instancia del display
Servo servo;

// ─── CENTRO DEL DISPLAY ──────────────────────────────────────────────────────
#define CX 64   // Centro X
#define CY 30   // Centro Y de la cara

// ─── CONFIGURACIÓN SERVO ─────────────────────────────────────────────────────
#define SERVO_MIN_ANGLE   20     // Ángulo mínimo del barrido (grados)
#define SERVO_MAX_ANGLE   160   // Ángulo máximo del barrido (grados)
#define SERVO_PASO_MS     100    // Milisegundos entre cada paso
#define SERVO_PASO_GRADOS 2     // Grados por paso

// ─── CONFIGURACIÓN SENSOR HC-SR04 ────────────────────────────────────────────
#define SENSOR_DISTANCIA_CM  20    // Distancia de detección en centímetros
#define SENSOR_DEBOUNCE_MS   200   // Tiempo que la lectura debe ser estable (ms)

// ─── VARIABLES SERVO ─────────────────────────────────────────────────────────
int   anguloActual        = 0;     // Ángulo actual del servo
int   direccionBarrido    = 1;     // +1 = subiendo(180°), -1 = bajando(0°)
unsigned long ultimoPasoServo = 0; // Tiempo del último paso
bool  barridoActivo       = false; // TRUE = servo barriendo, FALSE = detenido

// ─── VARIABLES SENSOR ────────────────────────────────────────────────────────
bool  personaDetectada    = false; // Estado estable (con debounce)
bool  lecturaAnterior     = false; // Lectura cruda del ciclo anterior
unsigned long tiempoDebounce = 0;  // Cuándo empezó el cambio de lectura

// ─── VARIABLES DISPLAY / COMANDOS ────────────────────────────────────────────
String comando = "";
unsigned long tAnterior  = 0;
const unsigned long INTERVALO_HABLAR = 300;
int frameAltavoz = 0;

// ─── PROTOTIPOS DE FUNCIONES ──────────────────────────────────────────────────────────────
void servo_init();
void servo_startSweep();
void servo_stopAndHold();
void servo_updateSweep();
bool sensor_update();
void sensor_init();
void mostrarCaraNormal();
void mostrarCaraPensando();
void animacionHablar();
// helpers display
void dibujarOjosFelices();
void dibujarSonrisa();
void dibujarMejillas();
void dibujarAltavoz(int frame);


// ═════════════════════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;); // Bloqueo si no hay pantalla
  }

  sensor_init();
  servo_init();
  mostrarCaraNormal();
}


// ═════════════════════════════════════════════════════════════════════════════
//  LOOP
// ═════════════════════════════════════════════════════════════════════════════
void loop() {

  // ── 1. ACTUALIZAR SENSOR (con debounce) ──────────────────────────────────
  personaDetectada = sensor_update();

  // ── 2. CONTROL DEL SERVO ─────────────────────────────────────────────────
  if (personaDetectada) {
    // Hay persona → detener el servo donde está
    servo_stopAndHold();
  } else {
    // No hay persona → barrer en loop
    if (!barridoActivo) {
      servo_startSweep();
    }
    servo_updateSweep();
  }

  // ── 3. LEER COMANDO SERIAL ───────────────────────────────────────────────
  if (Serial.available() > 0) {
    comando = Serial.readStringUntil('\n');
    comando.trim();
  }

  // ── 4. MOSTRAR ANIMACIÓN SEGÚN COMANDO ───────────────────────────────────
  if (comando == "HABLAR") {
    animacionHablar();
  } else if (comando == "PENSANDO") {
    mostrarCaraPensando();
  } else {
    mostrarCaraNormal();
  }
}


// ═════════════════════════════════════════════════════════════════════════════
//  MÓDULO: SERVO
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Inicializa el servo en posición 0° y prepara variables.
 */
void servo_init() {
  servo.attach(PIN_SERVO);
  anguloActual     = SERVO_MIN_ANGLE;
  direccionBarrido = 1;
  barridoActivo    = false;
  servo.write(anguloActual);
  delay(300); // Tiempo para que llegue a posición inicial
}

/**
 * @brief Habilita el barrido automático desde la posición actual.
 * @note  Llamar solo una vez al iniciar el barrido, no en cada ciclo.
 */
void servo_startSweep() {
  barridoActivo   = true;
  ultimoPasoServo = millis(); // Reiniciar timer solo al comenzar
}

/**
 * @brief Detiene el servo en la posición actual (mantiene torque).
 */
void servo_stopAndHold() {
  barridoActivo = false;
  // No llamar servo.detach() → el servo mantiene posición por torque
}

/**
 * @brief Ejecuta un paso del barrido si pasó el tiempo configurado.
 *        Invierte dirección al llegar a los límites 0° y 180°.
 * @note  No bloqueante: usa millis() en lugar de delay().
 */
void servo_updateSweep() {
  unsigned long ahora = millis();

  // Esperar hasta el próximo paso
  if (ahora - ultimoPasoServo < SERVO_PASO_MS) {
    return;
  }
  ultimoPasoServo = ahora; // Reiniciar timer para el próximo paso

  // Avanzar un paso en la dirección actual
  anguloActual += (direccionBarrido * SERVO_PASO_GRADOS);

  // Invertir dirección al llegar a los límites
  if (anguloActual >= SERVO_MAX_ANGLE) {
    anguloActual     = SERVO_MAX_ANGLE;
    direccionBarrido = -1;
  } else if (anguloActual <= SERVO_MIN_ANGLE) {
    anguloActual     = SERVO_MIN_ANGLE;
    direccionBarrido = 1;
  }

  servo.write(anguloActual);
}


// ═════════════════════════════════════════════════════════════════════════════
//  MÓDULO: SENSOR HC-SR04
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Configura los pines TRIG y ECHO del HC-SR04 y limpia el estado inicial.
 */
void sensor_init() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  personaDetectada = false;
  lecturaAnterior  = false;
  tiempoDebounce   = 0;
}

/**
 * @brief Dispara un pulso ultrasónico y mide la distancia resultante.
 * @return Distancia en centímetros. Retorna 0 si la medición es inválida.
 */
float sensor_medirDistancia() {
  // Asegurar nivel bajo antes del disparo
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  // Pulso de disparo de 10 µs
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Medir duración del eco (timeout 30ms → ~500 cm máximo)
  long duracion = pulseIn(PIN_ECHO, HIGH, 30000);

  // Sin eco → objeto fuera de rango o sin respuesta
  if (duracion == 0) return 0;

  // Conversión: velocidad del sonido ~0.034 cm/µs, viaje de ida y vuelta
  return duracion * 0.034 / 2.0;
}

/**
 * @brief Lee el sensor HC-SR04 con debounce por software.
 * @details Mide la distancia y determina si hay un objeto dentro del umbral
 *          SENSOR_DISTANCIA_CM. Si la lectura cruda cambia respecto al ciclo
 *          anterior, espera SENSOR_DEBOUNCE_MS ms estable antes de actualizar
 *          el estado. Evita falsos positivos por reflexiones o ruido.
 * @return  TRUE si hay persona detectada de forma estable, FALSE si no.
 */
bool sensor_update() {
  float distancia    = sensor_medirDistancia();
  bool  lecturaActual = (distancia > 0 && distancia <= SENSOR_DISTANCIA_CM);

  if (lecturaActual != lecturaAnterior) {
    // Hubo un cambio → reiniciar temporizador de debounce
    tiempoDebounce  = millis();
    lecturaAnterior = lecturaActual;
  }

  // Si la lectura es estable por SENSOR_DEBOUNCE_MS ms → aceptarla
  if ((millis() - tiempoDebounce) >= SENSOR_DEBOUNCE_MS) {
    personaDetectada = lecturaActual;
  }

  return personaDetectada;
}


// ═════════════════════════════════════════════════════════════════════════════
//  MÓDULO: DISPLAY — HELPERS
// ═════════════════════════════════════════════════════════════════════════════

/** @brief Dibuja ojos con pupilas, centrados en (CX, CY). */
void dibujarOjosFelices() {
  display.fillCircle(CX - 16, CY - 8, 5, WHITE); // Ojo izq
  display.fillCircle(CX + 16, CY - 8, 5, WHITE); // Ojo der
  display.fillCircle(CX - 16, CY - 9, 2, BLACK); // Pupila izq
  display.fillCircle(CX + 16, CY - 9, 2, BLACK); // Pupila der
}

/** @brief Dibuja sonrisa en arco con 4 segmentos de línea. */
void dibujarSonrisa() {
  display.drawLine(CX - 18, CY + 10, CX - 12, CY + 18, WHITE);
  display.drawLine(CX - 12, CY + 18, CX,      CY + 22, WHITE);
  display.drawLine(CX,      CY + 22, CX + 12, CY + 18, WHITE);
  display.drawLine(CX + 12, CY + 18, CX + 18, CY + 10, WHITE);
}

/** @brief Dibuja círculos de mejillas. */
void dibujarMejillas() {
  display.drawCircle(CX - 20, CY + 6, 4, WHITE);
  display.drawCircle(CX + 20, CY + 6, 4, WHITE);
}

/**
 * @brief Dibuja ícono de altavoz con barras de volumen.
 * @param frame  0 = sin barras | 1 = baja | 2 = media | 3 = alta
 */
void dibujarAltavoz(int frame) {
  const int AX = 100;
  const int AY = 48;

  // Cuerpo del altavoz
  display.fillRect(AX, AY - 3, 5, 6, WHITE);
  // Cono
  display.drawLine(AX + 5, AY - 3, AX + 9, AY - 6, WHITE);
  display.drawLine(AX + 5, AY + 3, AX + 9, AY + 6, WHITE);
  display.drawLine(AX + 9, AY - 6, AX + 9, AY + 6, WHITE);

  // Barras de volumen (crecen en altura)
  if (frame >= 1) display.fillRect(AX + 12, AY - 2, 2,  4, WHITE); // baja
  if (frame >= 2) display.fillRect(AX + 15, AY - 3, 2,  7, WHITE); // media
  if (frame >= 3) display.fillRect(AX + 18, AY - 5, 2, 10, WHITE); // alta
}


// ═════════════════════════════════════════════════════════════════════════════
//  MÓDULO: DISPLAY — EXPRESIONES
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Cara feliz estática (estado NORMAL).
 *        Ojos + pupilas + sonrisa + mejillas.
 */
void mostrarCaraNormal() {
  display.clearDisplay();
  dibujarOjosFelices();
  dibujarSonrisa();
  dibujarMejillas();
  display.display();
}

/**
 * @brief Cara de sorpresa (estado PENSANDO).
 *        Cejas levantadas, ojos grandes, boca en "O", líneas de asombro.
 */
void mostrarCaraPensando() {
  display.clearDisplay();

  // Cejas levantadas
  display.drawLine(CX - 36, CY - 20, CX - 20, CY - 17, WHITE);
  display.drawLine(CX + 20, CY - 17, CX + 36, CY - 20, WHITE);

  // Ojos muy abiertos
  display.drawCircle(CX - 24, CY - 9, 7, WHITE);
  display.drawCircle(CX + 24, CY - 9, 7, WHITE);
  display.fillCircle(CX - 24, CY - 8, 3, WHITE); // Pupilas
  display.fillCircle(CX + 24, CY - 8, 3, WHITE);
  display.fillCircle(CX - 22, CY - 10, 1, BLACK); // Brillo
  display.fillCircle(CX + 26, CY - 10, 1, BLACK);

  // Boca en "O"
  display.drawCircle(CX, CY + 18, 7, WHITE);
  display.fillCircle(CX, CY + 18, 5, WHITE);
  display.fillCircle(CX, CY + 18, 3, BLACK);

  // Líneas de asombro
  display.drawLine(CX + 36, CY - 12, CX + 42, CY - 8, WHITE);
  display.drawLine(CX + 36, CY - 2,  CX + 44, CY - 2, WHITE);
  display.drawLine(CX + 36, CY + 8,  CX + 42, CY + 4, WHITE);

  display.display();
}

/**
 * @brief Cara feliz con altavoz animado (estado HABLANDO).
 *        El altavoz cicla 0→3 barras cada INTERVALO_HABLAR ms.
 */
void animacionHablar() {
  unsigned long ahora = millis();
  if (ahora - tAnterior >= INTERVALO_HABLAR) {
    tAnterior    = ahora;
    frameAltavoz = (frameAltavoz + 1) % 4; // 0→1→2→3→0
  }

  display.clearDisplay();
  dibujarOjosFelices();
  dibujarSonrisa();
  dibujarMejillas();
  dibujarAltavoz(frameAltavoz);
  display.display();
}

