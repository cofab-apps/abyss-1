"""Run both supplied prototype entry points. Pass the extracted handover directory."""
import functools
import http.server
import json
import os
import sys
import threading
from pathlib import Path
from playwright.sync_api import sync_playwright

root = Path(sys.argv[1]).resolve()
class Quiet(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *args):
        pass

server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), functools.partial(Quiet, directory=root))
threading.Thread(target=server.serve_forever, daemon=True).start()
with sync_playwright() as p:
    browser = p.chromium.launch(executable_path=os.getenv("CHROMIUM"), args=["--no-sandbox"])
    for entry in ("abyss1/index.html", "ABYSS-1-prototype.html"):
        page = browser.new_page()
        errors = []
        page.on("pageerror", lambda exc: errors.append(str(exc)))
        page.goto(f"http://127.0.0.1:{server.server_port}/{entry}")
        assert not errors, errors
        page.wait_for_function("pics.every(p => p.naturalWidth === 240)")
        assert page.eval_on_selector("#screen", "c=>[c.width,c.height]") == [240, 240]
        page.click("#next")
        assert page.evaluate("tracks[0].id") == 1
        page.click("#demo")
        page.wait_for_function("step >= 2 && ctx.state === 'running'")
        page.click("#stop")
        snapshot = page.evaluate("JSON.stringify(tracks)")
        page.click("#save")
        page.click("#clear")
        page.click("#load")
        assert page.evaluate("JSON.stringify(tracks)") == snapshot
        page.locator("#tracks button").nth(2).click()
        page.click("#mute")
        assert page.evaluate("tracks[2].mute && !tracks[1].mute")
        page.click("#mute")
        page.click("#clear")
        page.click("#rec")
        page.locator("#screen").click()  # Keyboard handler ignores a focused transport button.
        page.keyboard.press("a")
        page.click("#stop")
        assert page.evaluate("tracks[2].steps.includes(60)")
        assert not errors, errors
        print(json.dumps({"entry":entry, "portraits":16, "demo":True,"save_load":True,"record":True,"track_mute":True,"page_errors":errors}))
        page.close()
    browser.close()
server.shutdown()
