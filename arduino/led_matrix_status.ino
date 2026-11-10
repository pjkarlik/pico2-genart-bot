// Waveshare RP2350 LED Matrix Display
#include "DEV_Config.h"
#include "WS2812.h"
#include "Animations.h"

#define HEADER_1 0xAA
#define HEADER_2 0x55

#define MSG_STATUS 0x01
#define MSG_RESET  0x07

#define STATUS_IDLE      0x00
#define STATUS_RECEIVED  0x01
#define STATUS_SAVED     0x02
#define STATUS_POSTING   0x03
#define STATUS_POSTED    0x04
#define STATUS_SLEEPING  0x05
#define STATUS_ERROR     0x06

#define STATUS_QUEUE_SIZE 16

uint8_t statusQueue[STATUS_QUEUE_SIZE];
uint8_t queueHead = 0;
uint8_t queueTail = 0;
uint8_t queueCount = 0;

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

uint8_t currentStatus = STATUS_IDLE;
static uint8_t lastStatus = 255;

// --------------------------------------------------
// Enqueue Helpers
// --------------------------------------------------


bool enqueueStatus(uint8_t status)
{
    if (queueCount >= STATUS_QUEUE_SIZE)
    {
        // Queue full
        Serial.println("STATUS QUEUE FULL");
        return false;
    }

    statusQueue[queueTail] = status;
    queueTail = (queueTail + 1) % STATUS_QUEUE_SIZE;
    queueCount++;

    return true;
}


bool dequeueStatus(uint8_t &status)
{
    if (queueCount == 0)
        return false;

    status = statusQueue[queueHead];
    queueHead = (queueHead + 1) % STATUS_QUEUE_SIZE;
    queueCount--;

    return true;
}

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
        uint8_t status = data[0];

        Serial.print("QUEUE STATUS: ");
        Serial.println(status);

        enqueueStatus(status);
    }
}

// --------------------------------------------------
// Draw Frame
// --------------------------------------------------

void drawFrame(const uint16_t frame[6][8])
{
    WS2812_clear();

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {

            int rotatedX = 7 - x; 
            int rotatedY = 7 - y; 
            int index = frame[rotatedX][rotatedY];
            //int index = frame[y][x];

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
// Draw Status
// --------------------------------------------------

void drawIdle()
{
    int frameIndex = 0;
    int amt = 10;
    int loop = amt * 2;
    for(int i=0;i<loop;i++){
    frameIndex = i % amt;
    drawFrame(idleFrames[frameIndex]);
    delay(90);
    }
}

void drawReceived()
{
    int frameIndex = 0;
    int amt = 12;
    int loop = amt * 2;
    for(int i=0;i<loop;i++){
    frameIndex = i % amt;
    drawFrame(recieveFrames[frameIndex]);
    delay(90);
    }
}


void drawSaved()
{
    int frameIndex = 0;
    int amt = 9;
    int loop = amt * 2;
    for(int i=0;i<loop;i++){
    frameIndex = i % amt;
    drawFrame(savedFrames[frameIndex]);
    delay(90);
    }
}


void drawPosting()
{
    int frameIndex = 0;
    int amt = 11;
    int loop = amt * 2;
    for(int i=0;i<loop;i++){
    frameIndex = i % amt;
    drawFrame(postingFrames[frameIndex]);
    delay(90);
    }
}


void drawPosted()
{
    int frameIndex = 0;
    int amt = 11;
    int loop = amt * 2;
    for(int i=0;i<loop;i++){
    frameIndex = i % amt;
    drawFrame(postedFrames[frameIndex]);
    delay(90);
    }
}


void drawSleeping()
{
    int frameIndex = 0;
    int amt = 16;
    int loop = amt * 4;
    for(int i=0;i<loop;i++){
    frameIndex = i % amt;
    drawFrame(sleepingFrames[frameIndex]);
    delay(90);
    }
}


void drawError()
{
    int frameIndex = 0;
    int amt = 10;
    int loop = amt * 2;
    for(int i=0;i<loop;i++){
    frameIndex = i % amt;
    drawFrame(errorFrames[frameIndex]);
    delay(90);
    }
}

// --------------------------------------------------
// Main Loop
// --------------------------------------------------

void loop()
{
    // Pick up anything waiting on serial
    readSerial();

    // If there is a queued status, play it
    uint8_t nextStatus;

    if (dequeueStatus(nextStatus))
    {
        currentStatus = nextStatus;
        lastStatus = currentStatus;

        switch (currentStatus)
        {
            case STATUS_IDLE:
                drawIdle();
                break;

            case STATUS_RECEIVED:
                drawReceived();
                break;

            case STATUS_SAVED:
                drawSaved();
                break;

            case STATUS_POSTING:
                drawPosting();
                break;

            case STATUS_POSTED:
                drawPosted();
                break;

            case STATUS_SLEEPING:
                drawSleeping();
                break;

            case STATUS_ERROR:
                drawError();
                break;
        }
    }
}

