# HTTP Webserver Architecture and Request Flow
> Visual Guide to Request Processing and File Upload Handling

## 🔄 Complete Request Processing Flow

```
                   ┌─────────────────────────────────────┐
                   │            Client Request           │
                   │  curl -X POST -d "Hello, world!"    │
                   │  http://127.0.0.1:8080/upload/file  │
                   └─────────────────┬───────────────────┘
                                     │
                                     ▼
┌────────────────────────────────────────────────────────────────────────────┐
│                             TCP/IP Network Layer                            │
└─────────────────────────────────────┬──────────────────────────────────────┘
                                     │
                                     ▼
┌────────────────────────────────────────────────────────────────────────────┐
│                              Server::run()                                  │
│                                                                            │
│  while (true) {                                                            │
│      int ret = poll(_pollFds.data(), _pollFds.size(), 5000);               │
│      // Process active socket events                                       │
│      for (size_t i = 0; i < _pollFds.size(); ++i) {                        │
│          if (_pollFds[i].revents & POLLIN) {                               │
│              int fd = _pollFds[i].fd;                                      │
│              if (_sockets[fd].getType() == Socket::LISTENING)              │
│                  acceptConnection(_sockets[fd]);                           │
│              else                                                          │
│                  handleClient(_sockets[fd]);                               │
│          }                                                                 │
│      }                                                                     │
│  }                                                                         │
└────────────────────────────────────┬───────────────────────────────────────┘
                                     │
                                     ▼
┌────────────────────────────────────────────────────────────────────────────┐
│                        Server::acceptConnection()                           │
│                                                                            │
│  // Check if we can accept more clients                                    │
│  if (currentClients >= MAX_SOCKETS) {                                      │
│      // Reject with 503 Service Unavailable                                │
│      return;                                                               │
│  }                                                                         │
│                                                                            │
│  // Accept the connection                                                  │
│  int clientFd = accept(listeningSocket.getFd(), ...);                      │
│                                                                            │
│  // Configure the socket as non-blocking                                   │
│  fcntl(clientFd, F_SETFL, O_NONBLOCK);                                     │
│                                                                            │
│  // Add to poll array and socket map                                       │
│  pollfd pfd = {clientFd, POLLIN, 0};                                       │
│  _pollFds.push_back(pfd);                                                  │
│  _sockets[clientFd] = Socket(clientFd, Socket::CLIENT, ...);               │
└────────────────────────────────────┬───────────────────────────────────────┘
                                     │
                                     ▼
┌────────────────────────────────────────────────────────────────────────────┐
│                          Server::handleClient()                             │
│                                                                            │
│  // Read data from socket                                                  │
│  char buffer[10000];                                                       │
│  ssize_t bytes = recv(client.getFd(), buffer, sizeof(buffer) - 1, 0);      │
│                                                                            │
│  // Process the HTTP request                                               │
│  buffer[bytes] = '\0';                                                     │
│  client.appendToBuffer(buffer);                                            │
│  std::string requestString = client.getBuffer();                           │
│                                                                            │
│  Request req;                                                              │
│  Response res;                                                             │
│  parseHttpRequest(requestString, req, res);                                │
│                                                                            │
│  // Get server and location configurations                                 │
│  ServerConfig *serverConfig = findExactServerConfig(...);                  │
│  matchLocation(req, locations);                                            │
│                                                                            │
│  // Route request based on method                                          │
│  std::string method = req.getMethod();                                     │
│  std::string path = req.getPath();                                         │
│  std::string body = req.getBody();                                         │
│                                                                            │
│  if (method == "GET")                                                      │
│      handleGetRequest(res, path);                                          │
│  else if (method == "POST")                                                │
│      handlePostRequest(res, path, body);                                   │
│  else if (method == "DELETE")                                              │
│      handleDeleteRequest(res, path);                                       │
│  else                                                                      │
│      res.setStatus(405);                                                   │
│                                                                            │
│  makeReadyforSend(res, client);                                                │
│  client.clearBuffer();                                                     │
└───────────────┬────────────────────────────────────────┬───────────────────┘
                │                                        │
                ▼                                        ▼
┌───────────────────────────────┐    ┌───────────────────────────────────────┐
│     parseHttpRequest()        │    │        matchLocation()                │
│                               │    │                                       │
│ // Parse first line           │    │ // Find best matching location config │
│ POST /upload/file HTTP/1.1    │    │ const LocationConfig *bestMatch = NULL│
│                               │    │ size_t longestMatch = 0;              │
│ // Extract method, path, etc. │    │                                       │
│ request.setMethod("POST");    │    │ for (size_t i = 0; i < locations.size│
│ request.setPath("/upload/file")    │     // Match path against locations   │
│ request.setProtocol("HTTP/1.1")    │     // Find longest matching prefix   │
│                               │    │                                       │
│ // Parse headers              │    │ // Set matched location in request    │
│ Content-Length: 13            │    │ req.setMatchedLocation(bestMatch);    │
│ Host: 127.0.0.1:8080          │    │                                       │
│ ...                           │    └───────────────────────────────────────┘
│                               │
│ // Extract body based on      │
│ // Content-Length header      │
│ std::string body(length, '\0')│
│ stream.read(&body[0], length);│
│ request.setBody(body);        │
└──────────────┬────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                       Server::handlePostRequest()                         │
│                                                                          │
│ // For our example: path = "/upload/file", body = "Hello, world!"        │
│                                                                          │
│ // Construct full file path                                              │
│ std::string fullPath = "www" + path;  // "www/upload/file"               │
│                                                                          │
│ // Open file for writing                                                 │
│ std::ofstream outFile(fullPath.c_str());                                 │
│ if (!outFile.is_open()) {                                                │
│     // Handle error - return 500                                         │
│     res.setStatus(500);                                                  │
│     // ... set error message ...                                         │
│     return;                                                              │
│ }                                                                        │
│                                                                          │
│ // Write body to file                                                    │
│ outFile << requestBody;  // Writes "Hello, world!"                       │
│ outFile.close();                                                         │
│                                                                          │
│ // Create success response                                               │
│ std::string body = "<html><body><h1>POST Received</h1>...</html>";       │
│ res.setStatus(200);                                                      │
│ res.setHeader("Content-Type", "text/html");                              │
│ res.setHeader("Content-Length", intToStr(body.size()));                  │
│ res.setBody(body);                                                       │
└──────────────────────────────────┬───────────────────────────────────────┘
                                   │
                                   ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                        Server::makeReadyforSend()                             │
│                                                                          │
│ // Convert Response object to string                                     │
│ std::string responseString = response.toString();                        │
│                                                                          │
│ // Format: "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n..."          │
│                                                                          │
│ // Send the response back to client                                      │
│ ssize_t sent = send(client.getFd(), responseString.c_str(),             │
│                    responseString.size(), 0);                            │
│                                                                          │
│ // Close connection if needed                                            │
│ if (response.getHeaderValue("Connection") == "close")                    │
│     deleteClient(client);                                                │
└──────────────────────────────────┬───────────────────────────────────────┘
                                   │
                                   ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                          Client Receives Response                         │
│                                                                          │
│ HTTP/1.1 200 OK                                                          │
│ Content-Type: text/html                                                  │
│ Content-Length: 98                                                       │
│ Connection: keep-alive                                                   │
│                                                                          │
│ <html><body>                                                             │
│ <h1>POST Received</h1>                                                   │
│ <br>                                                                     │
│ <p>Path: /upload/file</p>                                                │
│ </body></html>                                                           │
└──────────────────────────────────────────────────────────────────────────┘
```

