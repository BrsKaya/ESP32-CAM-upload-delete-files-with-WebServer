#include <SD_MMC.h>
#include <WiFi.h>
#include <WebServer.h>


//Setting the static ip address you specified//
IPAddress local_IP(/*static ip address*/);   
//////////////////////////////////////////////


//Setting  your gateway IP address//
IPAddress gateway(/*gateway IP address*/);   
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8); // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional
///////////////////////////////////


//Configure the ethernet ssid and password that connected//
const char* ssid = "Your SSID"; 
const char* password = "Your PASSWORD"; 
//////////////////////////////////////////////////////////


//Required variables//
bool videoReceived = false ;
String videoName;
String folder;
int i = 0;
int k = 0;


//Setting 80, 81, 82 ports//
WebServer server1(80);    // Port to upload file
WebServer server2(81);    // Port to download file
WebServer server3(82);    // Port to list and delete files


//Function for uploading files to the SD card of ESP32 CAM//
void handleFileUpload() {
  static File uploadFile;
  HTTPUpload& upload = server1.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    Serial.printf("Download started: %s\n", filename.c_str());
    uploadFile = SD_MMC.open(filename, FILE_WRITE);
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
    Serial.printf("Writing the download: %d byte\n", bytesWritten);
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    uploadFile.close();
    Serial.printf("Upload complete, size %d\n", upload.totalSize);
    videoReceived = true;
    videoName = upload.filename;
    i = 0;
  }
}

//The HTML format required for port 80 to select and upload files//
void handleRoot() {
  server1.send(200, "text/html", "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='file'><input type='submit'></form>");
}


//Function for listing files in the ESP32 CAM's SD card//
void handleFileListDownload() {
  String filelist = "<h2>Files on SD card:</h2><br>";
  File root = SD_MMC.open("/");
  File file = root.openNextFile();

  while (file) {
    if (!file.isDirectory()) {
      filelist += "<a href='/download?filename=" + String(file.name()) + "'>" + String(file.name()) + "</a> <br>";
    }
    file = root.openNextFile();
  }

  server2.send(200, "text/html", filelist);
}


//Function for downloading files in the ESP32 CAM's SD card//
void handleFileDownload() {
  File file = SD_MMC.open("/" + server2.arg("filename"), FILE_READ);

  if (file) {
    server2.sendHeader("Content-Disposition", "attachment; filename=\"" + server2.arg("filename") + "\"");
    server2.sendHeader("Content-Type", "application/octet-stream");
    server2.sendHeader("Content-Length", String(file.size()));
    server2.streamFile(file, "application/octet-stream");
    file.close();
  } else {
    server2.send(404, "text/plain", "File not found");
  }
}


//Function for listing files in the ESP32 CAM's SD card //
void handleFileList() {
  String filelist = "<h2>Files on SD card:</h2><br>";
  File root = SD_MMC.open("/");
  File file = root.openNextFile();

  while (file) {
    if (!file.isDirectory()) {
      filelist += "<form method='POST' action='/delete' style='display:inline;'><input type='hidden' name='filename' value='" + String(file.name()) + "'><button type='submit'>Delete</button></form> " + String(file.name()) + "<br>";
    }
    file = root.openNextFile();
  }

  server3.send(200, "text/html", filelist);
}


// Function for deleting files in the ESP32 CAM's SD card//
void handleFileDelete() {
  String filename = "/" + server3.arg("filename");
  if (SD_MMC.remove(filename)) {
    server3.send(200, "text/plain", "File deleted");
  } else {
    server3.send(500, "text/plain", "Error deleting file");
  }
}


void setup() {
  Serial.begin(115200);

  if(!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {  
    Serial.println("STA error.");
  }
  
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);       
   
  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }

  Serial.println("Connected to wifi network.");
  Serial.println("ESP32 CAM IP: ");
  Serial.println(WiFi.localIP());
  
  if(!SD_MMC.begin()){
    Serial.println("Failed to initialize SD card.");
  }
  
  Serial.println("SD card connected.");
  
  server1.on("/", HTTP_GET, handleRoot);
  server1.on("/upload", HTTP_POST, []() {
    server1.send(200, "text/plain", "File uploaded.");
  }, handleFileUpload);
  server1.begin();
  Serial.println("Port 80 opened.");

  server2.on("/download", HTTP_GET, handleFileDownload);
  server2.on("/", HTTP_GET, handleFileListDownload);
  server2.begin();
  Serial.println("Port 81 opened.");

  server3.on("/", HTTP_GET, handleFileList);
  server3.on("/delete", HTTP_POST, handleFileDelete);
  server3.begin();
  Serial.println("Port 82 opened.");
}

void loop() {
  server1.handleClient();
  server2.handleClient();
  server3.handleClient();
}
