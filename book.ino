#include <GxEPD2_BW.h>            // ePaper library for GxEPD2 (black & white ePaper displays)
#include <WiFi.h>                 // ESP32 WiFi functions
#include <HTTPClient.h>           // For making HTTP requests
#include <ArduinoJson.h>          // JSON parsing and serialization
#include <esp_sleep.h>            // Deep sleep functions
#include <U8g2_for_Adafruit_GFX.h>// Font rendering with U8g2 on Adafruit GFX

// Select the display class and display driver class in the following files
#include "GxEPD2_display_selection_new_style.h"
#include "GxEPD2_display_selection.h"
#include "GxEPD2_display_selection_added.h"

// Credentials and API endpoints
const char* SSID        = "[enter your SSID]";      // Replace with your WiFi SSID
const char* PASSWORD    = "[enter your WiFi password]";  // Replace with your WiFi password
const char* TAGESSCHAU_API = "https://www.tagesschau.de/api2u/channels";
const char* OPENAI_API  = "https://api.openai.com/v1/chat/completions";
const char* OPENAI_KEY  = "[your OpenAI API key]"; // OpenAI API key
const char* LLM         = "gpt-4o-mini";   // The model you want to use

// Sleep time in microseconds: 3600s = 1 hour, multiplied by 12 = 12 hours
#define SLEEP_TIME 3600ULL * 1000000ULL * 12ULL

// On-board LED pin (for some development boards)
#define LED_BUILTIN 2

U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn on LED as an indicator
  Serial.begin(115200);

  // Initialize ePaper display
  displayInit();

  // Connect to WiFi
  connectToWifi();

  // Fetch JSON from Tagesschau and pick a random message
  String json = fetchAndProcessJSON();
  String message = chooseMessage(json);
  Serial.println(message);
  
  // Query GPT with the selected message, print result to Serial
  String analysis = queryGPT(message);
  Serial.println("------- start -------");
  Serial.println(getJSONValue(analysis, "description"));
  Serial.println(getJSONValue(analysis, "title"));
  Serial.println(getJSONValue(analysis, "author"));
  Serial.println("------- end -------");
 
  // Insert line breaks for better formatting on the ePaper display
  String author = addLineBreaks(getJSONValue(analysis, "author"), 40);
  String title = addLineBreaks(getJSONValue(analysis, "title"), 18);
  String description = addLineBreaks(getJSONValue(analysis, "description"), 50);
  
  // Display the extracted data (author, title, description) on the ePaper
  displayText("author", author.c_str());
  displayText("title", title.c_str());
  displayText("description", description.c_str());
  
  display.display();   // Trigger the ePaper to refresh
  display.hibernate(); // Put ePaper into low power mode

  // Finally, go to deep sleep for the specified duration
  goToDeepSleep();
}

/**
 * @brief Inserts line breaks into a String at a defined interval or at the last space.
 * 
 * This function searches for a space within the next 'number' characters
 * and inserts a newline. If no space is found, it forces a newline at the 'number'
 * character. This helps in nicely wrapping text for display.
 * 
 * @param text   The original String in which to insert line breaks
 * @param number The maximum number of characters before wrapping
 * @return String The modified String with inserted line breaks
 */
String addLineBreaks(String text, int number) {
  int length = text.length();
  int currentIndex = 0;

  while (currentIndex < length) {
    // Find the last space character within the next 'number' characters
    int breakIndex = text.lastIndexOf(' ', currentIndex + number);

    // If no space is found, or it's out of range, use the exact 'number' position
    if (breakIndex == -1 || breakIndex <= currentIndex) {
      breakIndex = currentIndex + number;
      if (breakIndex >= length) break; // Avoid out-of-bounds
    }

    // Insert a newline at the break index
    text = text.substring(0, breakIndex) + "\n" + text.substring(breakIndex + 1);

    // Move currentIndex past the inserted newline
    currentIndex = breakIndex + 1;

    // Update the length of the text due to the insertion of the newline
    length = text.length();
  }

  return text;
}

/**
 * @brief Initializes the ePaper display with specific settings.
 * 
 * Includes setting the rotation, clearing the screen, 
 * and configuring U8G2 for use with Adafruit GFX.
 */
