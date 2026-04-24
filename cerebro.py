import ollama
import speech_recognition as sr
import time
import sys
import serial
import os
from gtts import gTTS
import pygame

# Inicializar el mezclador de audio para reproducir la voz
pygame.mixer.init()

# --- CONFIGURACIÓN ---
NOMBRE_OFICIAL = "Chasky"
ALIASES = ["chasky", "chasqui", "chaski", "husky", "chucky", "jackie", "charly", "robot", "computadora", "flisol", "asistente", "aquí"]
PUERTO_SERIAL = '/dev/ttyACM0' 

# --- INICIALIZAR CONEXIÓN CON ARDUINO ---
try:
    print(f"[⚙️] Conectando con el cuerpo en {PUERTO_SERIAL}...")
    arduino = serial.Serial(PUERTO_SERIAL, 9600, timeout=1)
    time.sleep(2) # Espera a que el Arduino reinicie
    print("[✅] Conexión serial establecida.")
except Exception as e:
    arduino = None
    print(f"[⚠️] No se detectó el Arduino. El cerebro funcionará solo. Error: {e}")

# --- LA MEMORIA DE CHASKY ---
CONTEXTO = (
    "Sos Chasky, un robot asistente de inteligencia artificial anfitrión del FLISOL Rafaela. "
    "Tus creadores son Francisco Bevilacqua y Sebastián Clement. "
    "Sos un programa de computadora de software libre. "
    #"Si te preguntan qué estudiás, qué hacés o por tu vida personal, respondé EXCLUSIVAMENTE que sos un código y tu única misión es ayudar en el FLISOL. "
    "Para cualquier otra pregunta sobre el mundo (ciencia, geografía, etc.), respondé con la verdad objetiva. "
    #"Regla estricta: Respondé SIEMPRE en 1 o 2 oraciones máximo, directo al grano."
)

# --- 1. SISTEMA DE VOZ (gTTS + Pygame) ---
def hablar(texto):
    print(f"🤖 {NOMBRE_OFICIAL}: {texto}")
    
    # Avisamos al cuerpo que empiece a mover la boca
    if arduino:
        arduino.write('HABLAR\n'.encode('utf-8'))
        
    try:
        # Generar el audio con la voz de Google (es-us suele sonar mejor y más neutral)
        tts = gTTS(text=texto, lang='es-us', slow=False)
        tts.save("respuesta.mp3")
        
        # Cargar y reproducir el audio
        pygame.mixer.music.load("respuesta.mp3")
        pygame.mixer.music.play()
        
        # Esperar a que termine de reproducir para continuar
        while pygame.mixer.music.get_busy():
            pygame.time.Clock().tick(10)
            
        # Liberar el archivo de audio de la memoria y borrarlo
        pygame.mixer.music.unload()
        os.remove("respuesta.mp3")
        
    except Exception as e:
        print(f"[!] Error al generar el audio: {e}")
    
    # Avisamos al cuerpo que vuelva a posición de reposo
    if arduino:
        arduino.write('REPOSO\n'.encode('utf-8'))

# --- 2. SISTEMA DE APAGADO ---
def shutdown():
    print("\n\n[🛑] APAGADO DE EMERGENCIA")
    if arduino:
        arduino.write('REPOSO\n'.encode('utf-8'))
    hablar("Sistemas cerrados. Nos vemos en el FLISOL.")
    sys.exit(0)

# --- 3. CONFIGURACIÓN MICRÓFONO ---
r = sr.Recognizer()
m = sr.Microphone()
r.energy_threshold = 200
r.dynamic_energy_threshold = True
r.pause_threshold = 2.0
r.non_speaking_duration = 0.5

print("\n" + "="*45)
print("   SISTEMA CHASKY ACTIVO Y CONECTADO   ")
print("="*45 + "\n")

with m as source:
    r.adjust_for_ambient_noise(source, duration=2)
    hablar("Listo. Escuchando.")

try:
    while True:
        with m as source:
            print("\n[🎤] Esperando...")
            try:
                audio = r.listen(source, timeout=5, phrase_time_limit=10)
                frase = r.recognize_google(audio, language="es-AR").lower()
                print(f"[DEBUG] Google entendió: '{frase}'")
                
                if any(alias in frase for alias in ALIASES):
                    if "apagar" in frase or "chau" in frase:
                        shutdown()
                    
                    print(f"[{NOMBRE_OFICIAL}] ¡Me llamaron! Pensando...")
                    
                    # Avisamos al Arduino que estamos procesando (para que ponga carita de duda)
                    if arduino:
                        arduino.write('PENSANDO\n'.encode('utf-8'))
                    
                    pregunta_limpia = frase
                    for alias in ALIASES:
                        pregunta_limpia = pregunta_limpia.replace(alias, "")
                    pregunta_limpia = pregunta_limpia.strip()
                    
                    if not pregunta_limpia:
                        hablar("¡Acá estoy! ¿Qué necesitás?")
                        continue

                    # Elección de modelo según la compu
                    MODELO = 'llama3'

                    res = ollama.chat(
                        model=MODELO, 
                        messages=[
                            {'role': 'system', 'content': CONTEXTO},
                            {'role': 'user', 'content': pregunta_limpia}
                        ]
                    )
                    
                    hablar(res['message']['content'])
                    
            except sr.WaitTimeoutError:
                pass
            except sr.UnknownValueError:
                pass
            except Exception as e:
                print(f"[!] Error: {e}")
                time.sleep(1)

except KeyboardInterrupt:
    shutdown()