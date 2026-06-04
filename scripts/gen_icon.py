"""
CSP-inspired app icon for LayeredPaintApp.
- Deep navy rounded-square bg with subtle gradient
- Elegant silver stylus, top-right → bottom-left
- Organic paint splash (scattered drops + ink bloom) at nib
- No heavy glow rings — clean and professional
"""

import math
from PIL import Image, ImageDraw, ImageFilter

def lerp(a, b, t):
    t = max(0.0, min(1.0, t))
    return a + (b - a) * t

def lerp_c(c1, c2, t):
    return tuple(int(lerp(c1[i], c2[i], t)) for i in range(len(c1)))

def rounded_mask(S, r):
    m = Image.new("L", (S, S), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, S-1, S-1], radius=r, fill=255)
    return m

def make_icon(size):
    S = size
    OVER = 4
    W = S * OVER
    ws = W / 256.0

    img = Image.new("RGBA", (W, W), (0,0,0,0))
    d = ImageDraw.Draw(img)

    def p(fx, fy):
        return (int(fx * W), int(fy * W))

    def r(v):
        return max(1, int(v * ws))

    # ── Background ──────────────────────────────────────────────────────────
    for y in range(W):
        t = y / W
        c = lerp_c((24, 30, 64), (10, 13, 36), t)
        d.line([(0, y), (W-1, y)], fill=(*c, 255))

    # Very subtle warm-center vignette (just barely visible)
    cx, cy = int(W * 0.55), int(W * 0.42)
    for step in range(6, 0, -1):
        ri = int(W * 0.35 * step / 6)
        alpha = 8
        d.ellipse([cx-ri, cy-ri, cx+ri, cy+ri], fill=(60, 80, 150, alpha))

    # ── Paint splash — organic scattered drops ───────────────────────────────
    # Main ink bloom at nib tip area (around 0.28, 0.70)
    bx, by = int(0.28 * W), int(0.70 * W)

    # Bloom layers (dark → vivid center)
    bloom_colors = [
        (0.14, (20, 60, 160, 80)),
        (0.10, (30, 80, 200, 110)),
        (0.07, (50, 110, 230, 140)),
        (0.04, (80, 145, 255, 170)),
        (0.02, (130, 185, 255, 200)),
    ]
    for frac, col in bloom_colors:
        ri = int(frac * W)
        d.ellipse([bx-ri, by-ri, bx+ri, by+ri], fill=col)

    # Organic ink splatter drops (sizes + positions vary naturally)
    drops = [
        # (fx, fy, fr,  color,               alpha)
        (0.19, 0.62, 0.028, (220,  55,  55),   230),  # red
        (0.12, 0.70, 0.024, ( 45, 115, 225),   225),  # blue
        (0.22, 0.80, 0.020, ( 40, 180,  80),   220),  # green
        (0.10, 0.80, 0.016, (235, 175,  30),   220),  # yellow
        (0.34, 0.76, 0.018, (175,  55, 215),   215),  # purple
        (0.36, 0.64, 0.014, (230,  95,  35),   210),  # orange
        (0.15, 0.87, 0.013, ( 30, 185, 200),   200),  # cyan
        # tiny specks
        (0.08, 0.64, 0.008, (220,  55,  55),   200),
        (0.40, 0.70, 0.007, ( 40, 180,  80),   195),
        (0.25, 0.88, 0.009, ( 45, 115, 225),   190),
        (0.42, 0.58, 0.006, (235, 175,  30),   185),
    ]
    for fx, fy, fr, col, alpha in drops:
        dcx, dcy = int(fx * W), int(fy * W)
        dr2 = max(2, int(fr * W))
        # Body with slight gradient feel
        for step in range(dr2, 0, -max(1, dr2//4)):
            t2 = 1.0 - step / dr2
            c2 = lerp_c(col, (min(255, col[0]+40), min(255, col[1]+40), min(255, col[2]+40)), t2 * 0.5)
            d.ellipse([dcx-step, dcy-step, dcx+step, dcy+step], fill=(*c2, alpha))
        # Specular
        hs = max(1, dr2 // 3)
        d.ellipse([dcx-hs, dcy-dr2+max(1,dr2//5),
                   dcx+hs//2, dcy-dr2//2],
                  fill=(255, 255, 255, 120))

    # Ink splatter streaks (thin elongated ovals radiating from bloom)
    streaks = [
        (0.22, 0.60, 0.018, 0.007, -30, (220, 55, 55),  190),
        (0.38, 0.68, 0.016, 0.006,  10, (45, 115, 225), 180),
        (0.10, 0.76, 0.014, 0.005, -70, (40, 180, 80),  175),
        (0.32, 0.82, 0.015, 0.006,  40, (175, 55, 215), 170),
    ]
    for fx, fy, frl, frs, angle_deg, col, alpha in streaks:
        scx, scy = int(fx * W), int(fy * W)
        rl, rs = max(2, int(frl * W)), max(1, int(frs * W))
        angle = math.radians(angle_deg)
        # Rotate bounding box
        pts = []
        for (ex, ey) in [(-rl, -rs), (rl, -rs), (rl, rs), (-rl, rs)]:
            rx2 = ex * math.cos(angle) - ey * math.sin(angle)
            ry2 = ex * math.sin(angle) + ey * math.cos(angle)
            pts.append((scx + int(rx2), scy + int(ry2)))
        d.polygon(pts, fill=(*col, alpha))

    # ── Stylus pen ───────────────────────────────────────────────────────────
    # Axis: handle (0.86, 0.09) → tip (0.28, 0.70)
    ax0, ay0 = 0.86, 0.09
    ax1, ay1 = 0.28, 0.70

    dxx = ax1 - ax0
    dyy = ay1 - ay0
    length = math.hypot(dxx, dyy)
    nx, ny = -dyy / length, dxx / length  # left-normal

    def pp(t, hw):   # point along pen axis at param t, offset hw perpendicular
        x = ax0 + dxx * t + nx * hw
        y = ay0 + dyy * t + ny * hw
        return (int(x * W), int(y * W))

    def quad(t0, t1, hw0, hw1, fill):
        d.polygon([pp(t0, hw0), pp(t0, -hw0), pp(t1, -hw1), pp(t1, hw1)], fill=fill)

    def quad4(t0, hw0, t1, hw1, t2, hw2, t3, hw3, fill):
        d.polygon([pp(t0, hw0), pp(t1, hw1), pp(t2, hw2), pp(t3, hw3)], fill=fill)

    BW = 0.040   # barrel half-width at handle end
    BW2 = 0.028  # barrel half-width at ferrule start

    # -- Body --
    quad(0.12, 0.70, BW,  BW2,  (178, 185, 212))  # base color
    quad(0.12, 0.70, BW * 0.85,  BW2 * 0.85,
         (210, 216, 238))                           # highlight stripe (left)
    quad4(0.12, -BW * 0.5, 0.12, -BW,
          0.70, -BW2, 0.70, -BW2 * 0.5,
          (130, 136, 162))                          # shadow stripe (right)

    # Thin accent line along body center
    for ti in range(0, 101, 2):
        t2 = 0.12 + (0.70 - 0.12) * ti / 100
        hw = lerp(BW, BW2, ti / 100) * 0.05
        pt0, pt1 = pp(t2, hw), pp(t2 + 0.02, hw)
        d.line([pt0, pt1], fill=(100, 105, 132, 80), width=max(1, r(1)))

    # -- End cap --
    cap_r = max(2, int(BW * 0.92 * W))
    tip_cap = p(ax0, ay0)
    d.ellipse([tip_cap[0]-cap_r, tip_cap[1]-cap_r,
               tip_cap[0]+cap_r, tip_cap[1]+cap_r],
              fill=(192, 198, 222))
    quad(0.00, 0.12, BW * 0.88, BW, (192, 198, 222))
    # Highlight on cap
    d.ellipse([tip_cap[0]-cap_r+r(2), tip_cap[1]-cap_r+r(1),
               tip_cap[0]+r(1),       tip_cap[1]+r(2)],
              fill=(232, 236, 252))

    # -- Grip rings --
    for rt in [0.15, 0.19]:
        rw = 0.006
        quad(rt, rt + rw, BW + 0.006, BW2 + lerp(0.006, 0.000, (rt-0.15)/0.04),
             (95, 100, 126))
        quad(rt, rt + rw, BW * 0.2, BW * 0.8,
             (148, 153, 178))

    # -- Ferrule --
    FW = BW2 + 0.012
    quad(0.70, 0.80, FW, FW * 0.90, (112, 118, 144))
    quad(0.70, 0.80, FW * 0.90, FW * 0.10, (158, 163, 190))  # highlight

    # -- Nib --
    NW = FW * 0.82
    nib_pts = [pp(0.80, NW), pp(0.80, -NW), p(ax1, ay1)]
    d.polygon(nib_pts, fill=(48, 52, 76))
    nib_hl = [pp(0.80, NW), pp(0.80, 0.003), p(ax1, ay1)]
    d.polygon(nib_hl, fill=(82, 88, 116))

    # -- Ink at tip --
    tip_px, tip_py = int(ax1 * W), int(ay1 * W)
    ir = max(2, int(0.022 * W))
    d.ellipse([tip_px-ir, tip_py-ir, tip_px+ir, tip_py+ir],
              fill=(55, 135, 240, 220))
    d.ellipse([tip_px-ir//2, tip_py-ir,
               tip_px+ir//3, tip_py-ir//3],
              fill=(180, 218, 255, 150))

    # ── Border: hairline white rim ───────────────────────────────────────────
    rad = int((38 / 256) * W)
    for i in range(3):
        a = int(55 * (1 - i / 3))
        d.rounded_rectangle([i, i, W-1-i, W-1-i], radius=rad-i,
                             outline=(255, 255, 255, a), width=1)

    # ── Downsample + mask ────────────────────────────────────────────────────
    out = img.resize((S, S), Image.LANCZOS)
    mask = rounded_mask(S, int((38/256) * S))
    out.putalpha(mask)
    if size <= 24:
        out = out.filter(ImageFilter.SHARPEN)
    return out


if __name__ == "__main__":
    import os
    sizes = [16, 24, 32, 48, 64, 128, 256]
    images = [make_icon(s) for s in sizes]

    base = os.path.normpath(
        os.path.join(os.path.dirname(__file__), r"..\src\app\resources"))

    ico_path = os.path.join(base, "app_icon.ico")
    png_path = os.path.join(base, "app_icon_256.png")

    images[0].save(ico_path, format="ICO",
                   sizes=[(s,s) for s in sizes],
                   append_images=images[1:])
    print(f"Saved ICO:  {ico_path}")

    images[-1].save(png_path, format="PNG")
    print(f"Saved PNG:  {png_path}")
