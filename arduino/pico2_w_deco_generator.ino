#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define TFT_CS   17
#define TFT_DC   16
#define TFT_RST  20

Adafruit_ST7789 tft = Adafruit_ST7789(
    &SPI,
    TFT_CS,
    TFT_DC,
    TFT_RST
);

const char* ssid = "xxxxxx";
const char* password = "xxxxxx";

// IP 
const char* serverIP = "192.xxx.x.xxx";
const int serverPort = 8080;

WiFiClient client;

// --------------------------------------------------
// Display
// --------------------------------------------------

const int WIDTH  = 240;
const int HEIGHT = 240;

uint16_t buffer[WIDTH * HEIGHT];

int tileSize;
int tilesX;
int tilesY;

int mode;

// --------------------------------------------------
// Shader state
// --------------------------------------------------

float seed = 1.0;

// --------------------------------------------------
// Theme state
// --------------------------------------------------

enum ColorTheme {
    THEME_RANDOM,
    THEME_MONOCHROMATIC,
    THEME_COMPLEMENTARY,
    THEME_ANALOGOUS,
    THEME_TRIADIC,
    THEME_TETRADIC,
    THEME_WARM,
    THEME_SPLIT_COMPLEMENTARY
};

ColorTheme colorTheme;

uint16_t colorMain;
uint16_t colorSecond;
uint16_t colorThird;
uint16_t colorFourth;

// --------------------------------------------------
// GLSL compatibility functions
// --------------------------------------------------

float fract(float x) {
  return x - floor(x);
}

float smoothstep(float edge0, float edge1, float x) {

  float t = constrain(
    (x - edge0) / (edge1 - edge0),
    0.0f,
    1.0f
  );

  return t * t * (3.0f - 2.0f * t);
}

// --------------------------------------------------
// RGB565
// --------------------------------------------------

uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) |
         ((g & 0xFC) << 3) |
         (b >> 3);
}

// --------------------------------------------------
// HSV -> RGB565
// --------------------------------------------------

uint16_t hsv565(float h, float s, float v) {

  h = fmod(h, 360.0f);
  if (h < 0) h += 360.0f;

  float r, g, b;

  int i = int(h / 60.0f);
  float f = (h / 60.0f) - i;

  float p = v * (1.0f - s);
  float q = v * (1.0f - f * s);
  float t = v * (1.0f - (1.0f - f) * s);

  switch (i) {
    case 0:
      r = v; g = t; b = p;
      break;

    case 1:
      r = q; g = v; b = p;
      break;

    case 2:
      r = p; g = v; b = t;
      break;

    case 3:
      r = p; g = q; b = v;
      break;

    case 4:
      r = t; g = p; b = v;
      break;

    default:
      r = v; g = p; b = q;
      break;
  }

  return color565(
    (uint8_t)(r * 255.0f),
    (uint8_t)(g * 255.0f),
    (uint8_t)(b * 255.0f)
  );
}

// --------------------------------------------------
// Text Helper
// --------------------------------------------------

void drawText(const char *text, uint16_t color, int y)
{
    tft.setCursor(0, y);
    tft.setTextColor(color);
    tft.setTextWrap(true);
    tft.print(text);
}

// --------------------------------------------------
// 2D hash incorporating our seed
// --------------------------------------------------

float tileHash(float x, float y) {

  float n =
    sin(
      x * 227.1f +
      y * 311.7f +
      seed * 74.7f
    ) * 4375.5453f;

  return n - floor(n);
}

// --------------------------------------------------
// Should Subdivide function
// --------------------------------------------------

bool shouldSubdivide(float hs, float tr) {
  return hs > tr;
}

// --------------------------------------------------
// Main procedural drawing function
// --------------------------------------------------

