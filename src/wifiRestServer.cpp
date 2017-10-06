#include "config.h"

#include <WifiClient.h>
#include "wifiRestServer.h"

Stream * current_serial=NULL;
// Custom function accessible by the API
int ledControl(String command) {
  
    if(current_serial)
      current_serial->println("REST function called. command: "+command);
    return 1;
}
  
  
WifiRestServer::WifiRestServer(const uint8_t listenport) 
: listenport(listenport)
  , server(listenport)
  , rest()
{
  current_serial=NULL;
}


void WifiRestServer::setup(Stream &serial)
{
  current_serial = &serial;

  // Init variables and expose them to REST API
  temperature = 24;
  humidity = 40;
  rest.variable("temperature",&temperature);
  rest.variable("humidity",&humidity);

  // Function to be exposed
  rest.function("led",ledControl);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("1");
  rest.set_name("esp8266");
  
  // Start the server
  server.begin();  
  serial.print(">RESTServer SETUP: started on port:"); serial.println(listenport);
}

bool WifiRestServer::process() 
{
  if (!server.hasClient()) 
    return false;

  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return false;
  }
  if(client.available())
  {
    rest.handle(client);  
    return true;
  } 

  return false; //TODO

}


void WifiRestServer::send(const String& s_msg)
{
  //TODO
}
