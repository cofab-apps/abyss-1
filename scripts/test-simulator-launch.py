"""Exercise the documented deep link in touch Chromium. Pass a deployment URL or staged site folder."""
import functools
import hashlib
import http.server
import os
import sys
import threading
from pathlib import Path
from playwright.sync_api import sync_playwright

target, package = sys.argv[1:]
if target.startswith("https://"):
    origin = target.rstrip("/")
else:
    handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=target)
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    origin = f"http://127.0.0.1:{server.server_port}"

with sync_playwright() as playwright:
    browser = playwright.chromium.launch(executable_path=os.environ.get("CHROMIUM"))
    page = browser.new_page(viewport={"width": 780, "height": 360}, is_mobile=True, has_touch=True)
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
    page.goto(origin + "/?fw=abyss-1&embed=1&mhz=60", wait_until="domcontentloaded")
    with page.expect_response(lambda r: r.url.split("?")[0].endswith("/four-part-formant.fwsc")) as pending:
        page.locator("#start").tap()
    response = pending.value
    assert response.ok and hashlib.sha256(response.body()).digest() == hashlib.sha256(Path(package).read_bytes()).digest()
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
    print(f"PASS deep link, exact package, boot, touch note and release. Idle {idle:.5f}, note peak {peak:.5f}")
    browser.close()