void drawTilePattern() {

  float resolution = 240.0f;

  // This is the shader's scale for a smaller display.
  float scale = 1.f;
  float px = mode == 3 ? random(1.0,5.0) : 1.0f / resolution * scale;

  // ------------------------------------------------
  // First pass
  // ------------------------------------------------

  for (int y = 0; y < HEIGHT; y++) {

    for (int x = 0; x < WIDTH; x++) {

      int idX = x / tileSize;
      int idY = y / tileSize;

      int localX = x % tileSize;
      int localY = y % tileSize;

      float currentTileSize = tileSize;

      // --------------------------------------------------
      // Occasionally subdivide the tile
      // --------------------------------------------------

      float parentHash = tileHash(idX, idY);

      if (shouldSubdivide(parentHash, currentTileSize > 120 ? .425 : .85)) {

        // ----------------------------------------------
        // First subdivision
        // ----------------------------------------------

        int half = tileSize / 2;

        int subX = localX / half;
        int subY = localY / half;

        localX = localX % half;
        localY = localY % half;

        currentTileSize = half;

        idX = idX * 2 + subX;
        idY = idY * 2 + subY;


        // ----------------------------------------------
        // Second subdivision
        // ----------------------------------------------

        float subHash = tileHash(idX, idY);

        if (shouldSubdivide(subHash, currentTileSize > 120 ? .25 : .95)) {

          int half2 = currentTileSize / 2;

          int subX2 = localX / half2;
          int subY2 = localY / half2;

          localX = localX % half2;
          localY = localY % half2;

          currentTileSize = half2;

          idX = idX * 2 + subX2;
          idY = idY * 2 + subY2;
        }
      }

      // --------------------------------------------------
      // Position inside current tile
      // --------------------------------------------------

      float pX =
        ((float)localX + 0.5f) /
        currentTileSize - 0.5f;

      float pY =
        ((float)localY + 0.5f) /
        currentTileSize - 0.5f;

      // Random value for this tile
      float hs = tileHash(idX, idY);

      float tk = 0.0095f;
      float mx = 0.0005f;
      float fx = 184.0f;

      // ----------------------------------------------
      // Tile orientation
      // ----------------------------------------------

      float rnd = fract(hs * 37.32f);

      if (rnd > 0.5f)
        pX = -pX;

      if (rnd > 0.75f || rnd < 0.25f)
        pY = -pY;

      // ----------------------------------------------
      // Tile color
      // ----------------------------------------------

      rnd = fract(hs * 32.27f);

      //float hue = fract((hs * 1.23f) + rnd) * 360.0f;
   
      uint16_t tileColor;


      if (rnd > 0.65f) {

        int colorIndex = int(hs * 4.0f);
        uint16_t colors[] = {
            colorMain,
            colorSecond,
            colorThird,
            colorFourth
        };

        tileColor = colors[colorIndex];

      } else {

        tileColor = color565(255, 255, 255);
      }

      // ----------------------------------------------
      // Base color
      // ----------------------------------------------

      float base = 0.075f;

      float d = pX + pY;
      float f = pX - pY;

      // ----------------------------------------------
      // Main gradient
      // ----------------------------------------------

      float t;

      if (fract(hs * 32.27f) > 0.65f) {

        t = (f + 1.15f) * 1.5f;

      } else {

        t = (pY + 0.15f) * 1.75f;
      }

      t = constrain(t, 0.0f, 1.0f);

      // C starts as dark gray
      float cr = base;
      float cg = base;
      float cb = base;

      // Mix dark background with white
      float k = t;

      cr = base + (1.0f - base) * k;
      cg = base + (1.0f - base) * k;
      cb = base + (1.0f - base) * k;

      // ----------------------------------------------
      // Diagonal shadow
      //
      // GLSL:
      //
      // C = mix(C,k,smoothstep(px,-px,f));
      //
      // ----------------------------------------------

      float edgeF = 1.0f - smoothstep(
        -px,
        px,
        f
      );

      cr *= (1.0f - edgeF);
      cg *= (1.0f - edgeF);
      cb *= (1.0f - edgeF);

      // ----------------------------------------------
      // Color triangle
      // ----------------------------------------------

      float edgeD = 1.0f - smoothstep(
        -px,
        px,
        d
      );

      if (edgeD > 0.0f) {

        uint16_t rgb = tileColor;

        // Decode RGB565 approximately
        float rr = ((rgb >> 11) & 0x1F) / 31.0f;
        float gg = ((rgb >> 5) & 0x3F) / 63.0f;
        float bb = (rgb & 0x1F) / 31.0f;

        cr = cr * (1.0f - edgeD) + rr * edgeD;
        cg = cg * (1.0f - edgeD) + gg * edgeD;
        cb = cb * (1.0f - edgeD) + bb * edgeD;
      }

      buffer[y * WIDTH + x] = color565(
        (uint8_t)(cr * 255.0f),
        (uint8_t)(cg * 255.0f),
        (uint8_t)(cb * 255.0f)
      );
    }
  }
}

// --------------------------------------------------
// Tile Size Random
// --------------------------------------------------


void chooseTileSize() {

  const int sizes[] = {
    60,
    60,
    120,
    240
  };

  int count = sizeof(sizes) / sizeof(sizes[0]);

  tileSize = sizes[random(0, count)];

  tilesX = WIDTH / tileSize;
  tilesY = HEIGHT / tileSize;
}

// --------------------------------------------------
// Tile Palette Type
// --------------------------------------------------



float wrapHue(float h) {
    while (h >= 360.0f) h -= 360.0f;
    while (h < 0.0f) h += 360.0f;
    return h;
}

