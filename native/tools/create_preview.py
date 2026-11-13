import os
from PIL import Image, ImageDraw

def create_preview():
    texture_dir = "/home/ubuntu/eldoria/native/assets/textures/"
    output_path = "/home/ubuntu/eldoria/native/assets/preview_panel.png"
    
    textures = [
        "grass_stylized.png",
        "stone_stylized.png",
        "wood_stylized.png",
        "ore_aurora.png",
        "npc_faces.png"
    ]
    
    # Create a large canvas
    canvas_w, canvas_h = 1200, 800
    canvas = Image.new('RGB', (canvas_w, canvas_h), (30, 30, 35))
    draw = ImageDraw.Draw(canvas)
    
    # Draw title
    # draw.text((50, 20), "Eldoria Visual Assets - Stylized & Rounded", fill=(255, 255, 255))
    
    # Paste textures in a grid
    x_offset, y_offset = 50, 80
    thumb_size = 200
    for i, tex_name in enumerate(textures):
        path = os.path.join(texture_dir, tex_name)
        if os.path.exists(path):
            img = Image.open(path).convert('RGB')
            img.thumbnail((thumb_size, thumb_size))
            canvas.paste(img, (x_offset, y_offset))
            # draw.text((x_offset, y_offset + thumb_size + 5), tex_name.replace("_stylized.png", ""), fill=(200, 200, 200))
            
            x_offset += thumb_size + 40
            if x_offset > canvas_w - 200:
                x_offset = 50
                y_offset += thumb_size + 60

    # Draw a mockup of a rounded cube NPC
    # We'll simulate this with drawing primitives since we can't render 3D to image easily here
    npc_x, npc_y = 800, 150
    # Body
    draw.rounded_rectangle([npc_x, npc_y, npc_x+150, npc_y+250], radius=20, fill=(100, 150, 250))
    # Head
    draw.rounded_rectangle([npc_x+10, npc_y-130, npc_x+140, npc_y], radius=30, fill=(255, 220, 180))
    # Face (from texture)
    face_path = os.path.join(texture_dir, "npc_faces.png")
    if os.path.exists(face_path):
        face = Image.open(face_path).convert('RGBA')
        face.thumbnail((80, 80))
        canvas.paste(face, (npc_x+35, npc_y-100), face)

    canvas.save(output_path)
    print(f"Preview saved to {output_path}")

if __name__ == "__main__":
    create_preview()
