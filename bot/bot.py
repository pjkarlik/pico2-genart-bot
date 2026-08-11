import json
import logging
import os
import random
import time
from datetime import datetime, timezone
from pathlib import Path

from dotenv import load_dotenv
from atproto import Client
from PIL import Image

import config


# --------------------------------------------------
# Setup
# --------------------------------------------------

load_dotenv()

config.DATA_DIR.mkdir(exist_ok=True)
config.LOG_DIR.mkdir(exist_ok=True)


logging.basicConfig(
    filename=config.LOG_DIR / "bot.log",
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s"
)

console = logging.StreamHandler()
console.setLevel(logging.INFO)
logging.getLogger().addHandler(console)


# --------------------------------------------------
# Bluesky
# --------------------------------------------------

BLUESKY_HANDLE = os.getenv("BLUESKY_HANDLE")
BLUESKY_APP_PASSWORD = os.getenv("BLUESKY_APP_PASSWORD")

if not BLUESKY_HANDLE or not BLUESKY_APP_PASSWORD:
    raise RuntimeError(
        "BLUESKY_HANDLE and BLUESKY_APP_PASSWORD must be set in .env"
    )


client = Client(config.BLUESKY_SERVICE)

client.login(
    BLUESKY_HANDLE,
    BLUESKY_APP_PASSWORD
)

logging.info("Logged into Bluesky as %s", BLUESKY_HANDLE)


# --------------------------------------------------
# Posted history
# --------------------------------------------------

def load_posted():
    if not config.POSTED_FILE.exists():
        return []

    try:
        with open(config.POSTED_FILE, "r") as f:
            data = json.load(f)

        return data.get("posted", [])

    except Exception as e:
        logging.error("Could not load posted history: %s", e)
        return []


def save_posted(posted):
    with open(config.POSTED_FILE, "w") as f:
        json.dump(
            {"posted": posted},
            f,
            indent=4
        )


# --------------------------------------------------
# Find artwork
# --------------------------------------------------

def find_images():
    return sorted(
        config.IMAGE_DIR.glob("tile_*.png")
    )


def choose_image(posted):
    images = find_images()

    if not images:
        return None

    available = [
        image
        for image in images
        if image.name not in posted
    ]

    if not available:
        logging.info("All images have been posted.")
        return None

    return random.choice(available)


# --------------------------------------------------
# Image preparation
# --------------------------------------------------

SOCIAL_SCALE = 4

def prepare_image(path):

    img = Image.open(path)

    img = img.resize(
        (
            img.width * SOCIAL_SCALE,
            img.height * SOCIAL_SCALE
        ),
        Image.Resampling.NEAREST
    )

    temp_path = config.DATA_DIR / "_upload.png"

    img.save(
        temp_path,
        "PNG",
        optimize=True
    )

    return temp_path


# --------------------------------------------------
# Caption
# --------------------------------------------------

def make_caption(filename):

    number = filename.replace(
        "tile_", ""
    ).replace(
        ".png", ""
    )

    captions = [
        f"Random generative study tile:{number}",
        f"Procedural experiment tile:{number}",
        f"Another little machine-made universe tile:{number}",
        f"Generated somewhere between math and chaos tile:{number}",
        f"Random pixels tile:{number}",
    ]

    caption = random.choice(captions)

    return (
        f"{caption}\n\n"
        "#generativeart #creativecoding #avr #pico2 #digitalart"
    )


# --------------------------------------------------
# Post
# --------------------------------------------------

def post_image(path):

    logging.info("Preparing %s", path.name)

    upload_path = prepare_image(path)

    with open(upload_path, "rb") as f:
        image_data = f.read()

    if len(image_data) > config.MAX_IMAGE_SIZE:
        raise RuntimeError(
            f"{path.name} is too large for Bluesky: "
            f"{len(image_data):,} bytes"
        )

    caption = make_caption(path.name)

    alt_text = (
        f"Generative artwork created by a Raspberry Pi Pico 2 "
        f"and displayed in the Pico2Tiles project. "
        f"Artwork {path.stem.replace('tile_', '#')}."
    )

    logging.info("Posting %s", path.name)

    post = client.send_image(
        text=caption,
        image=image_data,
        image_alt=alt_text
    )

    logging.info(
        "Posted %s -> %s",
        path.name,
        post.uri
    )

    return post


# --------------------------------------------------
# Main loop
# --------------------------------------------------

def main():

    posted = load_posted()

    logging.info(
        "Bot started. %d images already posted.",
        len(posted)
    )

    while True:

        image = choose_image(posted)

        if image is None:

            logging.info(
                "No new artwork available. "
                "Checking again in 10 minutes."
            )

            time.sleep(600)
            continue

        try:

            post = post_image(image)

            posted.append(image.name)

            save_posted(posted)

            logging.info(
                "Successfully posted %s",
                image.name
            )

        except Exception as e:

            logging.exception(
                "Failed posting %s: %s",
                image.name,
                e
            )

        wait_minutes = random.randint(
            config.MIN_WAIT_MINUTES,
            config.MAX_WAIT_MINUTES
        )

        logging.info(
            "Sleeping for %d minutes.",
            wait_minutes
        )

        time.sleep(wait_minutes * 60)


if __name__ == "__main__":
    main()