void choosePaletteType() {

    const ColorTheme themes[] = {
        THEME_RANDOM,
        THEME_MONOCHROMATIC,
        THEME_COMPLEMENTARY,
        THEME_ANALOGOUS,
        THEME_TRIADIC,
        THEME_TETRADIC,
        THEME_WARM,
        THEME_SPLIT_COMPLEMENTARY
    };

    int count = sizeof(themes) / sizeof(themes[0]);

    colorTheme = themes[random(0, count)];

    float h;

    if (random(0, 100) < 40) {
        // 40% chance of warm base hue
        h = random(0, 61);
    } else {
        // 60% chance of any hue
        h = random(0, 360);
    }

    float h2 = h;
    float h3 = h;
    float h4 = h;

    switch (colorTheme) {

        case THEME_RANDOM:

            h2 = random(0, 360);
            h3 = random(0, 360);
            h4 = random(0, 360);

            colorMain   = hsv565(h,  1.0f, 0.95f);
            colorSecond = hsv565(h2, 0.9f, 0.90f);
            colorThird  = hsv565(h3, 0.9f, 0.90f);
            colorFourth = hsv565(h4, 0.8f, 0.85f);

            break;


        case THEME_MONOCHROMATIC:

            // Same hue, different saturation/value

            colorMain   = hsv565(h, 1.00f, 0.95f);
            colorSecond = hsv565(h, 0.85f, 0.75f);
            colorThird  = hsv565(h, 0.60f, 0.95f);
            colorFourth = hsv565(h, 0.95f, 0.45f);

            break;


        case THEME_COMPLEMENTARY:

            // Opposite hues on the color wheel

            h2 = wrapHue(h + 180);
            h3 = wrapHue(h + 15);
            h4 = wrapHue(h2 + 15);

            colorMain   = hsv565(h,  1.0f, 0.95f);
            colorSecond = hsv565(h2, 1.0f, 0.95f);
            colorThird  = hsv565(h3, 0.7f, 0.80f);
            colorFourth = hsv565(h4, 0.7f, 0.80f);

            break;


        case THEME_ANALOGOUS:

            // Neighboring hues

            h2 = wrapHue(h + 30);
            h3 = wrapHue(h + 60);
            h4 = wrapHue(h - 30);

            colorMain   = hsv565(h,  1.0f, 0.95f);
            colorSecond = hsv565(h2, 0.9f, 0.95f);
            colorThird  = hsv565(h3, 0.8f, 0.85f);
            colorFourth = hsv565(h4, 0.9f, 0.90f);

            break;


        case THEME_TRIADIC:

            // Three colors spaced 120 degrees apart

            h2 = wrapHue(h + 120);
            h3 = wrapHue(h + 240);
            h4 = wrapHue(h + 20);

            colorMain   = hsv565(h,  1.0f, 0.95f);
            colorSecond = hsv565(h2, 0.9f, 0.90f);
            colorThird  = hsv565(h3, 0.9f, 0.90f);
            colorFourth = hsv565(h4, 0.7f, 0.80f);

            break;


        case THEME_TETRADIC:

            // Four hues forming two complementary pairs

            h2 = wrapHue(h + 60);
            h3 = wrapHue(h + 180);
            h4 = wrapHue(h + 240);

            colorMain   = hsv565(h,  1.0f, 0.95f);
            colorSecond = hsv565(h2, 0.9f, 0.90f);
            colorThird  = hsv565(h3, 0.9f, 0.90f);
            colorFourth = hsv565(h4, 0.8f, 0.85f);

            break;


        case THEME_WARM:

            // Restrict hue to warm spectrum
            // Red -> Orange -> Yellow

            h  = random(0, 61);

            h2 = wrapHue(h + random(10, 31));
            h3 = wrapHue(h + random(30, 61));
            h4 = wrapHue(h - random(10, 31));

            colorMain   = hsv565(h,  1.0f, 0.95f);
            colorSecond = hsv565(h2, 1.0f, 0.95f);
            colorThird  = hsv565(h3, 0.9f, 0.95f);
            colorFourth = hsv565(h4, 0.8f, 0.85f);

            break;


        case THEME_SPLIT_COMPLEMENTARY:

            // Base hue + hues adjacent to its opposite

            h2 = wrapHue(h + 150);
            h3 = wrapHue(h + 210);
            h4 = wrapHue(h + 20);

            colorMain   = hsv565(h,  1.0f, 0.95f);
            colorSecond = hsv565(h2, 0.9f, 0.90f);
            colorThird  = hsv565(h3, 0.9f, 0.90f);
            colorFourth = hsv565(h4, 0.7f, 0.85f);

            break;
    }
}



// --------------------------------------------------
// Activate Tiles
// --------------------------------------------------

