import sys
from PIL import Image

img = Image.open('resources/logo.png')
# Windows icons typically need 256x256, 128x128, 64x64, 32x32, 16x16
icon_sizes = [(256, 256), (128, 128), (64, 64), (32, 32), (16, 16)]
img.save('resources/logo.ico', format='ICO', sizes=icon_sizes)
print("Saved logo.ico")
