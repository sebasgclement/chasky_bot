# 🤖 Chasky - Asistente de IA para FLISOL Rafaela

¡Bienvenidos al repositorio oficial de **Chasky**! 

Chasky es un robot asistente de inteligencia artificial, 100% de código abierto, diseñado para ser el anfitrión del **FLISOL (Festival Latinoamericano de Instalación de Software Libre) en Rafaela**. 

Fue creado por **Francisco Bevilacqua** y **Sebastián Clement** para demostrar el poder de la inteligencia artificial local y el hardware libre trabajando en conjunto.

## 🧠 ¿Cómo funciona?
Chasky está dividido en dos partes principales:
1. **El Cerebro (Python + Ollama):** Escucha por micrófono, transcribe la voz a texto, procesa la pregunta usando un modelo de lenguaje local (Llama) y responde por voz usando síntesis de texto a voz.
2. **El Cuerpo (Arduino):** Recibe comandos por conexión Serial desde el Cerebro y cobra vida moviendo un servomotor y gesticulando expresiones en una pantalla OLED.

---

## 🛠️ Requisitos de Hardware
* Una PC con Linux (testeado en Linux Mint).
* Micrófono y parlantes.
* Placa Arduino (Uno/Nano/Mega).
* Pantalla OLED I2C (128x64).
* Servomotor SG90.

---

## 🚀 Instalación y Configuración del Cerebro

### 1. Preparar el motor de IA (Ollama)
Chasky necesita Ollama para pensar de forma local sin depender de internet.
* Descargá e instalá [Ollama](https://ollama.com/).
* Abrí la terminal y descargá el modelo que vayas a usar (por defecto usamos una versión ligera para PCs estándar):
  ```bash
  ollama run llama3.2:1b
