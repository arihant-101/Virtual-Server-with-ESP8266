#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// Replace with your network credentials
const char* ssid = "Jesus";           // Your Wi-Fi SSID
const char* password = "Julius@521";  // Your Wi-Fi password

// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);

Dir dir = LittleFS.openDir("/");
while (dir.next()) {
  Serial.print("finding files");
  Serial.print("File found: ");
  Serial.println(dir.fileName());}

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("An error occurred while mounting LittleFS");
    return;
  }
  Serial.println("LittleFS mounted successfully");

  // Serve static files from LittleFS with default file as "index.html"
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // File upload handler
  server.on(
    "/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200);
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      static File uploadFile;

      if (!index) {  // Start of the file upload
        Serial.printf("Upload Start: %s\n", filename.c_str());
        uploadFile = LittleFS.open("/" + filename, "w");
        if (!uploadFile) {
          Serial.println("Failed to open file for writing");
          request->send(500, "text/plain", "Failed to open file for writing");
          return;
        }
      }

      if (uploadFile) {  // Write data to the file
        if (uploadFile.write(data, len) != len) {
          Serial.println("Write failed");
          request->send(500, "text/plain", "Write failed");
          return;
        }
      }

      if (final) {  // End of file upload
        Serial.printf("Upload Complete: %s\n", filename.c_str());
        uploadFile.close();
        request->send(200, "text/plain", "File uploaded successfully");
      }
    }
  );

  // List all files endpoint
  server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request){
    String filesList = "<h1>Uploaded Files:</h1><ul>";
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      filesList += "<li><a href='/download?file=" + fileName + "'>" + fileName + "</a></li>";
    }
    filesList += "</ul>";
    request->send(200, "text/html", filesList);
  });

server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    String fileName = request->getParam("file")->value();  // Get the file name from the URL parameter
    File file = LittleFS.open("/" + fileName, "r");

    if (!file) {
      request->send(404, "text/plain", "File not found");
      return;
    }

    // Set the appropriate content type based on file extension
    String contentType = "application/octet-stream"; // Default for binary file downloads
    if (fileName.endsWith(".html")) contentType = "text/html";
    else if (fileName.endsWith(".css")) contentType = "text/css";
    else if (fileName.endsWith(".js")) contentType = "application/javascript";

    // Create an AsyncWebServerResponse object to send the file
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/" + fileName, contentType);
    
    // Set headers for file download
    response->addHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
    
    // Send the response
    request->send(response);

    // Close the file
    file.close();
});

  // Start the server
  server.begin();
  Serial.println("Server started!");
}

void loop() {
  // Nothing needed here for AsyncWebServer
}
