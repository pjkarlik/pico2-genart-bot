
// Device Specific Drivers - Waveshare LEDMatrix RP2350
#include "DEV_Config.h"
#include "WS2812.h"

// Animations for LEDMatrix
#include "Animations.h"

#define HEADER_1 0xAA
#define HEADER_2 0x55

#define MSG_STATUS 0x01
#define MSG_RESET  0x07

#define STATUS_IDLE      0x00
#define STATUS_RUN       0x01
#define STATUS_TWIST     0x02

// Declaration for an SSD1306 display conne
void setup() {
    Serial.begin(115200);
    if (DEV_Module_Init() != 0)
        Serial.println("GPIO Init Fail!");
    else
        Serial.println("GPIO Init successful!");
    WS2812_init();
    led_ctrl.brightness = 5;
}

struct Pixel {
    float r;
    float g;
    float b;

    Pixel() : r(0), g(0), b(0) {}

    Pixel(float red, float green, float blue)
        : r(red), g(green), b(blue) {}
};

Pixel framebuffer[WIDTH][HEIGHT];

uint8_t currentStatus = STATUS_RUN;
static uint8_t lastStatus = 255;

// --------------------------------------------------
// Color Palette
// --------------------------------------------------

Pixel colorPalette[16] = {
        Pixel(0, 0, 0),
        Pixel(10,10,10),
        Pixel(9, 1, 1),
        Pixel(10, 0, 0),
        Pixel(9, 2, 0),
        Pixel(7, 1, 0),
        Pixel(9, 9, 0),
        Pixel(2, 8, 0),
        Pixel(0, 10, 0),
        Pixel(0, 8, 10),
        Pixel(0, 1, 10),
        Pixel(0, 0, 6),
        Pixel(7, 0, 10),
        Pixel(8, 0, 8),
        Pixel(1,1,1),
        Pixel(2,2,2),
};
// --------------------------------------------------
// Read Serial
// --------------------------------------------------

void readSerial()
{
    static enum {
        WAIT_HEADER_1,
        WAIT_HEADER_2,
        READ_TYPE,
        READ_LENGTH,
        READ_DATA
    } state = WAIT_HEADER_1;

    static uint8_t messageType = 0;
    static uint8_t messageLength = 0;
    static uint8_t messageData[32];
    static uint8_t index = 0;

    while (Serial.available())
    {
        uint8_t data = Serial.read();

        switch (state)
        {
            case WAIT_HEADER_1:

                if (data == HEADER_1)
                    state = WAIT_HEADER_2;

                break;


            case WAIT_HEADER_2:

                if (data == HEADER_2)
                {
                    state = READ_TYPE;
                }
                else
                {
                    state = WAIT_HEADER_1;
                }

                break;


            case READ_TYPE:

                messageType = data;
                state = READ_LENGTH;

                break;


            case READ_LENGTH:

                messageLength = data;
                index = 0;

                if (messageLength == 0)
                {
                    handleMessage(messageType, messageLength, messageData);
                    state = WAIT_HEADER_1;
                }
                else if (messageLength <= sizeof(messageData))
                {
                    state = READ_DATA;
                }
                else
                {
                    // Invalid packet
                    state = WAIT_HEADER_1;
                }

                break;


            case READ_DATA:

                messageData[index++] = data;

                if (index >= messageLength)
                {
                    handleMessage(
                        messageType,
                        messageLength,
                        messageData
                    );

                    state = WAIT_HEADER_1;
                }

                break;
        }
    }
}

// --------------------------------------------------
// Handle Message
// --------------------------------------------------

void handleMessage(
    uint8_t type,
    uint8_t length,
    uint8_t* data
)
{
    if (type == MSG_RESET)
    {
        lastStatus = 255;

        Serial.println("STATUS RESET");
        return;
    }

    if (type == MSG_STATUS && length >= 1)
    {
        currentStatus = data[0];

        Serial.print("STATUS: ");
        Serial.println(currentStatus);
    }
}

// --------------------------------------------------
// Draw Frame
// --------------------------------------------------

void drawFrame(const uint16_t frame[8][8])
{
    WS2812_clear();

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {

            int rotatedX = 7 - x; 
            int rotatedY = 7 - y; 
            int index = frame[rotatedX][rotatedY];

            WS2812_set_pixel(
                x,
                y,
                colorPalette[index].r,
                colorPalette[index].g,
                colorPalette[index].b
            );
        }
    }

    WS2812_show();
}

// --------------------------------------------------
// Animation State
// --------------------------------------------------

const unsigned long FRAME_DELAY = 80;

uint8_t animationFrame = 0;
unsigned long lastFrameTime = 0;

// --------------------------------------------------
// Draw Status Animation
// --------------------------------------------------

void updateAnimation()
{
    unsigned long now = millis();

    // Status changed — restart animation
    if (currentStatus != lastStatus)
    {
        lastStatus = currentStatus;
        animationFrame = 0;
        lastFrameTime = now;

        // Draw first frame immediately
        switch (currentStatus)
        {
            case STATUS_IDLE:
                drawFrame(idleFrames[animationFrame]);
                break;

            case STATUS_RUN:
                drawFrame(runFrames[animationFrame]);
                break;

            case STATUS_TWIST:
                drawFrame(runFrames[animationFrame]);
                break;
        }

        return;
    }

    // Not time for the next frame yet
    if (now - lastFrameTime < FRAME_DELAY)
        return;

    lastFrameTime = now;

    switch (currentStatus)
    {
        case STATUS_IDLE:
        {
            const int frameCount = 8;

            animationFrame++;

            if (animationFrame >= frameCount)
                animationFrame = 0;

            drawFrame(idleFrames[animationFrame]);

            break;
        }

        case STATUS_RUN:
        {
            const int frameCount = 5;

            animationFrame++;

            if (animationFrame >= frameCount)
                animationFrame = 0;

            drawFrame(runFrames[animationFrame]);

            break;
        }

        case STATUS_TWIST:
        {
            const int frameCount = 4;

            animationFrame++;

            if (animationFrame >= frameCount)
                animationFrame = 0;

            drawFrame(twistFrames[animationFrame]);

            break;
        }

    }
}


// --------------------------------------------------
// Main Loop
// --------------------------------------------------

void loop()
{
    readSerial();
    updateAnimation();
}


