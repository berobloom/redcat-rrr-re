#!/usr/bin/env python3
"""Drive the web build in headless Chromium: serve port/web/out, open
redcat.html (or $RC_PAGE), follow a script of waits and keys, screenshot the
canvas and print the program's console output.

Usage: test_web.py <out dir> [step ...]
  steps:  wait:<s>  shot:<name>  key:<Key> (Playwright key name)
          hold:<Key>:<s>  click (focus the canvas)
          clickat:<x>:<y> (a click at 640x480 game coordinates)
Needs: pip install playwright (any cached Chromium; set RC_CHROME to pick one)
"""
import functools
import glob
import http.server
import os
import sys
import threading
import time

from playwright.sync_api import sync_playwright

HERE = os.path.dirname(os.path.abspath(__file__))
out = sys.argv[1]
os.makedirs(out, exist_ok=True)

class Quiet(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *args):
        pass


handler = functools.partial(Quiet, directory=os.path.join(HERE, 'out'))
srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), handler)
threading.Thread(target=srv.serve_forever, daemon=True).start()

chrome = os.environ.get('RC_CHROME') or sorted(glob.glob(os.path.expanduser(
    '~/.cache/ms-playwright/chromium-*/chrome-linux*/chrome')))[-1]
with sync_playwright() as p:
    b = p.chromium.launch(executable_path=chrome,
                          args=['--use-gl=swiftshader', '--enable-unsafe-swiftshader'])
    page = b.new_page(viewport={'width': 1300, 'height': 980})
    page.on('console', lambda m: print('console:', m.text))
    page.on('pageerror', lambda e: print('pageerror:', e))
    page.goto('http://127.0.0.1:%d/%s' % (srv.server_address[1],
                                          os.environ.get('RC_PAGE', 'redcat.html')))
    for step in sys.argv[2:]:
        kind, _, arg = step.partition(':')
        if kind == 'wait':
            time.sleep(float(arg))
        elif kind == 'shot':
            page.locator('canvas').screenshot(path=os.path.join(out, arg + '.png'))
        elif kind == 'click':
            page.locator('canvas').click()
        elif kind == 'clickat':
            gx, _, gy = arg.partition(':')
            box = page.locator('canvas').bounding_box()
            page.mouse.click(box['x'] + float(gx) * box['width'] / 640,
                             box['y'] + float(gy) * box['height'] / 480)
        elif kind == 'key':
            page.keyboard.press(arg)
        elif kind == 'hold':
            key, _, secs = arg.partition(':')
            page.keyboard.down(key)
            time.sleep(float(secs))
            page.keyboard.up(key)
    b.close()
srv.shutdown()
