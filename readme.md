# Minerva's Book

_Minerva's Book_ is a small art installation disguised as a physical, 3D-printed book with an e-ink cover. This e-ink display periodically updates to show a randomly selected news headline fetched from a public news API. The headline is then passed to OpenAI's Chat Completion endpoint to search for real books related to that topic. In essence, the content on the book’s cover reflects current events and literary recommendations tied to those events.

More information can be found on my website: https://www.hinterding.com//projects/minervas-book

## Why "Minerva"?
Minerva was the Roman goddess of wisdom, arts, trade, and strategy. This project borrows her name because it marries the wisdom of literature (books) with daily events (news), creating a dynamic, knowledge-focused artifact.

## How It Works
1. **Connect to Wi-Fi** using the credentials you provide in `SSID` and `PASSWORD`.
2. **Fetch latest news** from the Tagesschau API (`TAGESSCHAU_API`).
3. **Select a random headline** from the fetched JSON data.
4. **Query OpenAI** (`OPENAI_API`) with that headline to find a real, existing book recommendation related to the news topic.
5. **Display the extracted data** (author, title, short description) on an e-ink display.
6. **Go into deep sleep** for a specified duration (`SLEEP_TIME`) to conserve power. After waking, the process repeats, ensuring the content on the cover is updated periodically.

## Technology Stack
- **ESP32** microcontroller (with built-in Wi-Fi)
- **E-Ink (ePaper) Display** (Supported by the [GxEPD2](https://github.com/ZinggJM/GxEPD2) library)
- **OpenAI's Chat Completion API** ([OpenAI API](https://platform.openai.com/))
- **Tagesschau News API** (public endpoint for German news)

## Hardware Requirements
- An ESP32 board
- A supported ePaper display (e.g. Waveshare ePaper board)
- (Optional) On-board LED (for status indication)
- Power supply for the ESP32

## Software Requirements
- **Arduino IDE** (or PlatformIO) with the following libraries installed:
  - [WiFi](https://www.arduino.cc/en/Reference/WiFi) (for ESP32)
  - [HTTPClient](https://www.arduino.cc/reference/en/libraries/arduinohttpclient/)  
  - [ArduinoJson](https://arduinojson.org/)  
  - [GxEPD2_BW](https://github.com/ZinggJM/GxEPD2)
  - [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX)
  - [esp_sleep.h](https://www.arduino.cc/reference/en/libraries/esp-deep-sleep/)
- **OpenAI API Key**: You will need a valid API key to query GPT.

## Getting Started
1. **Clone or download** this repository.
2. **Open** the `.ino` file in Arduino IDE (or relevant file in your platform).
3. **Install dependencies** listed above if not already present.
4. **Configure your Wi-Fi credentials** by replacing:
   ```cpp
   const char* SSID     = "[enter your SSID]";
   const char* PASSWORD = "[enter your WiFi password]";
   const char* OPENAI_KEY  = "[your OpenAI API key]";
   ```
5. Upload the code to your ESP32 board.
6. Power on the device. The first run will connect to Wi-Fi, fetch a news headline, query OpenAI, display the result, then go to sleep.