## 📦 Key Classes and Their Relationships

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│                                  WebServer Core                                    │
└───────────────────────────────────────────────────────────────────────────────────┘
     │
     ├─────────────┐
     │             │
     ▼             ▼
┌────────────┐ ┌──────────────┐     
│  Server    │ │ ConfigParser │     
└─────┬──────┘ └──────────────┘     
      │                             
      ├─────────────┐               
      │             │               
      ▼             ▼               
┌────────────┐ ┌──────────────┐     
│  Socket    │ │ ServerConfig │     
└────────────┘ └───────┬──────┘     
                       │            
                       ▼            
                ┌──────────────┐    
                │LocationConfig│    
                └──────────────┘    

┌───────────────────────────────────────────────────────────────────────────────────┐
│                               HTTP Request/Response                                │
└───────────────────────────────────────────────────────────────────────────────────┘
      ┌────────────┐          ┌──────────────┐
      │  Request   │◄────────►│   Response   │
      └────────────┘          └──────────────┘

┌───────────────────────────────────────────────────────────────────────────────────┐
│                                Special Handlers                                    │
└───────────────────────────────────────────────────────────────────────────────────┘
      ┌────────────┐          ┌──────────────┐
      │ CGIHandler │          │    Logger    │
      └────────────┘          └──────────────┘