void acitvateTiles() {
    seed = random(0, 100000) / 100.0f;
    mode = random(0,3);

    chooseTileSize();
    choosePaletteType();

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        buffer[i] = color565(19, 19, 19);
    }

    drawTilePattern();

    tft.drawRGBBitmap(
    0,
    0,
    buffer,
    WIDTH,
    HEIGHT
    );
}

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {

  pinMode(13, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);

  tft.init(240, 240);
  tft.fillScreen(ST77XX_BLACK);

  // Clear framebuffer
  memset(buffer, 0, sizeof(buffer));

  // Clear physical display
  tft.fillScreen(0x0000);

    // Connect to Wi-Fi
  Serial.begin(115200);

  delay(2000);

char message[64];

    snprintf(
        message,
        sizeof(message),
        "Connecting to %s",
        ssid
    );

    drawText(
        message,
        ST77XX_ORANGE,
        5
    );

  WiFi.begin(ssid, password);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500);

      Serial.print(".");
      Serial.print(" status=");
      Serial.println(WiFi.status());

      attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    drawText(
        "Connected!",
        ST77XX_GREEN,
        25
    );

    String ipMessage =
        "IP address: " +
        WiFi.localIP().toString();

    drawText(
        ipMessage.c_str(),
        ST77XX_GREEN,
        50
    );

  delay(2000);

  } else {
    drawText(
        "FAILED!",
        ST77XX_RED,
        25
      );

      String errorMessage =
        "WiFi status: " +
        String(WiFi.status());

      drawText(
        errorMessage.c_str(),
        ST77XX_RED,
        50
    );

    delay(2000);
    
  }

  randomSeed(
    analogRead(A0) +
    analogRead(A1) +
    micros()
  );

  acitvateTiles();

}

void sendImageToMac() {

  const int TOTAL_PIXELS = 240 * 240;
  const int IMAGE_SIZE = TOTAL_PIXELS * 2;

  Serial.println("Connecting to Mac...");

  if (!client.connect(serverIP, serverPort)) {
    Serial.println("Connection failed!");
    return;
  }

  Serial.println("Connected!");

  // HTTP headers
  client.println("POST /tile HTTP/1.1");
  client.print("Host: ");
  client.println(serverIP);
  client.println("Content-Type: application/octet-stream");
  client.print("Content-Length: ");
  client.println(IMAGE_SIZE);
  client.println("Connection: close");
  client.println();

  // --------------------------------------------------
  // Send RGB565 framebuffer in chunks
  // --------------------------------------------------

  const int CHUNK_PIXELS = 256;
  uint8_t sendBuffer[CHUNK_PIXELS * 2];

  int totalSent = 0;

  for (int start = 0; start < TOTAL_PIXELS; start += CHUNK_PIXELS) {

    int count = min(CHUNK_PIXELS, TOTAL_PIXELS - start);

    for (int i = 0; i < count; i++) {

      uint16_t pixel = buffer[start + i];

      // Server expects RGB565 big-endian:
      // high byte first, low byte second
      sendBuffer[i * 2]     = (pixel >> 8) & 0xFF;
      sendBuffer[i * 2 + 1] = pixel & 0xFF;
    }

    int bytesThisChunk = count * 2;

    size_t written = client.write(
      sendBuffer,
      bytesThisChunk
    );

    totalSent += written;

    // If the WiFi client couldn't accept the whole chunk,
    // stop rather than silently sending a truncated image.
    if (written != bytesThisChunk) {
      Serial.println("ERROR: Incomplete client.write()");
      Serial.print("Expected this chunk: ");
      Serial.println(bytesThisChunk);
      Serial.print("Actually written: ");
      Serial.println(written);
      Serial.print("Total bytes sent: ");
      Serial.println(totalSent);

      client.stop();
      return;
    }
  }

  // Make sure all outgoing data has been handed to the network stack.
  client.flush();

  Serial.println("Image sent!");
  Serial.print("Expected bytes: ");
  Serial.println(IMAGE_SIZE);
  Serial.print("Bytes written: ");
  Serial.println(totalSent);

  // --------------------------------------------------
  // Wait for server response
  // --------------------------------------------------

  unsigned long timeout = millis();

  while (client.connected() && millis() - timeout < 3000) {

    while (client.available()) {
      char c = client.read();
      Serial.write(c);
      timeout = millis();
    }
  }

  client.stop();

  Serial.println();
  Serial.println("Disconnected.");
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

  int buttonStateA = digitalRead(13); 
  if (buttonStateA == LOW) {
          acitvateTiles();
          delay(200);
  }

  int buttonStateB = digitalRead(12); 
  if (buttonStateB == LOW) {
          sendImageToMac();
          delay(200);
  }
  
  int buttonStateC = digitalRead(11); 
  if (buttonStateC == LOW) {
          choosePaletteType();

            drawTilePattern();

            tft.drawRGBBitmap(
            0,
            0,
            buffer,
            WIDTH,
            HEIGHT
            );

          delay(200);
  }



  delay(50);
}
