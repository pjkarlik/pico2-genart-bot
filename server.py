from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse
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

image_number = 176


def rgb565_to_image(data):
    """Convert raw RGB565 bytes into a Pillow RGB image."""

    pixels = []

    for i in range(0, len(data), 2):

        value = struct.unpack(">H", data[i:i + 2])[0]

        r = (value >> 11) & 0x1F
        g = (value >> 5) & 0x3F
        b = value & 0x1F

        r = (r * 255) // 31
        g = (g * 255) // 63
        b = (b * 255) // 31

        pixels.append((r, g, b))

    image = Image.new("RGB", (WIDTH, HEIGHT))
    image.putdata(pixels)

    return image


class TileHandler(BaseHTTPRequestHandler):

    def do_GET(self):

        parsed = urlparse(self.path)
        path = parsed.path

        print(f"GET {path}")

        # Gallery homepage
        if path == "/" or path == "":
            self.send_gallery()
            return

        # Image list
        if path == "/images":
            self.send_image_list()
            return

        # Serve individual images
        if self.path.startswith("/images/"):
            filename = self.path[len("/images/"):]

            # Basic safety check
            if "/" in filename or "\\" in filename:
                self.send_response(400)
                self.end_headers()
                return

            filepath = os.path.join(SAVE_DIR, filename)

            if not os.path.isfile(filepath):
                self.send_response(404)
                self.end_headers()
                return

            with open(filepath, "rb") as f:
                data = f.read()

            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Content-Length", len(data))
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()

            self.wfile.write(data)
            return

        self.send_response(404)
        self.end_headers()

    def do_POST(self):

        global image_number

        if self.path != "/tile":
            self.send_response(404)
            self.end_headers()
            return

        content_length = int(
            self.headers.get("Content-Length", 0)
        )

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

    def send_image_list(self):

        files = [
            f for f in os.listdir(SAVE_DIR)
            if f.lower().endswith(".png")
        ]

        files.sort()

        result = []

        for filename in files:

            filepath = os.path.join(SAVE_DIR, filename)

            result.append({
                "filename": filename,
                "url": "/images/" + filename,
                "timestamp": os.path.getmtime(filepath)
            })

        data = json.dumps(result).encode()

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", len(data))
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()

        self.wfile.write(data)

    def send_gallery(self):

        html = """
<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<title>Pico Tile Gallery</title>

<style>

* {
    box-sizing: border-box;
}

body {
    margin: 0;
    background: #111;
    color: #fff;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI",
                 sans-serif;
}

header {
    position: sticky;
    top: 0;
    z-index: 10;

    padding: 20px 30px;

    background: rgba(15, 15, 15, 0.95);

    border-bottom: 1px solid #333;

    display: flex;
    justify-content: space-between;
    align-items: center;
}

h1 {
    margin: 0;
    font-size: 22px;
    font-weight: 500;
}

#status {
    color: #888;
    font-size: 14px;
}

#gallery {

    padding: 30px;

    display: grid;

    grid-template-columns:
        repeat(auto-fill, minmax(240px, 1fr));

    gap: 24px;
}

.tile {

    background: #1c1c1c;

    border-radius: 8px;

    overflow: hidden;

    box-shadow:
        0 4px 20px rgba(0,0,0,0.4);

    transition:
        transform 0.2s,
        box-shadow 0.2s;
}

.tile:hover {

    transform: translateY(-4px);

    box-shadow:
        0 8px 30px rgba(0,0,0,0.6);
}

.tile img {

    display: block;

    width: 100%;

    aspect-ratio: 1;

    image-rendering: pixelated;

    background: #000;
}

.info {

    padding: 10px 12px;

    color: #888;

    font-size: 12px;
}

</style>

</head>

<body>

<header>

    <h1>🎨 Pico Tile Gallery</h1>

    <div id="status">
        Connecting...
    </div>

</header>

<div id="gallery"></div>

<script>

let knownImages = new Set();

async function updateGallery() {

    const status = document.getElementById("status");

    try {

        status.textContent = "Checking...";

        const response = await fetch(
            "/images?t=" + Date.now(),
            {
                cache: "no-store"
            }
        );

        if (!response.ok) {
            throw new Error(
                "HTTP " + response.status
            );
        }

        const images = await response.json();

        console.log("Images:", images);

        const gallery =
            document.getElementById("gallery");

        for (const image of images) {

            if (knownImages.has(image.filename)) {
                continue;
            }

            knownImages.add(image.filename);

            const tile =
                document.createElement("div");

            tile.className = "tile";

            const img =
                document.createElement("img");

            img.src =
                image.url 

            img.loading = "lazy";

            const info =
                document.createElement("div");

            info.className = "info";

            info.textContent =
                image.filename;

            tile.appendChild(img);
            tile.appendChild(info);

            // Newest image goes at the top
            gallery.prepend(tile);
        }

        status.textContent =
            images.length + " images";

    }

    catch (error) {

        console.error(
            "Gallery error:",
            error
        );

        status.textContent =
            "Error: " + error.message;
    }
}


// Initial load
updateGallery();

// Check every second
setInterval(updateGallery, 1000);

</script>

</body>

</html>
"""

        data = html.encode()

        self.send_response(200)
        self.send_header("Content-Type", "text/html")
        self.send_header("Content-Length", len(data))
        self.end_headers()

        self.wfile.write(data)

    def log_message(self, format, *args):
        # Keep terminal output clean
        return

server = ThreadingHTTPServer((HOST, PORT), TileHandler)
print()
print("===================================")
print("       Pico Tile Gallery")
print("===================================")
print()
print(f"Server: http://localhost:{PORT}")
print(f"Images: {os.path.abspath(SAVE_DIR)}")
print()
print("Waiting for Pico...")
print()

server.serve_forever()
