"""Check the player documentation against the package selected by the bundled simulator."""
import json
import re
from pathlib import Path
from urllib.parse import unquote, urlsplit

root = Path(__file__).resolve().parents[1]
catalog = json.loads((root / "simulator/firmwares.json").read_text())
current = next(item for item in catalog["firmwares"] if item["id"] == catalog["default"])
version = current["version"]
firmware = root / "simulator" / current["package"]
if not firmware.is_file() or not firmware.name.endswith(f"-{version}.fwsc"):
    raise SystemExit(f"Simulator package and version disagree: {firmware.name}, {version}")

def github_heading(heading):
    # The guides use plain headings. GitHub keeps hyphens and removes punctuation.
    return re.sub(r"[^\w -]", "", heading.lower()).replace(" ", "-")

pages = ("README.md", "docs/FEATURES.md", "docs/MANUAL.md")
errors = []
for filename in pages:
    page = root / filename
    body = page.read_text()
    if not re.search(rf"(?<![\d.]){re.escape(version)}(?![\d.])", body.splitlines()[0]):
        errors.append(f"{filename}: heading must identify release {version}")
    for label, link in re.findall(r"(!?\[[^\]\n]*\])\(([^)\s]+)\)", body):
        target = urlsplit(link)
        if target.scheme or target.netloc:
            continue
        path = page.parent / unquote(target.path) if target.path else page
        if not path.exists():
            errors.append(f"{filename}: missing {link}")
        elif target.fragment and path.suffix == ".md":
            headings = re.findall(r"^#{1,6} (.+)$", path.read_text(), re.MULTILINE)
            if unquote(target.fragment) not in {github_heading(h) for h in headings}:
                errors.append(f"{filename}: missing heading {link}")
        if label.startswith("!") and not label[2:-1].strip():
            errors.append(f"{filename}: image needs a description: {link}")
if errors:
    raise SystemExit("\n".join(errors))
print(f"Player documentation matches the {version} release heading and all local guide links resolve.")
