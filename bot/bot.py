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
import requests


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
# Send Message to Server
# --------------------------------------------------

SERVER_URL = "http://localhost:8080"


def send_status(status):
    try:
        requests.post(
            f"{SERVER_URL}/status",
            data=status,
            timeout=5
        )
    except Exception as e:
        logging.error(
            "Could not send status %s: %s",
            status,
            e
        )

# --------------------------------------------------
# Sleep with countdown
# --------------------------------------------------

def sleep_with_countdown(minutes):
    end_time = time.monotonic() + (minutes * 60)
    interval = 5 * 60

    while True:
        remaining_seconds = max(
            0,
            int(end_time - time.monotonic())
        )

        remaining_minutes = (remaining_seconds + 59) // 60

        if remaining_seconds <= 0:
            break

        send_status("RESET")
        time.sleep(0.1)
        
        print(
            f"Sleeping... {remaining_minutes} minutes remaining",
            flush=True
        )

        # Tell the Pico we're still in the sleeping state
        send_status("SLEEPING")

        sleep_time = min(interval, remaining_seconds)

        time.sleep(sleep_time)
        

    # Countdown is finished
    send_status("IDLE")

    print(
        "Sleep complete. Returning to idle.",
        flush=True
    )

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


def choose_images(posted):
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

    if len(available) >= 2 and random.random() < 0.5:
        return random.sample(available, 2)

    return [random.choice(available)]


# --------------------------------------------------
# Image preparation
# --------------------------------------------------

def prepare_image(paths):

    if len(paths) not in (1, 2):
        raise ValueError("Expected one or two source images.")

    source_images = []

    for path in paths:
        with Image.open(path) as image:
            source_images.append(image.copy())

    tile_size = source_images[0].size

    if any(image.size != tile_size for image in source_images):
        raise ValueError("Source images must have matching dimensions.")

    if len(source_images) == 1:
        cell_sources = [0, 0, 0, 0]
    else:
        cell_sources = [
            random.randrange(2)
            for _ in range(4)
        ]

        while len(set(cell_sources)) < 2:
            cell_sources = [
                random.randrange(2)
                for _ in range(4)
            ]

    tile_width, tile_height = tile_size
    grid = Image.new(
        source_images[0].mode,
        (tile_width * 2, tile_height * 2)
    )

    for cell_index, source_index in enumerate(cell_sources):
        grid.paste(
            source_images[source_index],
            (
                (cell_index % 2) * tile_width,
                (cell_index // 2) * tile_height
            )
        )

    temp_path = config.DATA_DIR / "_upload.png"

    grid.save(
        temp_path,
        "PNG",
        optimize=True
    )

    return temp_path


# --------------------------------------------------
# Caption
# --------------------------------------------------

def get_tile_numbers(paths):
    return [
        path.stem.replace("tile_", "")
        for path in paths
    ]


def make_caption(paths):

    numbers = get_tile_numbers(paths)
    tile_reference = (
        f"tile:{numbers[0]}"
        if len(numbers) == 1
        else f"tiles:{', '.join(numbers)}"
    )

    captions = [
        f"Random generative study - {tile_reference}",
        f"Procedural experiment of colors and shapes. {tile_reference}",
        f"Subdivided generative experiment - {tile_reference}",
        f"Pico2 generative art bot! {tile_reference}",
        f"Random Subdivided tiles <3 {tile_reference}",
    ]

    caption = random.choice(captions)

    return (
        f"{caption}\n\n"
        "#generativeart #creativecoding #avr #pico2 #digitalart"
    )


# --------------------------------------------------
# Post
# --------------------------------------------------

def post_image(paths):

    send_status("POSTING")
    time.sleep(2)

    image_names = ", ".join(
        path.name
        for path in paths
    )

    logging.info("Preparing %s", image_names)

    upload_path = prepare_image(paths)

    with open(upload_path, "rb") as f:
        image_data = f.read()

    if len(image_data) > config.MAX_IMAGE_SIZE:
        raise RuntimeError(
            f"Grid made from {image_names} is too large for Bluesky: "
            f"{len(image_data):,} bytes"
        )

    caption = make_caption(paths)

    numbers = get_tile_numbers(paths)

    if len(numbers) == 1:
        artwork_description = (
            f"Artwork #{numbers[0]} repeated in a 2 by 2 grid."
        )
    else:
        artwork_description = (
            f"Artworks #{numbers[0]} and #{numbers[1]} arranged "
            f"in a randomized 2 by 2 grid."
        )

    alt_text = (
        f"Generative artwork created by a Raspberry Pi Pico 2 "
        f"and displayed in the Pico2Tiles project. "
        f"{artwork_description}"
    )

    logging.info("Posting %s", image_names)

    post = client.send_image(
        text=caption,
        image=image_data,
        image_alt=alt_text
    )

    logging.info(
        "Posted %s -> %s",
        image_names,
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

        images = choose_images(posted)

        if images is None:

            logging.info(
                "No new artwork available. "
                "Checking again in 10 minutes."
            )

            time.sleep(600)
            continue

        try:

            post = post_image(images)

            posted.extend(
                image.name
                for image in images
            )

            save_posted(posted)

            send_status("POSTED")

            logging.info(
                "Successfully posted %s",
                ", ".join(
                    image.name
                    for image in images
                )
            )

        except Exception as e:

            send_status("ERROR")

            logging.exception(
                "Failed posting %s: %s",
                ", ".join(
                    image.name
                    for image in images
                ),
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

        #time.sleep(wait_minutes * 60)
        sleep_with_countdown(wait_minutes)

if __name__ == "__main__":
    main()

