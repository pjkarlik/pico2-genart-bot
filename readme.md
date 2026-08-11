# Pico 2 W Random Art Bot

A small autonomous generative-art project using a **Raspberry Pi Pico 2 W** to create procedural artwork, upload the resulting images to a local web gallery, and automatically post them to **Bluesky**.

<img src="./splash01.jpg" width=640px>
The goal is simple: let the hardware continuously create random artwork and occasionally share it with the world.

## How It Works

```text
┌──────────────────┐
│   Raspberry Pi   │
│      Pico 2 W    │
│                  │
│  Generate Art    │
└────────┬─────────┘
         │
         │ WiFi
         ▼
┌──────────────────┐
│  Local Mac/Web   │
│     Gallery      │
│                  │
│  tile_XXXX.png   │
└────────┬─────────┘
         │
         │ Python Bot
         ▼
┌──────────────────┐
│     Bluesky      │
│                  │
│  Random Artwork  │
│  + Caption       │
└──────────────────┘
```

## Pico 2 W

The artwork is generated directly on a **Raspberry Pi Pico 2 W**.

The Pico creates the procedural/pixel artwork and sends the resulting PNG images over WiFi. The images are stored using sequential filenames such as:

```text
tile_0001.png
tile_0002.png
tile_0003.png
...
```

The Pico is responsible for generating the artwork; the social-media posting is handled separately so the microcontroller can concentrate on rendering.

## Gallery

The generated artwork is collected into a simple static image gallery on the Mac.

For example:

```text
images/
├── index.html
├── tile_0001.png
├── tile_0002.png
├── tile_0003.png
└── ...
```

The gallery can be served locally during development or published to a static hosting service.

The hosting location is intentionally independent from the bot.

## Bluesky Bot

A small Python bot watches the local gallery directory for artwork that hasn't been posted yet.

When new artwork is available, the bot:

1. Finds unposted images.
2. Selects one randomly.
3. Creates a larger social-media version.
4. Uploads the image to Bluesky.
5. Adds a generated caption and hashtags.
6. Records the image as posted.
7. Waits before posting another image.

The bot currently uses a randomized delay of approximately **45–90 minutes** between posts.

This keeps the account feeling more like an autonomous art bot rather than a scheduled feed.

## Pixel Art Scaling

The original artwork is kept at its native resolution.

For Bluesky, the bot creates a **960×960** version using **nearest-neighbor scaling**.

For the original 240×240 artwork:

```text
240 × 240
    │
    │ 4× nearest-neighbor
    ▼
960 × 960
```

Nearest-neighbor scaling is important because it preserves the hard edges of the original pixels instead of introducing blurry interpolated pixels.

The original image remains unchanged in the gallery.

## Bot Structure

```text
bot/
├── bot.py
├── config.py
├── requirements.txt
├── .env
├── .gitignore
│
├── data/
│   └── posted.json
│
└── logs/
    └── bot.log
```

### `bot.py`

Handles the main bot loop, image selection, image preparation, and Bluesky posting.

### `config.py`

Contains configuration such as:

* Gallery location
* Posting interval
* Bluesky service
* Image scaling
* Site URL

### `posted.json`

Keeps track of artwork that has already been posted so restarting the bot doesn't result in duplicate posts.

### `.env`

Stores Bluesky credentials and should **never be committed to GitHub**.

Example:

```text
BLUESKY_HANDLE=yourbot.bsky.social
BLUESKY_APP_PASSWORD=xxxx-xxxx-xxxx-xxxx
```

## Running the Bot

Create and activate the Python virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
```

Install dependencies:

```bash
pip install -r requirements.txt
```

Then start the bot:

```bash
python3 bot.py
```

The bot will log in to Bluesky, find available artwork, make its first post, and then wait before selecting another image.

Stop it with:

```text
Ctrl+C
```

## The Idea

The project is intentionally split into three independent pieces:

**Generate → Publish → Share**

The Pico 2 W handles the interesting part — creating the art.

The Mac handles the gallery and automation.

Bluesky provides the public canvas for the resulting stream of random generative artwork.

The result is a little machine that can continuously make and share things without requiring much interaction once it's running.

---

### Built With

* Raspberry Pi Pico 2 W
* WiFi
* Python
* Pillow
* Bluesky / AT Protocol
* PNG
* Procedural / generative graphics
* Static HTML gallery
