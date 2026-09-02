#!/usr/bin/env python3
"""
Lightweight HTTP Server for Toy Allocator Visualizer.
Zero external dependencies (uses Python standard library).
Serves static frontend files and provides endpoints to read events and replay/run demo.
"""

import http.server
import json
import mimetypes
import os
import shutil
import socketserver
import subprocess
import sys
from urllib.parse import parse_qs, urlparse

DEFAULT_PORT = 5000
LOG_FILE = os.getenv("TOY_LOG_FILE", "allocator_events.jsonl")
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
STATIC_DIR = os.path.join(BASE_DIR, "static")

def read_events_log(filepath=LOG_FILE):
    full_path = os.path.join(BASE_DIR, filepath)
    if not os.path.exists(full_path):
        return []
    events = []
    try:
        with open(full_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line:
                    try:
                        events.append(json.loads(line))
                    except json.JSONDecodeError:
                        continue
    except Exception as e:
        print(f"Error reading log file {full_path}: {e}", file=sys.stderr)
    return events

def run_demo_program():
    """Compiles and executes the demo binary to regenerate the event log."""
    is_windows = sys.platform.startswith("win")
    
    # Check whether to invoke via wsl or directly
    if is_windows:
        compile_cmd = ["wsl", "gcc", "-DTOY_VISUALIZE", "-Wall", "-Wextra", "-o", "demo", "demo.c", "toy_allocator.c", "toy_logger.c"]
        run_cmd = ["wsl", "./demo"]
    else:
        compile_cmd = ["gcc", "-DTOY_VISUALIZE", "-Wall", "-Wextra", "-o", "demo", "demo.c", "toy_allocator.c", "toy_logger.c"]
        run_cmd = ["./demo"]

    try:
        c_proc = subprocess.run(compile_cmd, cwd=BASE_DIR, capture_output=True, text=True, check=False)
        if c_proc.returncode != 0:
            return False, f"Compilation failed:\n{c_proc.stderr}"

        r_proc = subprocess.run(run_cmd, cwd=BASE_DIR, capture_output=True, text=True, check=False)
        if r_proc.returncode != 0:
            return False, f"Execution failed:\n{r_proc.stderr}"

        return True, r_proc.stdout
    except Exception as e:
        return False, f"Failed to run demo: {str(e)}"

class AllocatorHandler(http.server.BaseHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_HEAD(self):
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path == "/api/events":
            events = read_events_log()
            payload = json.dumps({"events": events, "count": len(events)}).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if path == "/api/status":
            full_path = os.path.join(BASE_DIR, LOG_FILE)
            exists = os.path.exists(full_path)
            size = os.path.getsize(full_path) if exists else 0
            payload = json.dumps({
                "log_file": LOG_FILE,
                "exists": exists,
                "file_size": size,
                "cwd": BASE_DIR
            }).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        # Serve static files
        req_path = path.lstrip("/")
        if not req_path or req_path == "index.html":
            req_path = "index.html"

        file_path = os.path.join(STATIC_DIR, req_path)
        if os.path.exists(file_path) and os.path.isfile(file_path):
            mime_type, _ = mimetypes.guess_type(file_path)
            if not mime_type:
                mime_type = "application/octet-stream"

            try:
                with open(file_path, "rb") as f:
                    content = f.read()
                self.send_response(200)
                self.send_header("Content-Type", f"{mime_type}; charset=utf-8" if "text" in mime_type or "json" in mime_type or "javascript" in mime_type else mime_type)
                self.send_header("Content-Length", str(len(content)))
                self.end_headers()
                self.wfile.write(content)
                return
            except Exception as e:
                self.send_error(500, f"Error reading file: {e}")
                return

        self.send_error(404, "File Not Found")

    def do_POST(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path == "/api/run-demo":
            success, output = run_demo_program()
            events = read_events_log() if success else []
            payload = json.dumps({
                "success": success,
                "output": output,
                "events": events,
                "count": len(events)
            }).encode("utf-8")

            self.send_response(200 if success else 500)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        self.send_error(404, "Unknown endpoint")

def find_available_port(start_port=DEFAULT_PORT):
    import socket
    port = start_port
    while port < start_port + 50:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            if s.connect_ex(("127.0.0.1", port)) != 0:
                return port
            port += 1
    return start_port

def run_server(port=None):
    if port is None:
        port = int(os.getenv("PORT", DEFAULT_PORT))
    
    port = find_available_port(port)

    class ReusableTCPServer(socketserver.TCPServer):
        allow_reuse_address = True

    with ReusableTCPServer(("0.0.0.0", port), AllocatorHandler) as httpd:
        print(f"===========================================================")
        print(f"  Toy Allocator Visualizer Server running at:")
        print(f"  --> http://localhost:{port}")
        print(f"===========================================================")
        sys.stdout.flush()
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server.")
            httpd.shutdown()

if __name__ == "__main__":
    p = int(sys.argv[1]) if len(sys.argv) > 1 else None
    run_server(p)
