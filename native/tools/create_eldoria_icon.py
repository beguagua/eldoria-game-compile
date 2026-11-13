"""Create the Eldoria macOS application icon without external art dependencies."""
from PIL import Image, ImageDraw, ImageFilter
from pathlib import Path

root = Path(__file__).resolve().parents[1]
output = root / "assets" / "icons" / "Eldoria_512.png"
windows_output = root / "assets" / "icons" / "Eldoria.ico"
output.parent.mkdir(parents=True, exist_ok=True)

size = 512
img = Image.new("RGBA", (size, size), (15, 31, 39, 255))
draw = ImageDraw.Draw(img, "RGBA")

# Mineral sky, dim silhouette and the distinctive three-pronged Aurora sigil.
for y in range(size):
    t = y / (size - 1)
    r = int(23 + 23 * t)
    g = int(67 + 34 * t)
    b = int(76 + 20 * t)
    draw.line((0, y, size, y), fill=(r, g, b, 255))

glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
gd = ImageDraw.Draw(glow, "RGBA")
gd.ellipse((116, 90, 396, 370), fill=(79, 197, 210, 145))
glow = glow.filter(ImageFilter.GaussianBlur(34))
img = Image.alpha_composite(img, glow)
draw = ImageDraw.Draw(img, "RGBA")

draw.polygon([(0, 395), (86, 305), (166, 378), (241, 270), (327, 388), (418, 318), (512, 407), (512, 512), (0, 512)], fill=(18, 42, 47, 255))
draw.ellipse((93, 93, 419, 420), outline=(235, 179, 83, 255), width=20)
draw.ellipse((119, 119, 393, 394), outline=(95, 199, 207, 170), width=7)

points = [(256, 128), (291, 228), (374, 249), (303, 286), (321, 372), (256, 321), (190, 372), (209, 286), (138, 249), (221, 228)]
draw.polygon(points, fill=(244, 224, 156, 255))
draw.polygon([(256, 168), (275, 238), (335, 255), (281, 278), (292, 333), (256, 296), (219, 333), (231, 278), (177, 255), (236, 238)], fill=(81, 184, 195, 255))
draw.ellipse((239, 238, 273, 272), fill=(250, 242, 202, 255))

img.save(output)
img.save(windows_output, format="ICO", sizes=[(16, 16), (32, 32), (48, 48), (128, 128), (256, 256)])
print(output)
print(windows_output)
