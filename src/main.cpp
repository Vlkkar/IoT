
#include <mbed.h>
#include "TCPSocket.h"
#include "MQTTmbed.h"
#include "MQTTClientMbedOs.h"
#include "OPT3001.h"


DigitalOut led(LED1);
OPT3001 sensor_opt(I2C_SDA, I2C_SCL);
const int MAXLUM = 5000;        // Максимальная освещенность
const int luminocity_sp = 500;  // Желаемый уровень освещенности
const float K = 0.1;            // Коэффициент пропорциональности
   
PwmOut green(D9); 
PwmOut blue(D5); 
PwmOut red(D3);        

WiFiInterface *wifi;

char* buffer = 0;
float brightness = 0.0f;    


void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;

    buffer = new char[message.payloadlen + 1];
    memcpy(buffer, message.payload, message.payloadlen);
    buffer[message.payloadlen] = '\0';

}


const char *sec2str(nsapi_security_t sec)
{
    switch (sec) {
        case NSAPI_SECURITY_NONE:
            return "None";
        case NSAPI_SECURITY_WEP:
            return "WEP";
        case NSAPI_SECURITY_WPA:
            return "WPA";
        case NSAPI_SECURITY_WPA2:
            return "WPA2";
        case NSAPI_SECURITY_WPA_WPA2:
            return "WPA/WPA2";
        case NSAPI_SECURITY_UNKNOWN:
        default:
            return "Unknown";
    }
}


void mqtt_demo(NetworkInterface *net)
{
    
    char* topic = "base/relay/led1";
    char* topiccommand = "base/relay/command";
 
    TCPSocket socket;
    MQTTClient client(&socket);
 
    SocketAddress a;
    char* hostname = "dev.rightech.io";
    net->gethostbyname(hostname, &a);
    int port = 1883;
    a.set_port(port);
 
    printf("Connecting to %s:%d\r\n", hostname, port);
 
    socket.open(net);
    printf("Opened socket\n\r");
    int rc = socket.connect(a);
    if (rc != 0)
        printf("rc from TCP connect is %d\r\n", rc);
    printf("Connected socket\n\r");
 
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "8f226dcea860406e92a6888b9ee74fb0";
    data.username.cstring = "kikot";
    data.password.cstring = "2000";
    if ((rc = client.connect(data)) != 0)
        printf("rc from MQTT connect is %d\r\n", rc);
 
    if ((rc = client.subscribe(topiccommand, MQTT::QOS2, messageArrived)) != 0)
        printf("rc from MQTT subscribe is %d\r\n", rc);
 
    MQTT::Message message;

    OPT3001 sensor_opt(I2C_SDA, I2C_SCL);

    
    char buf[100];

    
    message.retained = false;
    message.dup = false;
    message.qos = MQTT::QOS1;
     
    while(true){
    sprintf(buf, "%d", sensor_opt.readSensor());
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish(topic, message);  
    client.yield();
    string ste = "ON", redword = "red", blueword="blue";          
    if (buffer == ste || buffer == redword || buffer == blueword) 
        {
            int luminocity = sensor_opt.readSensor();
            brightness += (K * (luminocity_sp - luminocity) / MAXLUM);
            brightness = (brightness < 0.0f)? 0.0f : (brightness > 1.0f)? 1.0f : brightness;
            green=brightness;
            if (buffer == ste){
                blue=brightness; 
                red=brightness;
                
            }
            if (buffer == redword){
                blue=0; 
                red=brightness;
                
            }
            if (buffer == blueword){
                blue=brightness; 
                red=0;
                
            }
        }
    else {
        blue=0;
        red=0;
        };
    }  
 
    
    if ((rc = client.unsubscribe(topic)) != 0)
        printf("rc from unsubscribe was %d\r\n", rc);
 
    if ((rc = client.disconnect()) != 0)
        printf("rc from disconnect was %d\r\n", rc);

 
    return;
}


int main()
{


    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
        printf("ERROR: No WiFiInterface found.\n");
        return -1;
    }

    printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        printf("\nConnection error: %d\n", ret);
        return -1;
    }

    mqtt_demo(wifi);

}