```

## 🔄 State Machine: Socket Lifecycle

```
                         ┌───────────────┐
                         │   LISTENING   │
                         └───────┬───────┘
                                 │ accept()
                                 ▼
┌────────────┐           ┌───────────────┐
│ Connection │◄──────────┤   RECEIVING   │◄───────────┐
│   Closed   │           └───────┬───────┘            │
└────────────┘                   │ recv() data        │
      ▲                          ▼                    │
      │                  ┌───────────────┐            │
      │                  │Process Request│            │
      │                  └───────┬───────┘            │
      │                          │                    │
      │                          ▼                    │
      │                  ┌───────────────┐            │
      │                  │    SENDING    │            │
      │                  └───────┬───────┘            │
      │                          │                    │
      │          ┌───────────────┴─────────┐          │
      │          │                         │          │
      │          ▼                         ▼          │
┌─────┴──────┐                    ┌───────────────┐   │
│ Connection │                    │   Keep-Alive  │───┘
│   Close    │                    │               │
└────────────┘                    └───────────────┘
```

## 📋 Request Parsing Flow

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│                              HTTP Request Format                                   │
└───────────────────────────────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────────────────────────────────┐
│ POST /upload/file.txt HTTP/1.1                                                     │
│ Host: 127.0.0.1:8080                                                               │
│ Content-Type: application/x-www-form-urlencoded                                    │
│ Content-Length: 13                                                                 │
│                                                                                    │
│ Hello, world!                                                                      │
└───────────────────────────────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────────────────────────────────┐
│                              Parsing Stages                                        │
└───────────────────────────────────────────────────────────────────────────────────┘

┌───────────────────┐     ┌───────────────────┐     ┌───────────────────┐
│  Request Line     │     │  Headers          │     │  Body             │
│                   │     │                   │     │                   │
│ 1. Extract method │     │ 1. Parse each line│     │ 1. Check encoding │
│ 2. Extract path   │     │ 2. Find colon     │     │ 2. If chunked:    │
│ 3. Extract version│     │ 3. Split key/value│     │    decode chunks  │
└─────────┬─────────┘     └─────────┬─────────┘     │ 3. If Content-Len:│
          │                         │               │    read N bytes   │
          ▼                         ▼               └─────────┬─────────┘
┌───────────────────┐     ┌───────────────────┐              │
│  Request.method   │     │  Request.headers  │              ▼
│  Request.path     │     │  - Content-Type   │     ┌───────────────────┐
│  Request.protocol │     │  - Content-Length │     │  Request.body     │
└───────────────────┘     └───────────────────┘     └───────────────────┘
```

## 🚀 POST Request: File Upload

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│                           File Upload Request Flow                                 │
└───────────────────────────────────────────────────────────────────────────────────┘

           Client                                           Server
             │                                                │
             │ POST /upload/file.txt HTTP/1.1                 │
             │ Host: 127.0.0.1:8080                          │
             │ Content-Length: 13                            │
             │                                               │
             │ Hello, world!                                 │
             │ ─────────────────────────────────────────────▶│
             │                                               │
             │                                               │ 1. Parse HTTP request
             │                                               │    - Extract method (POST)
             │                                               │    - Extract path (/upload/file.txt)
             │                                               │    - Extract body (Hello, world!)
             │                                               │
             │                                               │ 2. Process POST request
             │                                               │    - Construct path (www/upload/file.txt)
             │                                               │    - Open file for writing
             │                                               │    - Write "Hello, world!" to file
             │                                               │    - Close file
             │                                               │
             │                                               │ 3. Generate response
             │                                               │    - Set status code (200 OK)
             │                                               │    - Create HTML response
             │                                               │
             │ HTTP/1.1 200 OK                               │
             │ Content-Type: text/html                       │
             │ Content-Length: 98                            │
             │                                               │
             │ <html><body>                                  │
             │ <h1>POST Received</h1>                        │
             │ <p>Path: /upload/file.txt</p>                 │
             │ </body></html>                                │
             │ ◀─────────────────────────────────────────────│
             │                                               │
