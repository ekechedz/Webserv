# Webserv

**Webserv** is a lightweight HTTP server written in C++98, built from scratch as part of the 42 school curriculum.

## 🚀 Features

- HTTP/1.1 support
- Configurable via configuration file (inspired by NGINX)
- Non-blocking I/O using `poll()` (or equivalent)
- Static file serving
- Default error pages
- Supports GET, POST, and DELETE
- CGI support (e.g., PHP, Python)
- File uploads
- Directory listing and default index files
- Multiple server blocks and ports

## ⚙️ Usage

```bash
make
./webserv [config_file]
```

## 📝 Configuration

A configuration file allows you to define:

- Host and port
- Server names
- Routes and HTTP methods
- Root directories and index files
- Upload directories
- CGI handling
- Redirections
- Error pages

## 🛠 Status

This project is functional but **not production-ready**. The codebase is evolving, and changes may occur as development continues.

## 📎 Requirements

- C++98
- No external libraries (including Boost)
- Compatible with macOS and Linux

## 📄 License

This project is developed as part of 42 school and is intended for educational purposes.
