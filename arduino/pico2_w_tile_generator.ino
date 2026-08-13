#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

const char* ssid = "GingerAir";
const char* password = "A1abammaH1ck0ri";

// IP 
const char* serverIP = "192.168.1.228";
const int serverPort = 8080;

WiFiClient client;

Adafruit_GC9A01A tft(&SPI, 16, 17, 20);


int width = 240;
int height = 240;


float t = 0;
int range = 14;                     
int rhlf  = range/2;
int h = height;
int w = width;
int hh = h / range;
int ww = w / range;
int ow = (w-(ww*range))/2;
int oh = (h-(hh*range))/2;

float rra;
float rrb;
float rea;
float reb;
float rwa;
float rwb;
float bs;

uint16_t color1;
uint16_t color2;
uint16_t color3;
uint16_t color4;
uint16_t bgcolor;
uint16_t buffer[240 * 240];

void setup() {
  randomSeed(analogRead(A1));
  SPI.setSCK(18);
  SPI.setTX(19);
  SPI.begin();

  tft.begin();
  tft.setRotation(0);

  // Clear framebuffer
  memset(buffer, 0, sizeof(buffer));

  // Clear physical display
  tft.fillScreen(0x0000);

    // Connect to Wi-Fi
  Serial.begin(115200);

  delay(2000);

  Serial.println();
  Serial.println("Starting Wi-Fi...");
  Serial.print("SSID: ");
  Serial.println(ssid);

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
      Serial.println("Wi-Fi connected!");
      Serial.print("Pico IP: ");
      Serial.println(WiFi.localIP());
  } else {
      Serial.println("Wi-Fi FAILED");
      Serial.print("Final status: ");
      Serial.println(WiFi.status());
  }
}


uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           (b >> 3);
}

uint16_t hsv565(float h, float s, float v) {
    float r, g, b;

    int i = int(h / 60.0f) % 6;
    float f = h / 60.0f - i;

    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch(i)
    {
        case 0: r=v; g=t; b=p; break;
        case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break;
        case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break;
        default:r=v; g=p; b=q; break;
    }

    return color565(
        uint8_t(r * 255),
        uint8_t(g * 255),
        uint8_t(b * 255)
    );
}

void sendImageToMac() {

  const int imageSize = 240 * 240 * 2;

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
  client.println(imageSize);
  client.println("Connection: close");
  client.println();

  // Send RGB565 framebuffer
  for (int i = 0; i < 240 * 240; i++) {

    uint16_t pixel = buffer[i];

    // Send high byte first
    client.write((pixel >> 8) & 0xFF);

    // Send low byte
    client.write(pixel & 0xFF);
  }

  Serial.println("Image sent!");

  // Wait for server response
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

void resetGrid(){

    do {
      range = random(20, 81);
    } while (240 % range != 0);
    
  rhlf  = range/2;
  hh = h / range;
  ww = w / range;
  ow = (w-(ww*range))/2;
  oh = (h-(hh*range))/2;

  bs = 1;
  rrb = random(2,5);
  rra = rrb+1;

  reb = random(6,9);
  rea = reb+random(1,2);

  rwb = random(10,14);
  rwa = rwb+random(1,3);

  if(range>39){
    rra = rra*2;
    rrb = rrb*2;
    rea = rea*2;
    reb = reb*2;
    rwa = rwa*2;
    rwb = rwa*2;
  }
  
  color1 = hsv565(random(360), 1.0, 1.0);
  color2 = hsv565(random(360), 1.0, 1.0);
  color3 = hsv565(random(360), 1.0, 1.0);
  color4 = hsv565(random(360), 1.0, 1.0);
  bgcolor = hsv565(random(360), 1.0, .25);
}

void loop() {

  resetGrid();

  tft.fillScreen(bgcolor);

  drawMap();

  sendImageToMac();

  delay(200000);
}

void drawMap() {
  for(int x = 0; x < ww; x++){
    for(int y = 0; y < hh; y++){
      float check = random(1,5);
      drawTile(x, y, check);
    }
  }
  tft.drawRGBBitmap(0,0,buffer,240,240);
}

double dist(double a, double b, double c, double d) {
  return sqrt(double((a - c) * (a - c) + (b - d) * (b - d)));
}

void drawTile(int x, int y, int check) {
  for(int dx = 0; dx < range; dx++){ 
    for(int dy = 0; dy < range; dy++){

      uint16_t flop = color565(0,0,0);

      if(check < 4) {

      float d = check > 2 ? dist(0,0,dx,range-dy) :           dist(dx,dy,range,range) ;
      float f = check > 2 ? dist(range,range,dx,range-dy) :   dist(dx,dy,0,0);

      bool checkArc1 = d > rhlf-bs && d < rhlf+bs || f > rhlf-bs && f < rhlf+bs;
      bool checkArc2 = d > rhlf-rra && d < rhlf-rrb || f > rhlf-rra && f < rhlf-rrb ||
                       d < rhlf+rra && d > rhlf+rrb || f < rhlf+rra && f > rhlf+rrb;
      bool checkArc3 = d > rhlf-rea && d < rhlf-reb || f > rhlf-rea && f < rhlf-reb ||
                       d < rhlf+rea && d > rhlf+reb || f < rhlf+rea && f > rhlf+reb;
      bool checkArc4 = d > rhlf-rwa && d < rhlf-rwb || f > rhlf-rwa && f < rhlf-rwb ||
                       d < rhlf+rwa && d > rhlf+rwb || f < rhlf+rwa && f > rhlf+rwb;  
                         

      if ( checkArc1 ) flop = color1;
      if ( checkArc2) flop = color2;
      if ( checkArc3 ) flop = color3;
      if ( checkArc4 && range > 25) flop = color4;
      
      } else {

        bool cross1 = abs(dx - rhlf) < bs || abs(dy - rhlf) < bs;
        bool check2 = (abs(dx-rhlf) > rrb-1 && abs(dx-rhlf) < rra) || (abs(dy-rhlf) > rrb-1 && abs(dy-rhlf) < rra);
        bool check3 = (abs(dx-rhlf) > reb-1 && abs(dx-rhlf) < rea) || (abs(dy-rhlf) > reb-1 && abs(dy-rhlf) < rea);
        bool check4 = (abs(dx-rhlf) > rwb-1 && abs(dx-rhlf) < rwa) || (abs(dy-rhlf) > rwb-1 && abs(dy-rhlf) < rwa);

          if ( cross1 ) flop = color1;
          if ( check2 ) flop = color2;
          if ( check3 ) flop = color3;
          if ( check4 && range > 25) flop = color4;
      } 

      int px = ow + x * range + dx;
      int py = oh + y * range + dy;

      if (flop > 0) {
        buffer[py * width + px] = flop;
      } else {
        buffer[py * width + px] = bgcolor;
      }

    }
  }
}
