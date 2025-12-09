import sys
import time
import argparse

class DummyFile:
    def write(self, x): pass
    def flush(self): pass

_old_stdout = sys.stdout
_old_stderr = sys.stderr
sys.stdout = DummyFile()
sys.stderr = DummyFile()

import pygame

sys.stdout = _old_stdout
sys.stderr = _old_stderr

# ---- parse command line args ----
parser = argparse.ArgumentParser(description="RPG-style dialogue display")
parser.add_argument("dialogue", type=str, help="Dialogue text to display")
parser.add_argument("--background", type=str, default="test.png", help="Background image path")
parser.add_argument("--icon", type=str, default="testbg.png", help="Character icon image path")
parser.add_argument("--sound", type=str, default="text.wav", help="Sound effect for letters")
parser.add_argument("--font", type=str, default="font.ttf", help="Font file")
parser.add_argument("--side", type=str, choices=["left", "right"], default="left", help="Side of the icon")
parser.add_argument("--fontsize", type=int, default=24, help="Font size")
parser.add_argument("--width", type=int, default=800, help="Window width")
parser.add_argument("--height", type=int, default=600, help="Window height")
parser.add_argument("--speed", type=float, default=0.05, help="Text speed per character (seconds)")
parser.add_argument("--iconsize", type=int, nargs=2, metavar=('WIDTH', 'HEIGHT'), default=[64, 64],
                    help="Icon size as WIDTH HEIGHT")
parser.add_argument("--padding", type=int, default=20, help="Padding between icon/text and edges")
parser.add_argument("--halign", type=str, choices=["left", "center", "right"], default="left", help="Horizontal alignment")
parser.add_argument("--valign", type=str, choices=["top", "middle", "bottom"], default="bottom", help="Vertical alignment")
args = parser.parse_args()

# ---- initialize pygame ----
pygame.init()
screen = pygame.display.set_mode((args.width, args.height), pygame.NOFRAME)
pygame.display.set_caption("dialogue")

# ---- load resources ----
bg_image = pygame.image.load(args.background)
bg_image = pygame.transform.scale(bg_image, (args.width, args.height))
screen.blit(bg_image, (0, 0))  # draw background once

icon = pygame.image.load(args.icon)
icon_size = tuple(args.iconsize)
icon = pygame.transform.scale(icon, icon_size)
screen.blit(icon, (args.padding if args.side == "left" else args.width - icon_size[0] - args.padding,
                   args.height - icon_size[1] - args.padding))
pygame.display.update()  # draw icon once

font = pygame.font.Font(args.font, args.fontsize)
blip_sound = pygame.mixer.Sound(args.sound)

line_spacing = 5

# ---- calculate text area ----
if args.side == "left":
    text_x_start = args.padding + icon_size[0] + args.padding
    text_max_width = args.width - text_x_start - args.padding
else:
    text_x_start = args.padding
    text_max_width = args.width - text_x_start - icon_size[0] - 2 * args.padding

# ---- wrap text before rendering ----
lines = []
current_line = ""
for char in args.dialogue:
    test_line = current_line + char
    if font.size(test_line)[0] > text_max_width and current_line != "":
        lines.append(current_line)
        current_line = char
    else:
        current_line = test_line
if current_line:
    lines.append(current_line)

line_height = font.get_height() + line_spacing
total_text_height = len(lines) * line_height - line_spacing

# ---- calculate vertical alignment ----
if args.valign == "top":
    y_base = args.padding
elif args.valign == "middle":
    y_base = (args.height - total_text_height) // 2
else:  # bottom
    y_base = args.height - total_text_height - args.padding

# ---- helper for punctuation timing ----
def get_delay(char):
    if char in ".!?":
        return args.speed * 5
    elif char in ",;:":
        return args.speed * 3
    else:
        return args.speed

# ---- render each line character by character ----
for line_index, line in enumerate(lines):
    x_base = 0  # will be updated per line
    # compute horizontal alignment
    line_width = font.size(line)[0]
    if args.halign == "left":
        x_base = text_x_start
    elif args.halign == "center":
        x_base = (args.width - line_width) // 2
    else:  # right
        x_base = args.width - line_width - args.padding

    y_pos = y_base + line_index * line_height
    rendered_text = ""
    for char in line:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
        rendered_text += char
        text_surface = font.render(rendered_text, True, (255, 255, 255))
        screen.blit(text_surface, (x_base, y_pos))
        pygame.display.update()
        if char != " ":
            blip_sound.play()
        time.sleep(get_delay(char))

# ---- keep window open until closed or Enter pressed ----
running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_RETURN:  # Enter key
                running = False

pygame.quit()