```

## 🧠 Core Logic: Method Handlers

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│                               handleGetRequest()                                   │
└───────────────────────────────────────────────────────────────────────────────────┘
  Input: path = "/index.html"
  
  1. Resolve path:
     fullPath = "www" + path = "www/index.html"
  
  2. Open and read file:
     std::ifstream file(fullPath.c_str(), std::ios::binary);
     if (!file.is_open()) {
         // Return 404 Not Found
     }
  
  3. Create response:
     res.setStatus(200);
     res.setHeader("Content-Type", getContentType(fullPath));
     res.setBody(content);


┌───────────────────────────────────────────────────────────────────────────────────┐
│                              handlePostRequest()                                   │
└───────────────────────────────────────────────────────────────────────────────────┘
  Input: path = "/upload/file.txt", body = "Hello, world!"
  
  1. Resolve path:
     fullPath = "www" + path = "www/upload/file.txt"
  
  2. Open file for writing:
     std::ofstream outFile(fullPath.c_str());
     if (!outFile.is_open()) {
         // Return 500 Server Error
     }
  
  3. Write data:
     outFile << requestBody;
     outFile.close();
  
  4. Create response:
     res.setStatus(200);
     res.setBody("<html><body><h1>POST Received</h1>...</html>");


┌───────────────────────────────────────────────────────────────────────────────────┐
│                             handleDeleteRequest()                                  │
└───────────────────────────────────────────────────────────────────────────────────┘
  Input: path = "/upload/file.txt"
  
  1. Resolve path:
     fullPath = "www" + path = "www/upload/file.txt"
  
  2. Delete file:
     if (std::remove(fullPath.c_str()) == 0) {
         // File deleted successfully
         res.setStatus(200);
     } else {
         // Handle errors (404, 403, 500)
     }
  
  3. Create response
```

## 📝 CGI Request Handling

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│                              handleCgiRequest()                                    │
└───────────────────────────────────────────────────────────────────────────────────┘

Client                                          Server
  │                                               │
  │ GET /cgi-bin/test.py HTTP/1.1                 │
  │ ─────────────────────────────────────────────▶│
  │                                               │
  │                                               │ 1. Check if CGI request:
  │                                               │    - Match extension (.py)
  │                                               │    - Verify location config
  │                                               │
  │                                               │ 2. Create CGI environment:
  │                                               │    - Set REQUEST_METHOD
  │                                               │    - Set PATH_INFO
  │                                               │    - Set QUERY_STRING
  │                                               │    - Set HTTP headers
  │                                               │
  │                                               │ 3. Execute CGI script:
  │                                               │    - Fork process
  │                                               │    - Execute interpreter
  │                                               │    - Pass request body
  │                                               │    - Capture output
  │                                               │
  │                                               │ 4. Process CGI output:
  │                                               │    - Parse headers
  │                                               │    - Set response status
  │                                               │    - Set response body
  │                                               │
  │ HTTP/1.1 200 OK                               │
  │ Content-Type: text/html                       │
  │                                               │
  │ <html><body>                                  │
  │ <h1>Hello from CGI!</h1>                      │
  │ </body></html>                                │
  │ ◀─────────────────────────────────────────────│
```

## 🔧 Error Handling

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│                              Error Response Status Codes                           │
└───────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│ 400 Bad Request │ │ 404 Not Found   │ │ 500 Server Error│ │ 405 Not Allowed │
│                 │ │                 │ │                 │ │                 │
│ - Invalid HTTP  │ │ - File doesn't  │ │ - Can't open    │ │ - Unsupported   │
│   request format│ │   exist         │ │   file          │ │   HTTP method   │
│ - Malformed     │ │ - Resource not  │ │ - CGI execution │ │                 │
│   headers       │ │   available     │ │   failed        │ │                 │
└─────────────────┘ └─────────────────┘ └─────────────────┘ └─────────────────┘
      │                   │                    │                    │
      │                   │                    │                    │
      ▼                   ▼                    ▼                    ▼
┌───────────────────────────────────────────────────────────────────────────────────┐
│                              Error Response Generation                             │
└───────────────────────────────────────────────────────────────────────────────────┘
  res.setStatus(errorCode);
  res.setHeader("Content-Type", "text/html");
  res.setBody("<html><body><h1>" + errorMessage + "</h1></body></html>");
```

