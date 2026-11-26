"""Exercise the documented deep link in touch Chromium. Pass a deployment URL or staged site folder."""
import functools
import hashlib
import http.server
import os
import sys
import threading
from pathlib import Path

from PIL import Image
from playwright.sync_api import sync_playwright

target, package = sys.argv[1:3]
use_file_picker = sys.argv[3:] == ["--file"]
if target.startswith(("https://", "http://")):
    launch_url = target if "?" in target else target.rstrip("/") + "/?fw=abyss-1&embed=1&mhz=60"
else:
    handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=target)
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    query = "fw=local&mhz=60" if use_file_picker else "fw=abyss-1&embed=1&mhz=60"
    launch_url = f"http://127.0.0.1:{server.server_port}/?{query}"

def expected_pixels(ids):
    """Sample each tile's interior directly from the supplied 240px portrait."""
    result = []
    scale = 1 if len(ids) == 1 else 2
    for slot, creature in enumerate(ids):
        art = Path(__file__).resolve().parents[1] / "prototype" / "abyss1" / "assets" / f"{creature:02d}.png"
        pic = Image.open(art).convert("RGB").quantize(colors=16, dither=Image.Dither.NONE).convert("RGB")
        for y in range(12, 182 if scale == 1 else 240 // scale - 3, 10):
            for x in range(12, 240 // scale - 3, 10):
                r, g, b = pic.getpixel((scale * x, scale * y))
                colour = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
                offset = (slot % 2) * 120 + x + 240 * ((slot // 2) * 120 + y)
                result.append([offset, colour])
    return result


def check_screen(page, ids):
    page.wait_for_function("""expected => {
        const canvas = document.querySelector('#big');
        const pixels = canvas.getContext('2d').getImageData(0, 0, 240, 240).data;
        return expected.every(([offset, expectedColour]) => {
            const i = 4 * offset;
            const c = ((pixels[i] >> 3) << 11) | ((pixels[i+1] >> 2) << 5) | (pixels[i+2] >> 3);
            return c === expectedColour;
        });
    }""", arg=expected_pixels(ids), timeout=15000)


with sync_playwright() as playwright:
    proxy = os.environ.get("ABYSS_BROWSER_PROXY")
    browser = playwright.chromium.launch(executable_path=os.environ.get("CHROMIUM"),
                                         proxy={"server": proxy} if proxy else None)
    page = browser.new_page(viewport={"width": 780, "height": 360}, is_mobile=True, has_touch=True,
                            ignore_https_errors=os.environ.get("ABYSS_TRUST_TEST_PROXY") == "1")
    errors = []
    page.on("pageerror", lambda exc: errors.append(str(exc)))
    # Monitor actual worklet output without changing its connection to the speakers.
    page.add_init_script("""window.AudioWorkletNode = class extends AudioWorkletNode {
        constructor(...args) {
            super(...args);
            const meter = this.context.createAnalyser();
            this.connect(meter);
            const frame = new Float32Array(meter.fftSize);
            window.audioPeak = () => {
                meter.getFloatTimeDomainData(frame);
                return Math.max(...frame.map(Math.abs));
            };
        }
    }""")
    page.add_init_script("localStorage.setItem('fm1.bigscreen','1')")
    title = Path(package).stem if use_file_picker else "ABYSS-1"
    if use_file_picker:
        # Public emulator reads the selected file locally. Disallow network writes
        # during this private-artifact test and identify the bytes at file selection.
        page.route("**/*", lambda route: route.continue_() if route.request.method in {"GET", "HEAD"} else route.abort())
        page.add_init_script("""addEventListener('change', async e => {
            if (e.target.id !== 'open') return;
            const bytes = await e.target.files[0].arrayBuffer();
            const hash = new Uint8Array(await crypto.subtle.digest('SHA-256', bytes));
            window.selectedHash = Array.from(hash, v => v.toString(16).padStart(2,'0')).join('');
        }, true)""")
    page.goto(launch_url, wait_until="commit", timeout=65000)
    if use_file_picker:
        page.wait_for_function("document.querySelector('#fw')?.options?.length > 1 || document.querySelector('#firmware')?.options?.length > 1", timeout=60000)
        page.locator("#open").set_input_files(package)
    page.wait_for_function("t => document.querySelector('#title')?.textContent.includes(t)", arg=title, timeout=60000)
    if use_file_picker:
        page.wait_for_function("typeof window.selectedHash === 'string'")
        actual_hash = page.evaluate("window.selectedHash")
        page.locator("#start").tap()
    else:
        with page.expect_response(lambda r: r.url.split("?")[0].endswith(".fwsc")) as pending:
            page.locator("#start").tap()
        response = pending.value
        assert response.ok
        actual_hash = hashlib.sha256(response.body()).hexdigest()
    assert actual_hash == hashlib.sha256(Path(package).read_bytes()).hexdigest()
    page.wait_for_function("Number(document.querySelector('#status').textContent.match(/([0-9.]+) s$/)?.[1]) >= 2", timeout=60000)
    assert "60 MHz" in page.locator("#status").inner_text()
    page.locator("#panel").scroll_into_view_if_needed()
    panel = page.locator("#panel").bounding_box()
    touch = page.context.new_cdp_session(page)
    point = {"x": panel["x"] + panel["width"] * (384 / 1280), "y": panel["y"] + panel["height"] * 0.90}
    idle = page.evaluate("audioPeak()")
    touch.send("Input.dispatchTouchEvent", {"type": "touchStart", "touchPoints": [point]})
    page.wait_for_function("audioPeak() > 0.005", timeout=5000)
    peak = page.evaluate("audioPeak()")
    touch.send("Input.dispatchTouchEvent", {"type": "touchEnd", "touchPoints": []})
    page.wait_for_function("audioPeak() < 0.003", timeout=15000)
    assert idle < 0.003 and not errors, (idle, errors)
    print(f"Firmware SHA-256 {actual_hash}")
    print(f"PASS simulator launch, exact package, boot, touch note and release. Idle {idle:.5f}, note peak {peak:.5f}")
    # Exercise the same touch controls shown to a player, with coordinates in
    # the upstream panel's native 1280 by 820 layout.
    def gesture(x, y, rise=0):
        panel = page.locator("#panel")
        panel.scroll_into_view_if_needed()
        box = panel.bounding_box()
        point = {"x": box["x"] + x * box["width"] / 1280, "y": box["y"] + y * box["height"] / 820}
        touch.send("Input.dispatchTouchEvent", {"type": "touchStart", "touchPoints": [point]})
        if rise:
            point["y"] -= rise * box["height"] / 820
            touch.send("Input.dispatchTouchEvent", {"type": "touchMove", "touchPoints": [point]})
        page.wait_for_timeout(140)
        touch.send("Input.dispatchTouchEvent", {"type": "touchEnd", "touchPoints": []})

    if os.environ.get("ABYSS_DEMO_BROWSER"):
        gesture(1105, 340)  # SEQ
        gesture(1163, 340)  # PLAY, without entering any notes
        page.wait_for_function("audioPeak() > 0.008", timeout=8000)
        gesture(1163, 340)
        page.wait_for_function("audioPeak() < 0.002", timeout=12000)
        assert not errors, errors
        print("PASS onscreen SEQ then PLAY started the factory groove; PLAY stopped it.")
        browser.close()
        sys.exit(0)
    ids = [0, 4, 8, 12]
    check_screen(page, ids)
    for part in range(4):
        if part:
            gesture(250, 135, 16)  # SELECT
        gesture(110, 295, 16)  # PRESETS
        ids[part] += 1
        check_screen(page, ids)
        gesture(989, 264)  # SCL
        check_screen(page, [ids[part]])
        gesture(931, 340)  # HOME
        check_screen(page, ids)
    gesture(250, 295, 16)  # ALGORITHM changes the fourth slot from atmosphere to bass
    ids[3] = 1
    check_screen(page, ids)
    assert not errors, errors
    print("PASS public-panel touch gestures: four PRESETS changes, four SCL portraits, four HOME returns and a cross-family ALGORITHM change. All four tiles matched after every reassignment.")
    def mark(x, y, colour):
        page.wait_for_function("""([x, y, c]) => {
            const v = document.querySelector('#big').getContext('2d').getImageData(x,y,1,1).data;
            return (((v[0] >> 3) << 11) | ((v[1] >> 2) << 5) | (v[2] >> 3)) === c;
        }""", arg=[x, y, colour], timeout=10000)

    gesture(1221, 340)  # REC arms
    mark(22, 10, 0xF800)
    gesture(384, 735)   # first key starts the clock
    mark(9, 10, 0x57EC)
    gesture(250, 135, 16)  # move from the fourth part to the first
    gesture(533, 735)
    gesture(1221, 340)  # end the take
    gesture(1163, 340)  # PLAY stops
    page.wait_for_function("audioPeak() < 0.001", timeout=10000)
    gesture(989, 340)   # SAVE
    mark(32, 10, 0xFFFF)
    page.wait_for_function("document.querySelector('#save-status').textContent.includes('Saved in this browser')", timeout=20000)
    page.reload(wait_until="domcontentloaded")
    page.wait_for_function("t => document.querySelector('#title')?.textContent.includes(t)", arg=title, timeout=60000)
    page.locator("#start").tap()
    check_screen(page, ids)
    gesture(1163, 340)
    mark(9, 10, 0x57EC)
    page.wait_for_function("audioPeak() > 0.005", timeout=10000)
    assert not errors, errors
    print("PASS REC, overdub, stop, SAVE, reload, Start, PLAY. All four assignments restored and the saved loop produced audio without a new note.")
    browser.close()
