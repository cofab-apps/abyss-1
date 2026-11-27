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
firmware_id = "abyss-monsters"
if target.startswith(("https://", "http://")):
    launch_url = target if "?" in target else target.rstrip("/") + f"/?fw={firmware_id}&embed=1&mhz=60"
else:
    site = Path(target).resolve()
    bundled = site.name == "simulator" and (site.parent/"builds").is_dir()
    handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=site.parent if bundled else site)
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    query = "fw=local&mhz=60" if use_file_picker else f"fw={firmware_id}&embed=1&mhz=60"
    prefix = "/simulator" if bundled else ""
    launch_url = f"http://127.0.0.1:{server.server_port}{prefix}/?{query}"

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

    def mark(x, y, colour):
        page.wait_for_function("""([x, y, c]) => {
            const v = document.querySelector('#big').getContext('2d').getImageData(x,y,1,1).data;
            return (((v[0] >> 3) << 11) | ((v[1] >> 2) << 5) | (v[2] >> 3)) === c;
        }""", arg=[x, y, colour], timeout=10000)

    # Match the actual idle LCD to the target capture, then turn SELECT
    # one detent and check that the portrait and worklet both respond.
    base = Path(__file__).resolve().parents[1]
    img = Image.open(base / "monster-keys/hydra.png").convert("RGB")
    def colour(x, y):
        r, g, b = img.getpixel((x, y))
        return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)
    for x,y in [(80,75),(112,110),(150,143)]:
        mark(x,y,colour(x,y))
    gesture(250,135,16)
    page.wait_for_function("audioPeak() > 0.005", timeout=5000)
    page.wait_for_function("audioPeak() < 0.002", timeout=12000)
    gesture(250,135,-16)
    page.wait_for_timeout(1800)
    for x,y in [(80,75),(112,110),(150,143)]:
        mark(x,y,colour(x,y))
    assert not errors, errors
    print("PASS Hydra LCD matches the firmware capture, SELECT audibly changes creature and returns to Hydra.")
    browser.close()
