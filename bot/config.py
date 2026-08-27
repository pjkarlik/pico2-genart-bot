from pathlib import Path

# --------------------------------------------------
# Paths
# --------------------------------------------------

BASE_DIR = Path(__file__).resolve().parent

# Public website files
PUBLIC_DIR = BASE_DIR.parent / "public"
PUBLIC_IMAGE_DIR = PUBLIC_DIR / "images"
IMAGE_LIST_FILE = PUBLIC_DIR / "images.json"

# Generated images
IMAGE_DIR = PUBLIC_IMAGE_DIR

DATA_DIR = BASE_DIR / "data"
LOG_DIR = BASE_DIR / "logs"

POSTED_FILE = DATA_DIR / "posted.json"


# --------------------------------------------------
# Website
# --------------------------------------------------

SITE_URL = "https://pico2tiles.surge.sh"


# --------------------------------------------------
# Posting
# --------------------------------------------------

# Average roughly one post per 3 hours.
MIN_WAIT_MINUTES = 420
MAX_WAIT_MINUTES = 580


# How many images should be eligible
# before we start recycling old artwork.
MAX_POSTED_HISTORY = 500


# --------------------------------------------------
# Bluesky
# --------------------------------------------------

BLUESKY_SERVICE = "https://bsky.social"


# --------------------------------------------------
# Image rules
# --------------------------------------------------

MAX_IMAGE_SIZE = 2 * 1024 * 1024