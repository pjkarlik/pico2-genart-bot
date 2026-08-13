from http.server import BaseHTTPRequestHandler, HTTPServer
from PIL import Image
import os
import struct
import json

HOST = "0.0.0.0"
PORT = 8080

WIDTH = 240
HEIGHT = 240
FRAME_SIZE = WIDTH * HEIGHT * 2  # RGB565 = 2 bytes/pixel

SAVE_DIR = "images"
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

        self.send_response(200)
        self.end_headers()
        self.wfile.write(b"OK")


    def do_GET(self):

        # -----------------------------
        # API: list images
        # -----------------------------

        if self.path == "/api/images":

            files = [
                filename
                for filename in os.listdir(SAVE_DIR)
                if filename.lower().endswith(".png")
            ]

            # Newest first
            files.sort(reverse=True)

            response = json.dumps(files).encode("utf-8")

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(response)))
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()

            self.wfile.write(response)

            return


        # -----------------------------
        # Serve images
        # -----------------------------

        if self.path.startswith("/images/"):

            filename = self.path[len("/images/"):]

            # Basic security check
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
            self.send_header("Cache-Control", "public, max-age=31536000")
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