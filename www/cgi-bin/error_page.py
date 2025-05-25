import requests

BASE_URL = "http://127.0.0.1:8080"

def test_404():
    r = requests.get(f"{BASE_URL}/no_such_file.html")
    assert r.status_code == 404, f"Expected 404, got {r.status_code}"
    print("✅ 404 Not Found test passed")

def test_500():
    r = requests.get(f"{BASE_URL}/cgi-bin/slow_cgi.py")
    assert r.status_code == 500, f"Expected 500, got {r.status_code}"
    print("✅ 500 Internal Server Error test passed")

def test_403():
    r = requests.get(f"{BASE_URL}/upload/")
    assert r.status_code == 403, f"Expected 403, got {r.status_code}"
    print("✅ 403 Forbidden test passed")

def test_405():
    r = requests.put(f"{BASE_URL}/")
    assert r.status_code == 405, f"Expected 405, got {r.status_code}"
    print("✅ 405 Method Not Allowed test passed")

if __name__ == "__main__":
    test_404()
    test_500()
    test_403()
    test_405()
