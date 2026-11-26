"""Verify a local package via the upstream web UI and AudioWorklet. Args: site, fwsc."""
import functools
import http.server
import os
from pathlib import Path
import sys
import threading
from playwright.sync_api import sync_playwright

site, firmware = map(Path, sys.argv[1:])
handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=site)
httpd = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
threading.Thread(target=httpd.serve_forever, daemon=True).start()
with sync_playwright() as p:
    browser = p.chromium.launch(executable_path=os.getenv("CHROMIUM"), args=["--no-sandbox"])
    page = browser.new_page()
    errors = []
    page.on("pageerror", lambda exc: errors.append(str(exc)))
    page.goto(f"http://127.0.0.1:{httpd.server_port}/")
    page.wait_for_function("location.search.includes('mhz=')")
    page.set_input_files("#open", firmware)
    page.wait_for_function("document.querySelector('#status').textContent.includes('ready')")
    page.click("#start")
    page.wait_for_function("""() => {
        const status = document.querySelector('#status').textContent;
        const time = status.match(/([0-9.]+) s$/);
        return status.includes('MHz') && time && Number(time[1]) >= 2;
    }""", timeout=60000)
    print(page.locator("#status").inner_text())
    assert not errors, errors
    browser.close()
httpd.shutdown()