void displayInit(){
  // For Waveshare boards with a "clever" reset circuit, 2ms reset pulse
  display.init(115200, true, 2, false);
  display.setRotation(3);         // Adjust rotation if your display is oriented differently
  display.fillScreen(GxEPD_WHITE);// Clear the screen (white background)
  display.setFullWindow();        // Use the full screen for drawing
  display.firstPage();            // Required for GxEPD2 library usage

  // Initialize U8G2 wrapper for custom font usage
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFontDirection(0);   // Left to right (default)
  u8g2_for_adafruit_gfx.setForegroundColor(0); // 0 = black in Adafruit GFX
  u8g2_for_adafruit_gfx.setBackgroundColor(1); // 1 = white in Adafruit GFX
}

/**
 * @brief Displays text (author, title, or description) on the ePaper
 *        by calculating line placement and using helper functions.
 * 
 * @param type A string that determines which font and position settings to use
 * @param text The text to display
 */
void displayText(String type, const char* text)
{
  int16_t characterLengthOffset = 0;
  int16_t lineHeight = 0;
  int16_t y = 0;
  int16_t xOffset = 0;

  // Configure different display settings based on type
  if(type == "author"){
    u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR14_tf);
    characterLengthOffset = -2;   // Adjust horizontal offset for author
    xOffset = 0;
    lineHeight = 20;             // Vertical spacing between lines
    y = 100;                      // Starting vertical position
  }

  if(type == "title"){
    u8g2_for_adafruit_gfx.setFont(u8g2_font_osb35_tf);
    characterLengthOffset = -9;  // Adjust horizontal offset for title
    xOffset = 0;
    lineHeight = 54;             // Vertical spacing for larger font
    y = 180;                     
  }

  if(type == "description"){
    u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR14_tf);
    characterLengthOffset = -2;  // Adjust horizontal offset for description
    xOffset = 26;
    lineHeight = 26;
    y = 480;
  }
  
  // Split the text into lines by the '\n' character
  String str = String(text);
  int startIndex = 0;
  int endIndex = str.indexOf('\n');

  while (endIndex != -1) {
    String line = str.substring(startIndex, endIndex);
    displayCenteredLine(line.c_str(), characterLengthOffset * line.length() + xOffset, y);
    y += lineHeight; // move to next line
    startIndex = endIndex + 1;
    endIndex = str.indexOf('\n', startIndex);
  }

  // Handle the last line if there's no more '\n'
  String lastLine = str.substring(startIndex);
  displayCenteredLine(lastLine.c_str(), characterLengthOffset * lastLine.length() + xOffset, y);
}

/**
 * @brief Draws a single line of text centered horizontally on the display.
 * 
 * @param text                  The text to display
 * @param characterLengthOffset Additional horizontal offset based on character length
 * @param y                     The vertical position to place the text
 */
void displayCenteredLine(const char* text, int16_t characterLengthOffset, int16_t y)
{
  int16_t tbx, tby; 
  uint16_t tbw, tbh;

  // Calculate text bounds for centering
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
  
  // Center the line with offset
  int16_t x = ((display.width() - tbw) / 2) + characterLengthOffset - tbx / 2;
  u8g2_for_adafruit_gfx.setCursor(x, y);
  u8g2_for_adafruit_gfx.print(text);
}

/**
 * @brief Main loop function. Empty in this case because we use deep sleep.
 */
void loop() {
  // Everything is done in setup(); deep sleep is used to save power.
}

/**
 * @brief Puts the ESP32 into deep sleep mode for the predefined SLEEP_TIME.
 */
void goToDeepSleep() {
  Serial.println("Going to sleep for " + String(SLEEP_TIME/1000000/60) + " minutes");
  
  // Shut down WiFi to save power
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.end();
  delay(500);  // Ensure all serial output is complete
  
  // Enable the timer wakeup for SLEEP_TIME microseconds
  esp_sleep_enable_timer_wakeup(SLEEP_TIME);
  esp_deep_sleep_start();         // Enter deep sleep
}

/**
 * @brief Connects to WiFi using the provided SSID and PASSWORD.
 * 
 * Retries up to 20 times. If unsuccessful, goes to deep sleep.
 */