## 📡 Server Configuration

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│                       Server Configuration Hierarchy                               │
└───────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────┐
│ ServerConfig            │
│                         │
│ - host: 127.0.0.1       │
│ - port: 8080            │
│ - serverName: example   │
│ - index: index.html     │
└────────────┬────────────┘
             │
             │ Contains
             ▼
┌─────────────────────────┐
│ LocationConfig          │
│                         │
│ - path: /upload/        │◄───── Matched when client
│ - root: www/            │       requests /upload/file.txt
│ - methods: GET,POST,DEL │
│ - redirect: ""          │
│ - cgiPath: ""           │
│ - cgiExt: ""            │
└─────────────────────────┘

┌─────────────────────────┐
│ LocationConfig          │
│                         │
│ - path: /cgi-bin/       │◄───── Matched when client
│ - root: www/            │       requests /cgi-bin/test.py
│ - methods: GET,POST     │
│ - redirect: ""          │
│ - cgiPath: /usr/bin/python3
│ - cgiExt: .py           │
└─────────────────────────┘
```

## 🚚 Transport Layer Management

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│                             Socket Management                                      │
└───────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────┐
│ Socket Map          │
│                     │
│ Key: Socket FD      │
│ Value: Socket Object│
└─────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│ Poll Array                                                       │
│                                                                 │
│ ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐ │
│ │ fd: 3   │  │ fd: 4   │  │ fd: 5   │  │ fd: 6   │  │ fd: 7   │ │
│ │ events: │  │ events: │  │ events: │  │ events: │  │ events: │ │
│ │ POLLIN  │  │ POLLIN  │  │ POLLIN  │  │ POLLIN  │  │ POLLIN  │ │
│ └─────────┘  └─────────┘  └─────────┘  └─────────┘  └─────────┘ │
└─────────────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────────────────────────────────┐
│                      Connection Management Features                                │
└───────────────────────────────────────────────────────────────────────────────────┘
- Non-blocking I/O (O_NONBLOCK)
- Connection limits (MAX_SOCKETS)
- Request limits per connection (MAX_REQUESTS)
- Connection timeouts (30 seconds)
- Keep-alive support (Connection: keep-alive)
```

## 📊 Performance Considerations

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│                            Scaling and Performance                                 │
└───────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────┐ ┌─────────────────────┐ ┌─────────────────────┐
│ Connection Limits   │ │ Request Processing  │ │ Resource Management │
│                     │ │                     │ │                     │
│ - MAX_SOCKETS       │ │ - Non-blocking I/O  │ │ - Client timeouts   │
│   prevents resource │ │ - poll() for        │ │ - Connection: close │
│   exhaustion        │ │   efficient event   │ │   for older clients │
│ - 503 response when │ │   handling          │ │ - Buffer clearing   │
│   server is full    │ │ - File descriptor   │ │ - Socket cleanup    │
└─────────────────────┘ │   management        │ └─────────────────────┘
                        └─────────────────────┘
```

## 🧪 Testing Strategy

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│                                Test Components                                     │
└───────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────┐ ┌─────────────────────┐ ┌─────────────────────┐
│ Core Functionality  │ │ Edge Cases          │ │ Performance Tests   │
│                     │ │                     │ │                     │
│ - GET requests      │ │ - Missing headers   │ │ - Max client test   │
│ - POST file uploads │ │ - Invalid paths     │ │ - Concurrent        │
│ - DELETE operations │ │ - Large uploads     │ │   connections       │
│ - CGI execution     │ │ - Bad requests      │ │ - Timeout handling  │
└─────────────────────┘ └─────────────────────┘ └─────────────────────┘

┌───────────────────────────────────────────────────────────────────────────────────┐
│                           File Upload Test Example                                 │
└───────────────────────────────────────────────────────────────────────────────────┘

1. Create test file:
   ```python
   upload_file = os.path.join(TEST_DIR, "upload_test.txt")
   content = b"This is a file upload test"
   with open(upload_file, "wb") as f:
       f.write(content)
   ```

2. Send POST request:
   ```python
   with open(upload_file, "rb") as f:
       r = requests.post(f"{HOST}/upload/upload_test.txt", data=f)
   ```

3. Verify response:
   ```python
   assert r.status_code == 200, f"POST upload expected 200, got {r.status_code}"
   ```

4. Check uploaded file:
   ```python
   uploaded_file = os.path.join(UPLOAD_PATH, "upload_test.txt")
   # Verify file exists and content matches
   ```
```
