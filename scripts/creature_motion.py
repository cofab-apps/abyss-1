"""Two registered movement cels per creature, built from its own indexed artwork."""
import math


# (left, top, width, height, movement) in the original 240px portraits.
# Keep the border fixed so that a partial blit meets the still backdrop.
REGIONS = [
    (14, 118, 164, 110, "jaw"), (16, 108, 132, 80, "jaw"),
    (14, 100, 142, 72, "jaw"), (12, 112, 160, 68, "jaw"),
    (12, 120, 216, 108, "arms"), (18, 108, 140, 70, "jaw"),
    (16, 104, 144, 70, "jaw"), (10, 12, 218, 102, "heads"),
    (12, 114, 216, 112, "arms"), (68, 66, 108, 66, "jaw"),
    (12, 42, 214, 100, "wings"), (14, 108, 146, 72, "jaw"),
    (14, 100, 140, 90, "jaw"), (12, 120, 216, 108, "arms"),
    (40, 110, 158, 86, "jaw"), (40, 110, 158, 86, "jaw"),
]


def displacement_mesh(region):
    """Inverse displacement in native pixels, sampled on an 8-pixel lattice."""
    _, _, width, height, motion = region
    points = []
    for y in range(0, height + 7, 8):
        ny = min(y, height - 1) / (height - 1)
        for x in range(0, width + 7, 8):
            nx = min(x, width - 1) / (width - 1)
            weight = math.sin(math.pi * nx) * math.sin(math.pi * ny)
            dx = dy = 0
            if motion == "jaw":
                dy = 17 * weight
            elif motion == "arms":
                dx = 20 * weight * math.sin(nx * 2 * math.pi + ny * 2)
                dy = -8 * weight
            elif motion == "heads":
                dx = 13 * weight
                dy = 13 * weight * math.cos(2 * math.pi * nx)
            elif motion == "wings":
                dy = -45 * weight * abs(2 * nx - 1)
            points.extend((round(dx), round(dy)))
    return points
