"""Local read-only game monitor. Python 3 standard library only."""
import argparse
import json
import ctypes
import threading
import urllib.request
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--data', required=True, type=Path, help='Path to executable/GameData/monitor.json')
parser.add_argument('--port', type=int, default=8765)
parser.add_argument('--parent-pid', type=int)
args = parser.parse_args()
page = Path(__file__).with_name('index.html').read_bytes()

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        route = self.path.split('?')[0]
        status, mime = 200, 'application/json; charset=utf-8'
        if route == '/api/health':
            payload = json.dumps({'service': 'ash-world-monitor', 'data': str(args.data.resolve())}).encode('utf-8')
        elif route == '/':
            payload, mime = page, 'text/html; charset=utf-8'
        elif route == '/api/state':
            try:
                payload = args.data.read_bytes()
                json.loads(payload)
            except (OSError, ValueError):
                status, payload = 503, b'{"error":"snapshot_unavailable"}'
        else:
            status, payload = 404, b'{"error":"not_found"}'
        self.send_response(status)
        self.send_header('Content-Type', mime)
        self.send_header('Content-Length', str(len(payload)))
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.end_headers()
        self.wfile.write(payload)

    def log_message(self, *_):
        pass

def watch_parent(server):
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes = [ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong]
    kernel.OpenProcess.restype = ctypes.c_void_p
    kernel.WaitForSingleObject.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
    kernel.CloseHandle.argtypes = [ctypes.c_void_p]
    handle = kernel.OpenProcess(0x00100000, False, args.parent_pid)
    if not handle:
        server.shutdown()
        return
    try:
        kernel.WaitForSingleObject(handle, 0xFFFFFFFF)
        server.shutdown()
    finally:
        kernel.CloseHandle(handle)

try:
    server = ThreadingHTTPServer(('127.0.0.1', args.port), Handler)
except OSError:
    try:
        with urllib.request.urlopen(f'http://127.0.0.1:{args.port}/api/health', timeout=2) as response:
            health = json.load(response)
        if health.get('service') == 'ash-world-monitor' and Path(health['data']).resolve() == args.data.resolve():
            sys.exit(0)  # Reuse only a matching server; never stop another process.
    except (OSError, ValueError, KeyError):
        pass
    args.data.parent.mkdir(parents=True, exist_ok=True)
    args.data.with_name('monitor-server-error.txt').write_text('8765 포트를 사용할 수 없습니다. 기존 서버의 경로와 포트를 확인하세요.', encoding='utf-8')
    sys.exit(1)

if args.parent_pid:
    threading.Thread(target=watch_parent, args=(server,), daemon=True).start()
else:
    print(f'브라우저에서 http://127.0.0.1:{args.port} 열기 / 종료: Ctrl+C')
with server:
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
