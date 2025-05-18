import subprocess
import requests
import time
import os
import sys
import threading

SERVER_BIN = "../webserv"
CONFIG_FILE = "../config_file/default.conf"
HOST = "http://127.0.0.1:8080"
UPLOAD_PATH = "../www/upload"
TEST_DIR = "./test_files"

def print_stream(stream, prefix):
    for line in iter(stream.readline, ''):
        if line:
            print(f"{prefix} {line.strip()}")

def wait_for_server(url, timeout=10):
    print(f"Waiting for server at {url} for up to {timeout} seconds...")
    start = time.time()
    while time.time() - start < timeout:
        try:
            r = requests.get(url)
            print(f"Server responded with status {r.status_code}")
            if r.status_code in (200, 404):
                return True
        except requests.exceptions.ConnectionError:
            print("Connection refused, retrying...")
        time.sleep(0.5)
    print("Timeout waiting for server!")
    return False

def main():
    print("Starting test script...")

    # Create upload and test directories if missing
    os.makedirs(UPLOAD_PATH, exist_ok=True)
    os.makedirs(TEST_DIR, exist_ok=True)

    # Start server subprocess
    server_proc = subprocess.Popen(
        [SERVER_BIN, CONFIG_FILE],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True  # decode output as text
    )

    # Start threads to print server stdout and stderr live
    threading.Thread(target=print_stream, args=(server_proc.stdout, "[server stdout]"), daemon=True).start()
    threading.Thread(target=print_stream, args=(server_proc.stderr, "[server stderr]"), daemon=True).start()

    if not wait_for_server(HOST):
        print("Error: Server did not start in time")
        server_proc.terminate()
        sys.exit(1)

    print("Server started!")

    try:
           # Test 1: GET /
        r = requests.get(f"{HOST}/")
        assert r.status_code == 200, f"GET / expected 200, got {r.status_code}"
        print("✅ Test 1: GET / passed")

        # Test 2: GET static file (create file first)
        index_path = "../www/index.html"
        with open(index_path, "w") as f:
            f.write("Hello Static File")

        r = requests.get(f"{HOST}/index.html")
        assert r.status_code == 200, f"GET /index.html expected 200, got {r.status_code}"
        assert "Hello Static File" in r.text, "Static file content mismatch"
        print("✅ Test 2: GET /index.html passed")

        # Test 3: GET directory listing /upload/ (expect 200 or 403)
        r = requests.get(f"{HOST}/upload/")
        assert r.status_code in (200, 403), f"GET /upload/ expected 200 or 403, got {r.status_code}"
        print("✅ Test 3: GET /upload/ passed")

        # Test 4: GET non-existent file -> 404
        r = requests.get(f"{HOST}/no_such_file.txt")
        assert r.status_code == 404, f"GET non-existent file expected 404, got {r.status_code}"
        print("✅ Test 4: GET non-existent file passed")

        # Test 5: POST upload a file
        upload_file = os.path.join(TEST_DIR, "upload_test.txt")
        content = b"This is a file upload test"
        with open(upload_file, "wb") as f:
            f.write(content)

        with open(upload_file, "rb") as f:
            r = requests.post(f"{HOST}/upload/upload_test.txt", data=f)
        assert r.status_code == 200, f"POST upload expected 200, got {r.status_code}"
        print("✅ Test 5: POST upload file passed")

        # Test 6: Check uploaded file exists and matches content
        uploaded_file = os.path.join(UPLOAD_PATH, "upload_test.txt")
        assert os.path.isfile(uploaded_file), "Uploaded file missing"
        with open(uploaded_file, "rb") as f:
            saved_content = f.read()
        assert saved_content == content, "Uploaded file content mismatch"
        print("✅ Test 6: Uploaded file content verified")

        # Test 7: DELETE existing file
        r = requests.delete(f"{HOST}/upload/upload_test.txt")
        assert r.status_code in (200, 204), f"DELETE existing file expected 200/204, got {r.status_code}"
        print("✅ Test 7: DELETE existing file passed")

        # Test 8: DELETE non-existent file (expect 404 or 204)
        r = requests.delete(f"{HOST}/upload/file_does_not_exist.txt")
        assert r.status_code in (404, 204), f"DELETE non-existent file expected 404/204, got {r.status_code}"
        print("✅ Test 8: DELETE non-existent file passed")

        # Test 9: POST large file upload (1MB)
        large_file = os.path.join(TEST_DIR, "large_upload.txt")
        with open(large_file, "wb") as f:
            f.write(os.urandom(1_000_000))

        with open(large_file, "rb") as f:
            r = requests.post(f"{HOST}/upload/large_file.txt", data=f)
        assert r.status_code in (200, 413), f"POST large file expected 200/413, got {r.status_code}"
        print("✅ Test 9: POST large file upload passed")

        # Test 10: HEAD request returns headers only
        r = requests.head(f"{HOST}/")
        assert r.status_code == 405, f"HEAD / expected 405, got {r.status_code}"
        print("✅ Test 10: HEAD request passed")

        # Test 11: Unsupported method PUT returns 405
        r = requests.put(f"{HOST}/")
        assert r.status_code == 405, f"PUT / expected 405, got {r.status_code}"
        print("✅ Test 11: PUT method returns 405")

        print("\nAll tests passed successfully!")
    except AssertionError as e:
        print(f"❌ Test failed: {e}")
    finally:
        print("Stopping server...")
        server_proc.terminate()
        try:
            server_proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            server_proc.kill()
        print("Server stopped.")

if __name__ == "__main__":
    main()
