# Pico 2 W Random Art Bot

A small autonomous generative-art system built around a **Raspberry Pi Pico 2 W**.

<img src="./splash03.jpg" width="320px">

The Pico continuously creates procedural pixel artwork and sends it over WiFi to a Python server. The server receives and stores the artwork, while a dedicated **8×8 status LED matrix** provides a physical indication of what the system is doing.

A separate Python bot watches for new artwork, prepares it for Bluesky, posts it, and reports its progress back to the status display.

## Parts list

* [Pico 2 W Microcontroller - Amazon.com](https://a.co/d/0bXTyKj8)
* [TFT LCD GC9A01 Driver - Amazon.com](https://a.co/d/04De4shb)
* [Waveshare 8×8 LED Matrix - Amazon.com](https://www.amazon.com/dp/B0FP888B1W)

The complete flow is:

**Generate → Receive → Save → Post → Sleep → Repeat**

---

## How It Works

```text
                    ┌──────────────────────┐
                    │    Raspberry Pi      │
                    │       Pico 2 W       │
                    │                      │
                    │   Generate Artwork   │
                    │      240 × 240       │
                    └──────────┬───────────┘
                               │
                               │ WiFi
                               ▼
                    ┌──────────────────────┐
                    │    Python Server     │
                    │      server.py       │
                    │                      │
                    │  Receive RGB565      │
                    │  Convert to PNG      │
                    │  Save Artwork        │
                    │  Image API           │
                    │  Status Controller   │
                    └───────┬───────┬──────┘
                            │       │
                   Images   │       │ Serial
                            │       ▼
                            │  ┌──────────────────┐
                            │  │   Status LED     │
                            │  │      Matrix      │
                            │  │                  │
                            │  │  Current State   │
                            │  │  System Metrics  │
                            │  └──────────────────┘
                            │
                            ▼
                    ┌──────────────────────┐
                    │     Python Bot       │
                    │       bot.py         │
                    │                      │
                    │  Find New Artwork    │
                    │  Select Image        │
                    │  Prepare 960×960     │
                    │  Generate Caption    │
                    │  Post to Bluesky     │
                    │  Track Posted Images │
                    │  Sleep / Countdown   │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │       Bluesky        │
                    │                      │
                    │   Random Artwork     │
                    │   + Caption          │
                    └──────────────────────┘
```

The system is intentionally divided into independent components:

* **Pico 2 W** — generates the artwork
* **Python server** — receives and stores artwork and provides the API
* **Status LED Matrix** — provides a physical system/status display
* **Python bot** — handles Bluesky posting and scheduling
* **Vite gallery** — provides the web interface
* **Bluesky** — publishes the resulting artwork

---

# Hardware

## Pico 2 W

The artwork is generated directly on a **Raspberry Pi Pico 2 W**.

The Pico generates the procedural/pixel artwork at:

```text
240 × 240 pixels
```

The image is transmitted as raw **RGB565** data over WiFi.

The Pico does not need to know anything about PNG files, the web gallery, or Bluesky. Its job is simply:

```text
Generate
   ↓
Render
   ↓
Send
```

This keeps the microcontroller focused on generating the artwork.

---

## Status LED Matrix

The system also includes an **8×8 RGB LED matrix** connected to the host/controller running the status firmware.

The matrix provides a physical status display for the bot.

The Python side communicates with the matrix using a small serial protocol.

Messages use the following framing:

```text
0xAA 0x55
```

followed by the message type and payload.

The status system supports states including:

```text
IDLE
RECEIVED
SAVED
POSTING
POSTED
SLEEPING
ERROR
```

A reset message is also available so the display can be explicitly restarted/reset between states.

The matrix continuously animates the current state rather than displaying a single frame and stopping.

---

# Status Flow

The status display follows the lifecycle of an artwork.

```text
             ┌─────────┐
             │  IDLE   │
             └────┬────┘
                  │
                  ▼
        ┌─────────────────┐
        │    RECEIVED     │
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │      SAVED      │
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │     POSTING     │
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │     POSTED      │
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │    SLEEPING     │
        │  countdown      │
        └────────┬────────┘
                 │
                 ▼
             ┌─────────┐
             │  IDLE   │
             └─────────┘
```

If something goes wrong, the system can enter:

```text
ERROR
```

The display checks for new serial messages while its animation is running, allowing a new status to interrupt the current animation without waiting for the animation to finish.

---

# Status LED Matrix

The project includes an **8×8 RGB LED matrix** that provides a physical status display for the otherwise headless system.

The matrix currently displays the same **animated status indicators** used throughout the bot's workflow. Each status has its own animation, and the animation continues to loop until a new status message is received.

The current status states are:

```text
IDLE
RECEIVED
SAVED
POSTING
POSTED
SLEEPING
ERROR
```

The status flow allows the physical display to show what the bot is currently doing without needing to look at the server or bot logs.

## Status Flow

```text
IDLE
  │
  ▼
RECEIVED
  │
  ▼
SAVED
  │
  ▼
POSTING
  │
  ▼
POSTED
  │
  ▼
SLEEPING
  │
  └──────────────► IDLE
```

If an error occurs, the display can switch to:

```text
ERROR
```

## Animated Statuses

The Arduino LED-matrix firmware maintains the **last received status** and continuously runs its associated animation.

This means the matrix does not simply display a status once and stop. Instead:

```text
Receive status
      │
      ▼
Store current status
      │
      ▼
Run status animation
      │
      │
      └─────── loop
```

While the animation is running, the firmware continues checking the serial connection for new messages. When a new status arrives, the current animation is interrupted and the new status animation begins.

This allows the server and bot to update the display immediately even when an animation is still running.

## Reset Messages

The serial protocol also supports an explicit reset message.

This is useful when the same status needs to be triggered again.

For example, the bot may already be displaying `SLEEPING`. Sending another `SLEEPING` status by itself would not necessarily restart the animation because the status has not changed.

A reset message can first clear/restart the display state:

```text
RESET
  │
  ▼
SLEEPING
```

This is particularly useful for the sleep countdown/status updates.

## Serial Protocol

Messages are framed using:

```text
0xAA 0x55
```

The protocol currently includes message types for status updates and display resets.

The status messages use the following state values:

```text
STATUS_IDLE       0x00
STATUS_RECEIVED   0x01
STATUS_SAVED      0x02
STATUS_POSTING    0x03
STATUS_POSTED     0x04
STATUS_SLEEPING   0x05
STATUS_ERROR      0x06
```

The LED matrix therefore acts as a simple physical **activity/status indicator**, showing the current state of the art-generation and posting pipeline through animation.


---

# Python Image Server

`server.py` is the central bridge between the Pico, stored artwork, the gallery, and the status display.

It:

* Receives artwork from the Pico over WiFi
* Receives raw RGB565 image data
* Converts the image to PNG
* Saves the artwork
* Provides the image API
* Serves image files to the gallery
* Communicates with the status LED matrix
* Reports image-receiving activity through the status system

The server runs on:

```text
localhost:8080
```

---

## Image API

The image list is available at:

```text
GET /api/images
```

The response is a JSON array containing image filenames.

For example:

```json
[
    "tile_0012.png",
    "tile_0011.png",
    "tile_0010.png"
]
```

Individual images are available at:

```text
GET /images/tile_0012.png
```

The gallery therefore does not need direct access to the filesystem.

---

# Image Reception Flow

When the Pico sends an image, the server processes it roughly like this:

```text
Pico
 │
 │  RGB565 image
 ▼
server.py
 │
 ├── RECEIVED
 │
 ├── Convert RGB565 → RGB
 │
 ├── Save PNG
 │
 └── SAVED
       │
       ▼
    images/
```

The server is responsible for making the artwork persistent.

The bot does not need to communicate directly with the Pico.

---

# Artwork Files

Artwork is stored using sequential filenames:

```text
tile_0001.png
tile_0002.png
tile_0003.png
...
```

The original artwork remains at its native:

```text
240 × 240
```

resolution.

This original image is what is displayed by the gallery.

---

# Bluesky Bot

`bot/bot.py` is responsible for turning the generated artwork into social-media posts.

The bot operates independently from the Pico and image server.

Its basic lifecycle is:

```text
Find new artwork
      ↓
Select artwork
      ↓
POSTING
      ↓
Prepare image
      ↓
Generate caption
      ↓
Upload to Bluesky
      ↓
POSTED
      ↓
Sleep
      ↓
Repeat
```

When the bot is ready to post, it searches for artwork that has not already been posted.

One image is selected and posted.

After a successful post, the image is recorded in the posted-image history so it won't be posted again after a restart.

---

# Posting Schedule

The bot intentionally does not post continuously.

The current randomized posting interval is approximately:

```text
420–580 minutes
```

This creates a less predictable, more autonomous posting schedule.

After posting, the bot enters the sleeping state and maintains a countdown until the next posting opportunity.

The status display can show this state as:

```text
SLEEPING
xxx minutes remaining
```

The bot periodically updates the sleep status so the physical display reflects the current countdown.

---

# Posted Image History

The bot maintains a record of artwork that has already been posted.

This prevents the same artwork from being selected again after restarting the bot.

The history is stored in:

```text
bot/data/posted.json
```

The file allows the bot to continue where it left off rather than starting over whenever the process restarts.

---

# Pixel-Art Scaling

The original artwork is generated at:

```text
240 × 240
```

For Bluesky, the bot creates a larger:

```text
960 × 960
```

version.

The scaling factor is:

```text
4×
```

using nearest-neighbor scaling.

```text
240 × 240
     │
     │ 4× nearest-neighbor
     ▼
960 × 960
```

Nearest-neighbor scaling is important because it preserves the hard edges of the generated pixels.

The original 240×240 image remains unchanged.

---

# Gallery

The gallery is a small **Vite + TypeScript + SCSS** application.

<img src =splash02.gif>

Live gallery:

**Pico 2 Tiles**

https://pico2tiles.surge.sh/

The gallery retrieves artwork through the Python server API.

Current features include:

* Responsive image grid
* 12 images per page
* Pagination
* Pixel-art friendly scaling
* Click-to-open image viewer
* Large image display
* Nearest-neighbor/pixelated rendering
* Repeating image backgrounds in the viewer

The frontend structure is approximately:

```text
src/
├── main.ts
├── gallery.ts
└── style.scss

index.html
vite.config.ts
package.json
tsconfig.json
```

---

# Development Proxy

During development, Vite runs on:

```text
localhost:5173
```

while the Python server runs on:

```text
localhost:8080
```

Vite proxies API and image requests:

```text
Browser
   │
   ▼
Vite :5173
   │
   ├── /api/*
   │
   └── /images/*
          │
          ▼
      Python :8080
```

The frontend can therefore use:

```text
/api/images
```

and:

```text
/images/tile_0012.png
```

without hard-coding the Python server address.

---

# Project Structure

The project is organized approximately like this:

```text
pico2-genart-bot/
│
├── bot/
│   ├── bot.py
│   ├── config.py
│   │
│   ├── data/
│   │   └── posted.json
│   │
│   └── logs/
│       └── bot.log
│
├── images/
│   ├── tile_0001.png
│   ├── tile_0002.png
│   └── ...
│
├── src/
│   ├── main.ts
│   ├── gallery.ts
│   └── style.scss
│
├── index.html
├── server.py
├── vite.config.ts
├── package.json
├── tsconfig.json
├── pyproject.toml
├── uv.lock
└── README.md
```

---

# Important Files

## `server.py`

The Python server is responsible for:

* Receiving artwork from the Pico
* Converting RGB565 data
* Saving PNG files
* Providing the image API
* Serving images
* Communicating with the status LED matrix

---

## `bot/bot.py`

The bot handles:

* Finding new artwork
* Selecting an image
* Preparing the Bluesky image
* Generating captions
* Posting to Bluesky
* Tracking posted artwork
* Managing the posting interval
* Reporting bot status
* Managing the sleep/countdown state

---

## `bot/config.py`

Contains project configuration including things such as:

* Image locations
* Posting interval
* Bluesky configuration
* Image scaling
* Site URL
* History limits

---

## `bot/data/posted.json`

Persistent history of artwork that has already been posted.

This prevents duplicate posts when the bot restarts.

---

## `.env`

Contains private credentials and should **never be committed to GitHub**.

Example:

```text
BLUESKY_HANDLE=yourbot.bsky.social
BLUESKY_APP_PASSWORD=xxxx-xxxx-xxxx-xxxx
```

---

# Python Environment

The Python portion uses **uv** for dependency and virtual-environment management.

## Install uv

If uv is not already installed:

```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

Verify:

```bash
uv --version
```

---

## Install Dependencies

From the project root:

```bash
uv sync
```

This creates the project's virtual environment and installs the dependencies defined by:

```text
pyproject.toml
uv.lock
```

There is no need to manually activate the virtual environment.

---

# Running the Project

After cloning the repository:

```bash
uv sync
npm install
```

The system has three main processes.

---

## Terminal 1 — Python Server

```bash
uv run python server.py
```

The server listens for artwork from the Pico and provides the HTTP API.

```text
Tile server running on port 8080
```

---

## Terminal 2 — Bluesky Bot

```bash
uv run python bot/bot.py
```

The bot watches for new artwork, posts it to Bluesky, and manages the posting/sleep cycle.

---

## Terminal 3 — Vite Gallery

```bash
npm run dev
```

The development gallery will typically be available at:

```text
http://localhost:5173
```

The three processes are independent:

```text
Terminal 1
──────────
uv run python server.py

Terminal 2
──────────
uv run python bot/bot.py

Terminal 3
──────────
npm run dev
```

The gallery and bot can be stopped without preventing the Pico from generating and sending artwork, provided the image server remains running.

Stop a process with:

```text
Ctrl+C
```

---

# Installing Node Dependencies

The gallery uses Node/npm.

```bash
npm install
```

Start the development server:

```bash
npm run dev
```

Create a production build:

```bash
npm run build
```

---

# Adding Python Dependencies

Add a dependency with:

```bash
uv add package-name
```

For example:

```bash
uv add pillow
```

Remove a dependency with:

```bash
uv remove package-name
```

`uv` updates both `pyproject.toml` and `uv.lock`.

---

# Git and Environment Files

The repository should include:

```text
pyproject.toml
uv.lock
package.json
package-lock.json
```

It should **not** include:

```text
.env
.venv/
__pycache__/
*.pyc
```

The `.env` file contains private Bluesky credentials and must never be committed.

The `.venv` directory is managed by uv and can be recreated with:

```bash
uv sync
```

---

# System Architecture

The project is intentionally split into independent responsibilities.

```text
                  GENERATE
                     │
                     ▼
              ┌─────────────┐
              │   Pico 2 W  │
              └──────┬──────┘
                     │
                     │ WiFi
                     ▼
                  RECEIVE
                     │
                     ▼
              ┌─────────────┐
              │   Server    │
              │  server.py  │
              └──────┬──────┘
                     │
             ┌───────┴────────┐
             │                │
             ▼                ▼
           STORE           STATUS
             │                │
             ▼                ▼
          images/        LED Matrix
             │
             ▼
           WATCH
             │
             ▼
              ┌─────────────┐
              │     Bot     │
              │   bot.py    │
              └──────┬──────┘
                     │
                     ▼
                   POST
                     │
                     ▼
                 Bluesky
                     │
                     ▼
                   SLEEP
                     │
                     └───────────► repeat
```

---

# The Status System

One of the goals of the project is that the machine should be understandable without needing to look at a terminal.

The status LED matrix provides a physical representation of the software state.

The important states are:

```text
IDLE
    Nothing currently happening.

RECEIVED
    A new artwork payload has arrived from the Pico.

SAVED
    The artwork has successfully been converted and saved.

POSTING
    The bot is currently preparing/uploading artwork.

POSTED
    Artwork was successfully posted.

SLEEPING
    The bot is waiting before its next post.

ERROR
    Something failed and requires attention.
```

The matrix continuously animates the current state and checks for incoming serial messages while animating.

This means a new status can interrupt the current animation immediately rather than waiting for the previous animation to finish.

---

# The Complete Lifecycle

Putting everything together, a typical artwork cycle looks like this:

```text
1. Pico generates artwork
          │
          ▼
2. Pico sends RGB565 data
          │
          ▼
3. Server receives artwork
          │
          ├── LED → RECEIVED
          │
          ▼
4. Server converts/saves PNG
          │
          ├── LED → SAVED
          │
          ▼
5. Bot discovers new artwork
          │
          ▼
6. Bot selects an unposted image
          │
          ├── LED → POSTING
          │
          ▼
7. Bot creates 960×960 version
          │
          ▼
8. Bot generates caption
          │
          ▼
9. Bot posts to Bluesky
          │
          ├── LED → POSTED
          │
          ▼
10. Image recorded in posted.json
          │
          ▼
11. Bot enters sleep
          │
          ├── LED → SLEEPING
          │
          │   countdown
          │
          ▼
12. Next posting window
          │
          ▼
        repeat
```

The Pico can continue generating artwork independently while the bot is sleeping.

That separation is intentional: **generation, storage, presentation, and publishing are independent jobs.**

---

# The Idea

The original concept was simply:

**Generate → Collect → Display → Share**

The project has evolved into something closer to a small autonomous machine:

**Generate → Receive → Save → Share → Sleep → Repeat**

The **Pico 2 W** creates the art.

The **Python server** receives and stores it.

The **status LED matrix** makes the machine's internal state visible.

The **Vite gallery** provides a public visual archive.

The **Python bot** decides when artwork gets shared.

**Bluesky** provides the public stream of generated artwork.

Once everything is running, the system can operate on its own:

```text
        ┌──────────────────────┐
        │                      │
        │    CREATE ART        │
        │         ↓            │
        │    SAVE ART          │
        │         ↓            │
        │    POST ART          │
        │         ↓            │
        │    SLEEP             │
        │         ↓            │
        │    CREATE AGAIN      │
        │                      │
        └──────────────────────┘
```

The goal is for the hardware to feel less like a computer running a script and more like a little autonomous art machine.

---

# Built With

* Raspberry Pi Pico 2 W
* WiFi
* 8×8 RGB LED Matrix
* Arduino/C++ LED firmware
* Python
* uv
* Pillow
* TypeScript
* Vite
* SCSS
* HTML / CSS
* Bluesky / AT Protocol
* PNG
* RGB565
* Procedural / generative graphics
* Nearest-neighbor pixel scaling
* Serial communication