void connectToWifi(){
  int connectionAttempts = 0;

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED && connectionAttempts < 20) {
    delay(500);
    Serial.print(".");
    connectionAttempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi.");
    goToDeepSleep();
  }
}

/**
 * @brief Fetches JSON data from the Tagesschau API, extracts the 'content' fields,
 *        and returns a reduced JSON string containing only those fields.
 * 
 * @return String Reduced JSON with only 'content' fields
 */
String fetchAndProcessJSON() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(TAGESSCHAU_API);
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();

      const size_t capacity = 16384;
      DynamicJsonDocument doc(capacity);
      deserializeJson(doc, payload);

      // Create a new JSON doc for reduced data
      DynamicJsonDocument reducedDoc(capacity);
      JsonArray channelsArray = doc["channels"].as<JsonArray>();
      JsonArray reducedContentArray = reducedDoc.createNestedArray("content");

      // Loop through channels, then through content array
      for (JsonObject channelItem : channelsArray) {
        if (channelItem.containsKey("content")) {
          JsonArray contentArray = channelItem["content"].as<JsonArray>();
          for (JsonObject contentItem : contentArray) {
            // Copy only the "value" field
            JsonObject reducedContentItem = reducedContentArray.createNestedObject();
            reducedContentItem["value"] = contentItem["value"];
          }
        }
      }

      // Serialize the reduced document to a String
      String reducedJson;
      serializeJson(reducedDoc, reducedJson);
      return reducedJson;
    }
    http.end();
  }
  return "";  // Return empty string if not connected or HTTP fail
}

/**
 * @brief Chooses a random "value" field from the "content" array in the JSON
 *        for further processing (e.g., feeding into GPT).
 * 
 * @param json A JSON string containing "content" array
 * @return String Randomly selected content/value
 */
String chooseMessage(String json) {
  const size_t capacity = 16384;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, json);

  JsonArray contentArray = doc["content"].as<JsonArray>();
  int index = random(contentArray.size());
  return contentArray[index]["value"].as<String>();
}

/**
 * @brief Sends a message to the specified OpenAI Chat Completion endpoint
 *        and returns the GPT response in JSON format (as a String).
 * 
 * @param message The text prompt to be passed to GPT
 * @return String GPT response in JSON format (or an error message)
 */
String queryGPT(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connecting to GPT...");
  
    HTTPClient http;
    http.begin(OPENAI_API);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + OPENAI_KEY);

    // Build the system and user messages for the GPT prompt
    String prompt = "Research existing, real books related to the topic [" + message + "]. "
                    "Return exactly one result in this format: { 'author': 'author of the book', "
                    "'title': 'title of the book', 'description': 'Summary of the book, not longer than 3 sentences.' }";
    Serial.println(prompt);

    DynamicJsonDocument doc(1024);
    doc["model"] = LLM;

    // Create the messages array: system and user
    JsonArray messages = doc.createNestedArray("messages");
    JsonObject systemMessage = messages.createNestedObject();
    systemMessage["role"] = "system";
    systemMessage["content"] = "You are a helpful assistant.";
    JsonObject userMessage = messages.createNestedObject();
    userMessage["role"] = "user";
    userMessage["content"] = prompt;

    // Serialize JSON request to string
    String requestBody;
    serializeJson(doc, requestBody);

    // POST request to OpenAI
    int httpResponseCode = http.POST(requestBody);
    String response = http.getString();
    http.end();

    // If everything went fine, parse the content out of the JSON
    if (httpResponseCode == HTTP_CODE_OK) {
      DynamicJsonDocument responseDoc(2048);
      deserializeJson(responseDoc, response);

      // Extract the relevant text from the JSON response
      String content = responseDoc["choices"][0]["message"]["content"];
      Serial.println(content);
      return content;
    } else {
      return "Error querying GPT: " + String(httpResponseCode);
    }
  }
  return "WiFi not connected";
}

/**
 * @brief Extracts a specific field from a JSON string.
 * 
 * @param analysis A JSON string containing fields like "author", "title", "description"
 * @param field    The field name to extract
 * @return String  The value of the requested field
 */
String getJSONValue(String analysis, String field) {
  const size_t capacity = 2048;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, analysis);

  // Return the requested field as String
  String value = doc[field].as<String>();
  return value;
}
