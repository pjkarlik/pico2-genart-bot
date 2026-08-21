from http.server import BaseHTTPRequestHandler, HTTPServer
from PIL import Image
import os
import struct
import json
from pathlib import Path

HOST = "0.0.0.0"
PORT = 8080

WIDTH = 240
HEIGHT = 240
FRAME_SIZE = WIDTH * HEIGHT * 2  # RGB565 = 2 bytes/pixel

BASE_DIR = Path(__file__).resolve().parent

SAVE_DIR = BASE_DIR / "public" / "images"
IMAGE_LIST_FILE = BASE_DIR / "public" / "images.json"

SAVE_DIR.mkdir(parents=True, exist_ok=True)

os.makedirs(SAVE_DIR, exist_ok=True)

image_number = 201

def rgb565_to_image(data):
    """Convert raw RGB565 bytes into a Pillow RGB image."""

    pixels = []

    for i in range(0, len(data), 2):
        value = struct.unpack(">H", data[i:i + 2])[0]

        r = (value >> 11) & 0x1F
        g = (value >> 5) & 0x3F
        b = value & 0x1F

        # Expand 5/6-bit values to 8-bit
        r = (r * 255) // 31
        g = (g * 255) // 63
        b = (b * 255) // 31

        pixels.append((r, g, b))

    image = Image.new("RGB", (WIDTH, HEIGHT))
    image.putdata(pixels)

    return image

def update_image_list():
    files = [
        filename
        for filename in os.listdir(SAVE_DIR)
        if filename.lower().endswith(".png")
    ]

    # Newest first
    files.sort(reverse=True)

    data = {
        "images": files
    }

    with open(IMAGE_LIST_FILE, "w", encoding="utf-8") as file:
        json.dump(data, file, indent=4)

    print(f"Updated {IMAGE_LIST_FILE} ({len(files)} images)")
    
class TileHandler(BaseHTTPRequestHandler):

    def do_POST(self):

        global image_number

        if self.path != "/tile":
            self.send_response(404)
            self.end_headers()
            return

        content_length = int(self.headers.get("Content-Length", 0))

        if content_length != FRAME_SIZE:
            print(
                f"Wrong image size: "
                f"{content_length} bytes "
                f"(expected {FRAME_SIZE})"
            )

            self.send_response(400)
            self.end_headers()
            return

        data = self.rfile.read(content_length)

        image = rgb565_to_image(data)

        filename = f"tile_{image_number:04d}.png"
        filepath = os.path.join(SAVE_DIR, filename)

        image.save(filepath)

        print(f"Saved {filepath}")

        image_number += 1

        update_image_list()

        self.send_response(200)
        self.end_headers()
        self.wfile.write(b"OK")


    def do_GET(self):

        # -----------------------------
        # Serve image list
        # -----------------------------

        if self.path == "/images.json":

            if not os.path.isfile(IMAGE_LIST_FILE):
                self.send_response(404)
                self.end_headers()
                return

            with open(IMAGE_LIST_FILE, "rb") as file:
                data = file.read()

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()

            self.wfile.write(data)

            return


        # -----------------------------
        # Serve images
        # -----------------------------

        if self.path.startswith("/images/"):

            filename = self.path[len("/images/"):]

            if "/" in filename or "\\" in filename:
                self.send_response(400)
                self.end_headers()
                return

            filepath = os.path.join(SAVE_DIR, filename)

            if not os.path.isfile(filepath):
                self.send_response(404)
                self.end_headers()
                return

            with open(filepath, "rb") as file:
                data = file.read()

            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Content-Length", str(len(data)))
            self.send_header(
                "Cache-Control",
                "public, max-age=31536000"
            )
            self.end_headers()

            self.wfile.write(data)

            return

        # -----------------------------
        # Anything else
        # -----------------------------

        self.send_response(404)
        self.end_headers()


    def log_message(self, format, *args):
        # Keep the console output clean
        return


server = HTTPServer((HOST, PORT), TileHandler)

print(f"Tile server running on port {PORT}")
print(f"Saving images to: {os.path.abspath(SAVE_DIR)}")
print("Waiting for Pico...")

server.serve_forever()