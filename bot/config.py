from pathlib import Path

# --------------------------------------------------
# Paths
# --------------------------------------------------

BASE_DIR = Path(__file__).resolve().parent

IMAGE_DIR = BASE_DIR.parent / "images"

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
MIN_WAIT_MINUTES = 245
MAX_WAIT_MINUTES = 290


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