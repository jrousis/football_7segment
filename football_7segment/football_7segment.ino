#define INVERT_DISPLAY false 
#define DIGITS_DISPLAY 8

#include "IR_rousis_keys.h"

#include <IRrecv.h>
#include <IRutils.h>
#include <esp_now.h>
#include <esp32fota.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPUI.h>
#include "time.h"
#include <Rousis7segment.h>
//#include <ModbusRTU.h>
#include <EEPROM.h>
#include <string>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ds1302.h>
#include "Chrono.h"

#define OPEN_HOT_SPOT 39
#define LED 13
#define RXD2 16
#define TXD2 17
#define RXD1 19
#define TXD1 18
#define DIR 4
#define REG1 10
#define REG_TIME 10
#define REG_BRIGHTNESS 20
//#define EEP_BRIGHT_ADDRESS 0X100
#define BUTTON1   36 //pin D5
#define SLAVE_ID 0
#define SAMPLES_MAX 30
#define PHOTO_SAMPLE_TIME 3000
#define BUZZER 15
//----------------------------------------------------------------------------------
//Remote control buttons

#define INSTR_1 0xA151
#define INSTR_2 0xA152
#define INSTR_3 0xA153
#define INSTR_4 0xA154
#define INSTR_5 0xA155
#define INSTR_6 0xA156
#define INSTR_7 0xA157
#define INSTR_8 0xA158
//----------------------------------------------------------------------------------


const uint16_t kRecvPin = 35;
IRrecv irrecv(kRecvPin);
decode_results results;

//===========================================================================
//   WiFi now 
// MAC:38:18:2B:3C:EF:24
uint8_t broadcastAddress1[] = { 0x38, 0x18, 0x2B, 0x3C, 0xEF, 0x24 }; // MAC Address of the receiver
esp_err_t result;

typedef struct struct_packet {
    char time[9];
    uint8_t alarm;
    uint8_t id;
    uint16_t insrtuction;
    char device[16];
    char display[32];
    uint8_t buzzer_time;
    uint8_t brightness;
} struct_packet;

struct_packet display_packet;
struct_packet display_received;
esp_now_peer_info_t peerInfo;

// Call back when data is sent
void OnDataSent(const uint8_t* mac, esp_now_send_status_t status) {
	Serial.print("\r\nLast Packet Send Status:\t");
	Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
	memcpy(&display_received, data, sizeof(display_received));
	Serial.print("Last Packet Recv from: ");
	Serial.println(mac[0], HEX);
	Serial.print("Instruction: ");
	Serial.println(display_received.insrtuction, HEX);
	Serial.print("Brightness: ");
	Serial.println(display_received.brightness, HEX);
	Serial.print("Device: ");
	Serial.println(display_received.device);
	Serial.print("Display: ");
	Serial.println(display_received.display);
	Serial.print("Buzzer time: ");
	Serial.println(display_received.buzzer_time, HEX);
	Serial.print("Alarm: ");
	Serial.println(display_received.alarm, HEX);
	Serial.print("Time: ");
	Serial.println(display_received.time);
}

//===========================================================================

//  ----- EEProme addresses -----
//EEPROM data ADDRESSER
#define EEP_WIFI_SSID 0
#define EEP_WIFI_PASS 32
#define EEP_USER_LOGIN 128
#define EEP_USER_PASS 160
#define EEP_SENSOR 196
#define EEP_DEFAULT_LOGIN 197
#define EEP_DEFAULT_WiFi 198
#define EEP_PRIGHTNESS 199
#define EEP_ADDRESS 200
#define EEP_WEB_UPDATE 201
#define EEP_WIFINOW_EN 202
#define EEP_TEMPO 203
#define EEP_TIME_DISP 204
#define EEP_DATE_DISP 205
#define EEP_TEMP_DISP 206
#define EEP_HUMI_DISP 207
#define EEP_COUNTDOWN 208

const String soft_version = "2.0.6";
const char* soft_ID = "Rousis Systems LTD\nScoreboard_V: 2.0.6";
//============================================================================
// DS1302 RTC instance
Ds1302 rtc(4, 32, 2);

// GPIO where the DS18B20 is connected to
const int oneWireBus = 32;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

//ModbusRTU mb;
Rousis7segment myLED(DIGITS_DISPLAY, 33, 26, 27, 14);    // Uncomment if not using OE pin

const int Open_Hotspot = OPEN_HOT_SPOT;

static unsigned long oldTime = 0;
int i = 0;
char Display_buf[15];
uint8_t Score_host = 0;
uint8_t Score_guest = 0;
uint8_t Team_fouls_host = '0';
uint8_t Team_fouls_guest = '0';
uint8_t Period = '0';
uint8_t Brightness = 255;
uint8_t BrighT_Update_Count = 0;
uint8_t Clock_Updated_once = 0;
uint8_t Connect_status = 0;
uint8_t Show_IP = 0;
uint8_t Last_Update = 0xff;
String st;
String content;
String esid;
String epass = "";

uint8_t Sensor_on;
uint8_t Default_login;
int8_t samples_metter = -1;
uint16_t Timer_devide_photo = 0;
uint8_t IR_toggle;

uint16_t ButtonsTimer;
uint16_t ButtonsGame;
uint16_t photosensor;
uint16_t mainSlider;
uint16_t button1;
uint16_t status;
uint16_t scoreLabel;
uint16_t WiFiStatus;
// Potentiometer is connected to GPIO 34 (Analog ADC1_CH6) 
const int photoPin = 36;
// variable for storing the potentiometer value
int photoValue = 0;

// esp32fota esp32fota("<Type of Firme for this device>", <this version>, <validate signature>);
esp32FOTA esp32FOTA("esp32-fota-http", soft_version, false);
const char* manifest_url = "http://smart-q.eu/fota/football_7segment.json";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0; // 7200;
const int   daylightOffset_sec = 0; // 3600;

uint8_t buzzer_cnt = 0;
uint8_t flash_cnt = 0;
bool flash_on = false;
bool blink = false;
unsigned long photo_delay = 0;

#define COUNTDOWN_EN 1
#define COUNTDOWN_INIT_TIME 45 //45 Minutes
unsigned int compare_elapsed = 10000;
uint16_t countdown_time;
uint16_t offset_time = 0;
int minutes = 0;
int seconds = 0;
bool countdown_on = false;
bool game_on = false;
bool countdown_timeout = false;
//==================================================================================
// GUI settings 
//----------------------------------------------------------------------------------
#define SAMPLES_MAX 60
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;

const char* ssid = "Rousis";
const char* password = "rousis074520198";
const char* hostname = "espui";

uint16_t password_text, user_text;
uint16_t wifi_ssid_text, wifi_pass_text;

String stored_user, stored_password;
String stored_ssid, stored_pass;
uint8_t photo_samples[60];

uint16_t mainTime;
uint16_t tempoNumber;
uint16_t hostNumber;
uint16_t guestNumber;
uint16_t cntdownTime;
uint16_t liveTime;

Chrono chrono(Chrono::SECONDS);
//----------------------------------------------------------------------------------------
hw_timer_t* flash_timer = NULL;
portMUX_TYPE falshMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR FlashInt()
{
    portENTER_CRITICAL_ISR(&falshMux);

    if (flash_cnt)
    {
        if (flash_on)
        {
            // dmd.drawString(1, 0, "   > ", 5, GRAPHICS_NORMAL);
            flash_on = false;
        }
        else {
            // dmd.drawString(1, 0, Line1_buf, 5, GRAPHICS_NORMAL);
            flash_on = true;
        }
        flash_cnt--;
    }

    if (buzzer_cnt) {
        buzzer_cnt--;
    }
    else {
        digitalWrite(BUZZER, LOW);
    }

    //photo_sample();

    portEXIT_CRITICAL_ISR(&falshMux);
}
// ---------------------------------------------------------------------------------------
//  UI functions callback
void getTimeCallback(Control* sender, int type) {
    if (type == B_UP) {
        //reset the mainTime callback
        ESPUI.getControl(mainTime)->callback = generalCallback;
        //update the mainTime callback

        ESPUI.updateTime(mainTime);
    }

    /*if (type == B_UP) {
        Serial.print("Time set: ID: ");
        Serial.print(sender->id);
        Serial.print(", Value: ");
        Serial.println(sender->value);
        ESPUI.updateTime(mainTime);
    }
    else if (type == TM_VALUE) {
        Serial.print("Time_Value: ");
        Serial.println(sender->value);
    }*/
}

void numberCall(Control* sender, int type) {
    Serial.println("Instruction Nn:");
    Serial.println(sender->label);
    Serial.println("Type:");
    Serial.println(type);

}

void textCall(Control* sender, int type) {
    Serial.print("Text: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);
}

void slider(Control* sender, int type) {
    Serial.print("Slider: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);
    Brightness = (sender->value.toInt()) & 0xff;
    //dmd.setBrightness(Brightness);
    EEPROM.write(EEP_PRIGHTNESS, Brightness);
    EEPROM.commit();

    myLED.displayBrightness(Brightness);
    /*if (!mb.slave()) {
        mb.writeHreg(SLAVE_ID, REG_BRIGHTNESS, Brightness, cbWrite);
    }*/
}

void enterWifiDetailsCallback(Control* sender, int type) {
    if (type == B_UP) {
        if (sender->label == "WiFi")
        {
            Serial.println();
            Serial.print("Saving credentials to EPROM: ");
            Serial.print(ESPUI.getControl(wifi_ssid_text)->value);
            Serial.print(" - Size: ");
            Serial.println(sizeof(ESPUI.getControl(wifi_ssid_text)->value));
            /* Serial.println(ESPUI.getControl(wifi_pass_text)->value); */
            writeString(EEP_WIFI_SSID, ESPUI.getControl(wifi_ssid_text)->value, 32);
            writeString(EEP_WIFI_PASS, ESPUI.getControl(wifi_pass_text)->value, 64);
            //EEPROM.commit();

            ESPUI.getControl(status)->value = "Saved credentials to EEPROM<br>SSID: "
                + read_String(EEP_WIFI_SSID, 32)
                + "<br>Password: " + read_String(EEP_WIFI_PASS, 32);
            ESPUI.updateControl(status);

            Serial.println(read_String(EEP_WIFI_SSID, 32));
            Serial.println(read_String(EEP_WIFI_PASS, 32));
        }
        else if (sender->label == "User") {
            Serial.println();
            Serial.println("Saving User to EPROM: ");
            /*Serial.println(ESPUI.getControl(user_text)->value);
            Serial.println(ESPUI.getControl(password_text)->value);*/
            writeString(EEP_USER_LOGIN, ESPUI.getControl(user_text)->value, sizeof(ESPUI.getControl(user_text)->value));
            writeString(EEP_USER_PASS, ESPUI.getControl(password_text)->value, sizeof(ESPUI.getControl(password_text)->value));
            //EEPROM.commit();

            ESPUI.getControl(status)->value = "Saved login details to EEPROM<br>User name: "
                + read_String(EEP_USER_LOGIN, 32)
                + "<br>Password: " + read_String(EEP_USER_PASS, 32);
            ESPUI.updateControl(status);
            Serial.println(read_String(EEP_USER_LOGIN, 32));
            Serial.println(read_String(EEP_USER_PASS, 32));
        }


        /*Serial.println("Reset..");
        ESP.restart();*/
    }
}

void writeString(char add, String data, uint8_t length)
{
    int _size = length; // data.length();
    int i;
    for (i = 0; i < _size; i++)
    {
        if (data[i] == 0) { break; }
        EEPROM.write(add + i, data[i]);
    }
    EEPROM.write(add + i, '\0');   //Add termination null character for String Data
    EEPROM.commit();
}

String read_String(char add, uint8_t length)
{
    int i;
    char data[100]; //Max 100 Bytes
    int len = 0;
    unsigned char k;
    k = EEPROM.read(add);
    while (k != '\0' && len < length)   //Read until null character
    {
        k = EEPROM.read(add + len);
        data[len] = k;
        len++;
    }
    data[len] = '\0';
    return String(data);
}

void ReadWiFiCrententials(void) {
    //uint8_t DefaultWiFi = EEPROM.read(EEP_DEFAULT_WiFi);
    if (Default_login || digitalRead(OPEN_HOT_SPOT) == 0) { ///testing to default
        Serial.println("Load default WiFi Crententials..");
        unsigned int i;
        stored_ssid = "Rousis";  // "Zyxel_816E51"; // "rousis";
        stored_pass = "rousis074520198";  // "8GJ4B3GP"; // "rousis074520198";
        writeString(EEP_WIFI_SSID, stored_ssid, sizeof(stored_ssid));
        writeString(EEP_WIFI_PASS, stored_pass, sizeof(stored_pass));
        stored_user = "espboard";
        stored_password = "12345678";
        writeString(EEP_USER_LOGIN, stored_user, sizeof(stored_user));
        writeString(EEP_USER_PASS, stored_password, sizeof(stored_password));

        EEPROM.write(EEP_DEFAULT_WiFi, 0); //Erase the EEP_DEFAULT_WiFi
        EEPROM.write(EEP_DEFAULT_LOGIN, 0);
        EEPROM.commit();
    }
    else {
        stored_ssid = read_String(EEP_WIFI_SSID, 32);
        stored_pass = read_String(EEP_WIFI_PASS, 32);
    }
}

void Readuserdetails(void) {
    stored_user = read_String(EEP_USER_LOGIN, 32);
    stored_password = read_String(EEP_USER_PASS, 32);

    Serial.println();
    Serial.print("stored_user: ");
    Serial.println(stored_user);
    Serial.print("stored_password: ");
    Serial.println(stored_password);
    /*readStringFromEEPROM(stored_user, 128, 32);
    readStringFromEEPROM(stored_password, 160, 32);*/
}

void ReadSettingEeprom() {
    Serial.println("Rad Settings EEPROM: ");
    Sensor_on = EEPROM.read(EEP_SENSOR);
    if (Sensor_on > 1) { Sensor_on = 1; }
    Serial.print("Photo sensor= ");
    Serial.println(Sensor_on, HEX);
    Default_login = EEPROM.read(EEP_DEFAULT_LOGIN);
    Serial.print("Default login= ");
    Serial.println(Default_login, HEX);
    Brightness = EEPROM.read(EEP_PRIGHTNESS);
    Serial.print("Eprom Brightness= ");
    Serial.println(Brightness);
    Serial.print("Display Items: ");
    Serial.print(EEPROM.read(EEP_TIME_DISP)); Serial.print(" ");
    Serial.print(EEPROM.read(EEP_DATE_DISP)); Serial.print(" ");
    Serial.print(EEPROM.read(EEP_TEMP_DISP)); Serial.print(" ");
    Serial.print(EEPROM.read(EEP_HUMI_DISP)); Serial.println();
    Serial.println("-------------------------------");
}

void textCallback(Control* sender, int type) {
    //This callback is needed to handle the changed values, even though it doesn't do anything itself.
}

void ResetCallback(Control* sender, int type) {
    if (type == B_UP) {
        Serial.println("Reset..");
        ESP.restart();
    }
}

void ClearCallback(Control* sender, int type) {
    switch (type) {
    case B_DOWN:
        Serial.println("Button DOWN");
        setTime(2023, 7, 19, 12, 0, 0, 0);
        break;
    case B_UP:
        Serial.println("Button UP");
        break;
    }
}

void ScoreboardCallback(Control* sender, int type) {
    switch (type) {
    case B_DOWN:
        Serial.print("Button DOWN = ");
        Serial.println(sender->value);

        if (sender->value == "Sart" && game_on)
        {
            chrono.resume();
			//Send Start instruction to the shoot clock
			Serial1.write(0XA1); Serial1.write(0X53);

            buzzer_ring(1);
        }
        else if (sender->value == "Stop")
		{
			chrono.stop();
            //Send Stop instruction to the shoot clock
            Serial1.write(0XA1); Serial1.write(0X54);
            buzzer_ring(1);
		}
		else if (sender->value == "Up cunter")
		{
            compare_elapsed = -1;
            offset_time = 0;
			chrono.restart();
            chrono.stop();
			countdown_on = false;
            game_on = true;
		}
		else if (sender->value == "Countdown")
		{
            compare_elapsed = -1;	
            offset_time = 0;
            uint8_t cntint = EEPROM.read(EEP_COUNTDOWN);
            if (cntint > 99)
                cntint = COUNTDOWN_INIT_TIME;
			countdown_time = chrono.elapsed() + cntint * 60;
            chrono.restart();
            chrono.stop();
			countdown_on = true;
            game_on = true;

		}
        else if (sender->value == "Escape")
        {
            chrono.stop();
			countdown_on = false;
			game_on = false;
		}
		else if (sender->value == "F1")
		{
			
		}
		else if (sender->value == "Horn")
		{
			buzzer_ring(4);
		}
		else if (sender->value == "Clear")
		{
			Score_host = 0;
			Score_guest = 0;
            Team_fouls_host = '0';
            Team_fouls_guest = '0';
            Period = '0';
			score_to_buf();
			myLED.print(Display_buf, INVERT_DISPLAY);
			updateStatusBoard();
            ESPUI.getControl(hostNumber)->value = String(Score_host);
            ESPUI.getControl(guestNumber)->value = String(Score_guest);
            ESPUI.updateControl(hostNumber);
            ESPUI.updateControl(guestNumber);
        }
        
    case B_UP:
        //.println("Button UP");
        break;
    }
}

void updateStatusBoard() {
    char disp_buf[13] = { 0 };
    char buf[10];

    itoa(minutes, buf, 10);
    if (buf[1] == 0) {
		disp_buf[0] = '0';
		disp_buf[1] = buf[0];
	}
	else {
		disp_buf[0] = buf[0];
		disp_buf[1] = buf[1];
	}
    disp_buf[2] = ':';
    itoa(seconds, buf, 10); 
    if (buf[1] == 0) {
        disp_buf[3] = '0';
        disp_buf[4] = buf[0];
        }
    else {
        disp_buf[3] = buf[0];
		disp_buf[4] = buf[1];
	}
    disp_buf[5] = '\n';

    itoa(Score_host, buf, 10);
    if (buf[1] == 0) {
		disp_buf[6] = ' ';
		disp_buf[7] = buf[0];
	}
	else {
		disp_buf[6] = buf[0];
		disp_buf[7] = buf[1];
	}
    disp_buf[8] = '-';
    itoa(Score_guest, buf, 10);
    if (buf[1] == 0) {
        disp_buf[9] = buf[0]; 
        disp_buf[10] = ' ';
        }
    else {
        disp_buf[9] = buf[0];
        disp_buf[10] = buf[1];
        }
    disp_buf[11] = '\0';

    ESPUI.getControl(scoreLabel)->value = disp_buf;
    ESPUI.updateControl(scoreLabel);
}

void generalCallback(Control* sender, int type) {
    Serial.print("CB: id(");
    Serial.print(sender->id);
    Serial.print(") Type(");
    Serial.print(type);
    Serial.print(") '");
    Serial.print(sender->label);
    Serial.print("' = ");
    Serial.println(sender->value);
    if (sender->label == "Set_time") {
        // Ανάλυση του ISO 8601 string
        int year, month, day, hour, minute, second;
        float milliseconds;
        sscanf(sender->value.c_str(), "%d-%d-%dT%d:%d:%d.%fZ", &year, &month, &day, &hour, &minute, &second, &milliseconds);

        struct tm tm;
        tm.tm_year = year - 1900; // Το έτος πρέπει να είναι έτος από 1900.
        tm.tm_mon = month - 1; // Οι μήνες ξεκινούν από το 0 (Ιανουάριος).
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        // Ρύθμιση του χρόνου και της ημερομηνίας
        Serial.println("Trying to fix the local time - Set the local RTC..");
        Serial.println(&tm, "%H:%M:%S");
        //setTime(2021, 3, 28, 1, 59, 50, 1);
        setTimezone("GMT0");
        setTime(year, month, day, hour, minute, second, 1);
        SetExtRTC();
        setTimezone("EET-2EEST,M3.5.0/3,M10.5.0/4");
    }
    else if (sender->label == "Tempo Seconds")
    {
        EEPROM.write(EEP_TEMPO, sender->value.toInt());
        EEPROM.commit();
    }
    else if (sender->label == "Display Info Options")
    {
        EEPROM.write(EEP_TIME_DISP, sender->value.toInt());
        EEPROM.commit();
    }
    else if (sender->label == "Display Date")
    {
        EEPROM.write(EEP_DATE_DISP, sender->value.toInt());
        EEPROM.commit();
    }
    else if (sender->label == "Display Temp")
    {
        EEPROM.write(EEP_TEMP_DISP, sender->value.toInt());
        EEPROM.commit();
    }
    else if (sender->label == "Display Humi")
    {
        EEPROM.write(EEP_HUMI_DISP, sender->value.toInt());
        EEPROM.commit();
    }
    else if (sender->label == "Auto Sensor Brigntness")
    {
		Sensor_on = sender->value.toInt();
        EEPROM.write(EEP_SENSOR, sender->value.toInt());
        EEPROM.commit();
    }
    else if (sender->label == "Host score")
    {
        Score_host = sender->value.toInt();
        Serial.print("Score_host: ");
        Serial.println(Score_host);
        score_to_buf();
        myLED.print(Display_buf, INVERT_DISPLAY);
        updateStatusBoard();
    }
    else if (sender->label == "Guest score")
    {
        Score_guest = sender->value.toInt();
        Serial.print("Score_guest: ");
        Serial.println(Score_guest);
        score_to_buf();
        myLED.print(Display_buf, INVERT_DISPLAY);
        updateStatusBoard();
    }
    else if (sender->label == "Countdown time")
    {
        EEPROM.write(EEP_COUNTDOWN, sender->value.toInt());
        EEPROM.commit();
    }
    else if (sender->label == "Timer value")
    {
        Serial.print("Change Timer to: ");
        //Serial.println(sender->value);
        // convert sender->value in 2 integers minutes and seconds
        int sep = sender->value.indexOf(':');
        minutes = sender->value.substring(0, sep).toInt();
        seconds = sender->value.substring(sep + 1).toInt();
        Serial.print("Minutes: "); Serial.print(minutes);
        Serial.print(" Seconds: "); Serial.println(seconds);

        offset_time = (minutes * 60 + seconds);
        // check if chrono is running
        if (chrono.isRunning())
        {
            chrono.restart();
        }
        else {
            chrono.restart();
            chrono.stop();
        }
        
	}
}

void readStringFromEEPROM(String& buf, int baseaddress, int size) {
    buf.reserve(size);
    for (int i = baseaddress; i < baseaddress + size; i++) {
        char c = EEPROM.read(i);
        buf += c;
        if (!c) break;
    }
}
//=========================================================================
// Time settings
//=========================================================================
void setTimezone(String timezone) {
    Serial.printf("  Setting Timezone to %s\n", timezone.c_str());
    setenv("TZ", timezone.c_str(), 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
    tzset();
}

//bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) {
//    Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
//
//    return true;
//}
//=========================================================================

/*
 * Optional code, if you require a callback
 */
#if defined(USE_CALLBACK_FOR_TINY_RECEIVER)
 /*
  * This is the function, which is called if a complete frame was received
  * It runs in an ISR context with interrupts enabled, so functions like delay() etc. should work here
  */
#  if defined(ESP8266) || defined(ESP32)
IRAM_ATTR
#  endif

void handleReceivedTinyIRData() {
#  if defined(ARDUINO_ARCH_MBED) || defined(ESP32)
    /*
     * Printing is not allowed in ISR context for any kind of RTOS, so we use the slihjtly more complex,
     * but recommended way for handling a callback :-). Copy data for main loop.
     * For Mbed we get a kernel panic and "Error Message: Semaphore: 0x0, Not allowed in ISR context" for Serial.print()
     * for ESP32 we get a "Guru Meditation Error: Core  1 panic'ed" (we also have an RTOS running!)
     */
     // Do something useful here...
#  else
    // As an example, print very short output, since we are in an interrupt context and do not want to miss the next interrupts of the repeats coming soon
    printTinyReceiverResultMinimal(&Serial);
#  endif
}
#endif

TaskHandle_t Task0;

void setup() {
    EEPROM.begin(255);
    Serial.begin(115200);
	while (!Serial) { ; }
	//=======================================================================
    //print the file location and date
    
	Serial.println(__FILE__); 
	Serial.println(__DATE__);
	Serial.print("Software Verions: "); Serial.println(soft_version);
	Serial.println(soft_ID);
	Serial.println("-------------------------------------------------");
    //-----------------------------------------------------------------------
    irrecv.enableIRIn();

    chrono.start();
    chrono.stop();
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
	Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
    sensors.begin();
    sensors.setResolution(9);
    rtc.init();
    /*mb.begin(&Serial2, DIR, true);
    mb.master();*/
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);
    pinMode(Open_Hotspot, INPUT_PULLUP);
    myLED.displayEnable();     // This command has no effect if you aren't using OE pin
    myLED.normalMode();

	delay(100);
    ReadSettingEeprom();
    photo_sample();
    myLED.displayBrightness(Brightness);
    /*if (!mb.slave()) {
        mb.writeHreg(SLAVE_ID, REG_BRIGHTNESS, Brightness, cbWrite);
    }*/

    myLED.TestSegments(DIGITS_DISPLAY);

    Serial.print("Hot spot switch: ");
    if (digitalRead(OPEN_HOT_SPOT))
    {
        Serial.println("OFF");
    }
    else {
        Serial.println("ON");
    }

    uint8_t cpuClock = ESP.getCpuFreqMHz();
    flash_timer = timerBegin(1, cpuClock, true);
    timerAttachInterrupt(flash_timer, &FlashInt, true);
    timerAlarmWrite(flash_timer, 100000, true);
    timerAlarmEnable(flash_timer);
    //-----------------------------------------------------------------------------------------------
    esp32FOTA.setManifestURL(manifest_url);
    esp32FOTA.printConfig();
    //-----------------------------------------------------------------------
  // GUI interface setup
    WiFi.setHostname(hostname);
    ReadWiFiCrententials();

    Serial.println("Stored SSID:");
    Serial.println(stored_ssid);
    Serial.println("Stored password:");
    Serial.println(stored_pass);
    Serial.println(".....................");
    /*Serial.println("Stored User:");
    Serial.println(stored_user);
    Serial.println("Stored User pass:");
    Serial.println(stored_password);*/
    // try to connect to existing network
    WiFi.mode(WIFI_STA);

    WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
    //WiFi.begin(ssid, password);
    Serial.print("\n\nTry to connect to existing network");

    uint8_t timeout = 10;

    // Wait for connection, 5s timeout
    do {
        delay(500);
        Serial.print(".");
        timeout--;
    } while (timeout && WiFi.status() != WL_CONNECTED);

    // not connected -> create hotspot
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("\n\nCreating hotspot");

        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP(ssid);
        timeout = 5;

        do {
            delay(500);
            Serial.print(".");
            timeout--;
        } while (timeout);
    }

	//print the WiFi channel of the device
	Serial.print("WiFi channel: "); Serial.println(WiFi.channel());

    //------------------------------------------------------------------------
    // Init ESP-NOW
	// Print MAC address
	Serial.print("MAC Address: "); Serial.println(WiFi.macAddress());
	// Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
	} else {
		Serial.println("initialized ESP - NOW");
	}

    // Καθορισμός παραμέτρων για τον πομπό
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress1, 6); // Διεύθυνση MAC του πομπού
    peerInfo.channel = WiFi.channel();  // Ορίστε το κανάλι του πομπού
    peerInfo.encrypt = false; // Χωρίς κρυπτογράφηση

    // Προσθήκη του πομπού ως peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
	} else {
        Serial.println("Peer added successfully");
    }
	// Register for a callback function that will be called when data is received
	esp_now_register_recv_cb(OnDataRecv);

    //print the WiFi channel of the device
	Serial.print("WiFi channel: "); Serial.println(WiFi.channel());
            
    //------------------------------------------------------------------------

    dnsServer.start(DNS_PORT, "*", apIP);

    Serial.println("\n\nWiFi parameters:");
    Serial.print("Mode: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? "Station" : "Client");
    Serial.print("IP address: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());

    Serial.println("WiFi mode:");
    Serial.println(WiFi.getMode());

    IPdisplay();

    //---------------------------------------------------------------------------------------------
    //We need this CSS style rule, which will remove the label's background and ensure that it takes up the entire width of the panel
    String clearLabelStyle = "background-color: unset; width: 100%;";

    uint16_t tab1 = ESPUI.addControl(ControlType::Tab, "Main Display", "Main Display");
    uint16_t tab2 = ESPUI.addControl(ControlType::Tab, "User Settings", "User");
    uint16_t tab3 = ESPUI.addControl(ControlType::Tab, "WiFi Settings", "WiFi");
    
    ButtonsTimer = ESPUI.addControl(ControlType::Button, "Game Timer", "Sart", ControlColor::Turquoise, tab1, &ScoreboardCallback);
    ESPUI.addControl(ControlType::Button, "ButStop", "Stop", ControlColor::None, ButtonsTimer, &ScoreboardCallback);
    ESPUI.addControl(ControlType::Button, "ButUp", "Up cunter", ControlColor::None, ButtonsTimer, &ScoreboardCallback);
    ESPUI.addControl(ControlType::Button, "ButDown", "Countdown", ControlColor::None, ButtonsTimer, &ScoreboardCallback);

    ButtonsGame = ESPUI.addControl(ControlType::Button, "Game Instructions", "Escape", ControlColor::Carrot, tab1, &ScoreboardCallback);
    ESPUI.addControl(ControlType::Button, "ButF1", "F1", ControlColor::None, ButtonsGame, &ScoreboardCallback);
    ESPUI.addControl(ControlType::Button, "ButHorn", "Horn", ControlColor::None, ButtonsGame, &ScoreboardCallback);
    ESPUI.addControl(ControlType::Button, "ButClear", "Clear", ControlColor::None, ButtonsGame, &ScoreboardCallback);

    liveTime = ESPUI.addControl(Text, "Timer value", "", Carrot, tab1, &generalCallback);
    ESPUI.setInputType(liveTime, "time");

    hostNumber = ESPUI.addControl(Number, "Host score", String(Score_host), Emerald, tab1, &generalCallback);
    guestNumber = ESPUI.addControl(Number, "Guest score", String(Score_guest), Emerald, tab1, &generalCallback);

    uint8_t cntint = EEPROM.read(EEP_COUNTDOWN);
    cntdownTime = ESPUI.addControl(Number, "Countdown time", String(cntint), Sunflower, tab1, &generalCallback);

    ESPUI.addControl(Separator, "Score controls", "", Peterriver, tab1);

    //We will now need another label style. This one sets its width to the same as a switcher (and turns off the background)
    String switcherLabelStyle = "width: 60px; margin-left: .3rem; margin-right: .3rem; background-color: unset;";
    //Some controls can even support vertical orientation, currently Switchers and Sliders
    ESPUI.addControl(Separator, "Display controls", "", None, tab1);
    uint8_t Switch_val = EEPROM.read(EEP_TIME_DISP);
    auto vertgroupswitcher = ESPUI.addControl(Switcher, "Display Info Options", String(Switch_val), Dark, tab1, generalCallback);
    ESPUI.setVertical(vertgroupswitcher);
    //On the following lines we wrap the value returned from addControl and send it straight to setVertical
    Switch_val = EEPROM.read(EEP_DATE_DISP);
    ESPUI.setVertical(ESPUI.addControl(Switcher, "Display Date", String(Switch_val), None, vertgroupswitcher, generalCallback));
    Switch_val = EEPROM.read(EEP_TEMP_DISP);
    ESPUI.setVertical(ESPUI.addControl(Switcher, "Display Temp", String(Switch_val), None, vertgroupswitcher, generalCallback));
    Switch_val = EEPROM.read(EEP_HUMI_DISP);
    ESPUI.setVertical(ESPUI.addControl(Switcher, "Display Humi", String(Switch_val), None, vertgroupswitcher, generalCallback));
    //The mechanism for labelling vertical switchers is the same as we used above for horizontal ones
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "", None, vertgroupswitcher), clearLabelStyle);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Time", None, vertgroupswitcher), switcherLabelStyle);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Date", None, vertgroupswitcher), switcherLabelStyle);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Temp", None, vertgroupswitcher), switcherLabelStyle);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Humi", None, vertgroupswitcher), switcherLabelStyle);

    //Number inputs also accept Min and Max components, but you should still validate the values.
    uint8_t tempo = EEPROM.read(EEP_TEMPO);
    char tempoString[3];
    itoa(tempo, tempoString, 10);
    tempoNumber = ESPUI.addControl(Number, "Tempo Seconds", tempoString, Emerald, tab1, generalCallback);
    ESPUI.addControl(Min, "", "1", None, tempoNumber);
    ESPUI.addControl(Max, "", "9", None, tempoNumber);

    photosensor = ESPUI.addControl(Switcher, "Auto Sensor Brigntness", "0", Dark, tab1, generalCallback);
    ESPUI.getControl(photosensor)->value = String(Sensor_on);

    mainSlider = ESPUI.addControl(ControlType::Slider, "Brightness", "255", ControlColor::Alizarin, tab1, &slider);
    ESPUI.addControl(Min, "", "0", None, mainSlider);
    ESPUI.addControl(Max, "", "255", None, mainSlider);
    ESPUI.getControl(mainSlider)->value = String(Brightness);

    ESPUI.addControl(Button, "Synchronize with your time", "Set", Carrot, tab1, getTimeCallback);

    Readuserdetails();
    user_text = ESPUI.addControl(Text, "User name", stored_user, Alizarin, tab2, textCallback); // stored_user
    ESPUI.addControl(Max, "", "32", None, user_text);
    password_text = ESPUI.addControl(Text, "Password", stored_password, Alizarin, tab2, textCallback); //stored_password
    ESPUI.addControl(Max, "", "32", None, password_text);
    ESPUI.setInputType(password_text, "password");
    ESPUI.addControl(Button, "User", "Save", Peterriver, tab2, enterWifiDetailsCallback);

    wifi_ssid_text = ESPUI.addControl(Text, "SSID", stored_ssid, Alizarin, tab3, textCallback); // stored_ssid
    ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
    wifi_pass_text = ESPUI.addControl(Text, "Password", stored_pass, Alizarin, tab3, textCallback); //stored_pass
    ESPUI.addControl(Max, "", "64", None, wifi_pass_text);
    ESPUI.addControl(Button, "WiFi", "Save", Peterriver, tab3, enterWifiDetailsCallback);

    ESPUI.addControl(ControlType::Button, "Reset device", "Reset", ControlColor::Emerald, tab3, &ResetCallback);

    //WiFiStatus = ESPUI.addControl(Label, "WiFi Status", "Wifi Status ok", ControlColor::None, tab3);

    scoreLabel = ESPUI.addControl(Label, "Score Board", "--\n--", Dark, Control::noParent, generalCallback);
    ESPUI.setElementStyle(scoreLabel, "text-shadow: 3px 3px #74b1ff, 6px 6px #c64ad7; font-size: 60px; font-variant-caps: small-caps; background-color: unset; color: #c4f0bb; -webkit-text-stroke: 1px black;");

    photoValue = analogRead(photoPin);
    status = ESPUI.addControl(Label, "Status", "System status: OK", Wetasphalt, tab1);
    ESPUI.getControl(status)->value = "Photo sensor = " + String(photoValue);

	//Conver version to const char*
    ESPUI.addControl(Separator, soft_ID, "", Peterriver, tab1);

    ESPUI.begin("- Scoreboard", stored_user.c_str(), stored_password.c_str()); //"espboard", 

    digitalWrite(LED, LOW);
    //-------------------------------------------------------------
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    //configTime( 0, 0, ntpServer);
     //Europe/Athens	EET-2EEST,M3.5.0/3,M10.5.0/4
    setTimezone("EET-2EEST,M3.5.0/3,M10.5.0/4");
    printLocalTime();
    SetExtRTC();
    oldTime = millis();
    buzzer_ring(4);

    updateStatusBoard();

	//....................................................................
    //Testing sending to WiFi now 
	display_packet.id = 0x01;
	display_packet.insrtuction = 0x01;
  
    esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t*)&display_packet, sizeof(display_packet));
    if (result == ESP_OK) {
        Serial.println("WiFiNOW Sent 1 with success");
        //open LED to green
        /*digitalWrite(LED_PIN, LOW);
        previousMillis = millis();*/
    }
    else {
        Serial.println("WiFiNOW Error sending 1 the data");
    }
	//....................................................................

    //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
    xTaskCreatePinnedToCore(
        Task0code,   /* Task function. */
        "Task0",     /* name of task. */
        10000,       /* Stack size of task */
        NULL,        /* parameter of the task */
        0,           /* priority of the task */
        &Task0,      /* Task handle to keep track of created task */
        0);          /* pin task to core 0 */
    delay(500);
}

void Task0code(void* pvParameters) {

    uint8_t laps_count = 0;
    uint8_t lap_mode = 0;
    char buffer[5];
    
    for (;;) 
    {
        if (countdown_timeout && countdown_on)
        {
            chrono.restart();
            chrono.stop();
            timerAlarmDisable(flash_timer);

            digitalWrite(BUZZER, HIGH);
            myLED.print("        ", INVERT_DISPLAY);
            delay(1000);
            digitalWrite(BUZZER, LOW);
            myLED.print(Display_buf, INVERT_DISPLAY);

            delay(1000);
            digitalWrite(BUZZER, HIGH);
            myLED.print("        ", INVERT_DISPLAY);
            delay(1000);
            digitalWrite(BUZZER, LOW);
            myLED.print(Display_buf, INVERT_DISPLAY);

            delay(1000);
            digitalWrite(BUZZER, HIGH);
            myLED.print("        ", INVERT_DISPLAY);
            delay(1000);
            digitalWrite(BUZZER, LOW);
            myLED.print(Display_buf, INVERT_DISPLAY);

            timerAlarmEnable(flash_timer);
            delay(2000);
            countdown_timeout = false;
            countdown_on = false;
            game_on = false;
        }
        if (compare_elapsed != chrono.elapsed())
        {
            if (chrono.elapsed() > 5940)
            {
                chrono.restart();
                minutes = 0;
                seconds = 0;
            }
            int countdown_elapled;
            if (countdown_on) {
                if (offset_time != 0) {
                    countdown_elapled = offset_time - chrono.elapsed();
                }
                else {
                    countdown_elapled = countdown_time - chrono.elapsed();
                }
            }
            else {
                countdown_elapled = chrono.elapsed() + offset_time;
            }
            seconds = countdown_elapled % 60;
            minutes = (countdown_elapled / 60) % 60;
            compare_elapsed = chrono.elapsed();
            displayTimer(minutes, seconds);
            if (game_on && chrono.hasPassed(countdown_time))
                countdown_timeout = true;
            updateStatusBoard();
        }

        // Check if Serial2 have any data available
        if (Serial2.available() > 1) {
			uint16_t Instruction = 0;
            Serial.println();
            Serial.print("Instruction: ");
            while (Serial2.available() > 0) {
                // Read the most recent byte
                char incomingByte = Serial2.read();
				Instruction = (Instruction << 8) | incomingByte;
                // Print the incoming byte to the serial monitor
                
                Serial.print(incomingByte, HEX); Serial.print(" ");
            }

            switch (Instruction)
            {
			case INSTR_1:
				
                if (game_on) {
                    chrono.resume();
                    //Send Start instruction to the shoot clock
                    Serial1.write(0XA1); Serial1.write(0X53);
                    buzzer_ring(1);
                }				
                Serial.println();
                Serial.println("Start the timer");
				break;
			case INSTR_2:		
				chrono.stop();
                //Send Stop instruction to the shoot clock
                Serial1.write(0XA1); Serial1.write(0X53);
				buzzer_ring(1);
                Serial.println();
                Serial.println("Stop the timer");
				break;
			case INSTR_3:
                Serial.println();
				Serial.println("Host score +1");
				Score_host++;
				score_to_buf();
				myLED.print(Display_buf, INVERT_DISPLAY);
				ESPUI.getControl(hostNumber)->value = String(Score_host);
				ESPUI.updateControl(hostNumber);
				updateStatusBoard();				
				break;
			case INSTR_4:
				Serial.println();
				Serial.println("Host score -1");
				Score_host--;
				score_to_buf();
				myLED.print(Display_buf, INVERT_DISPLAY);
				ESPUI.getControl(hostNumber)->value = String(Score_host);
				ESPUI.updateControl(hostNumber);
				updateStatusBoard();
				break;
			case INSTR_5:
				Serial.println();
				Serial.println("Guest score +1");
				Score_guest++;
				score_to_buf();
				myLED.print(Display_buf, INVERT_DISPLAY);
				ESPUI.getControl(guestNumber)->value = String(Score_guest);
				ESPUI.updateControl(guestNumber);
				updateStatusBoard();
				break;
			case INSTR_6:
				Serial.println();
				Serial.println("Guest score -1");
				Score_guest--;
				score_to_buf();
				myLED.print(Display_buf, INVERT_DISPLAY);
				ESPUI.getControl(guestNumber)->value = String(Score_guest);
				ESPUI.updateControl(guestNumber);
				updateStatusBoard();
				break;
            default:
                break;
            }

            Serial.println(); Serial.println();
        }

        if (irrecv.decode(&results)) {
            if (results.decode_type == NEC && results.address == NEC_ADD_ROUSIS) {
                handleIRCommand(results.command);
            }
            else if (results.decode_type == NEC && results.command == 0xFFFFFFFF) {
                handleIRCommand(results.command); // Handle repeat code
            }
            else {
                //Serial.print("Unsupported IR signal or invalid address: ");
                //Serial.print("Protocol="); Serial.print(results.decode_type);
                //Serial.print(", Address=0x"); Serial.println(results.address, HEX);
            }
            irrecv.resume();
		}

        delay(2);
    }
}

void displayTimer(int minutes, int seconds)
{
    char buffer[5];
    itoa(minutes, buffer, 10);
    if (buffer[1] == 0)
	{
		buffer[1] = buffer[0];
		buffer[0] = '0';
	}
    Display_buf[0] = buffer[0]; Display_buf[1] = buffer[1] | 0x80;
    itoa(seconds, buffer, 10);
    if (buffer[1] == 0)
    {
        buffer[1] = buffer[0];
        buffer[0] = '0';
    }
    Display_buf[2] = buffer[0]; Display_buf[3] = buffer[1];
    myLED.print(Display_buf, INVERT_DISPLAY);
}

void loop() {
    dnsServer.processNextRequest();
    esp32FOTA.handle();
    //....................................................................
    oldTime = millis();
    if (game_on == false)
    {
        uint8_t tempo = EEPROM.read(EEP_TEMPO);
        if (tempo > 9 || tempo == 0)
            tempo = 9;
        Serial.print("Tempo Sec.= ");
        Serial.println(tempo);
        score_to_buf();
        digitalWrite(LED, HIGH);

        if (!game_on && !EEPROM.read(EEP_TIME_DISP) && !EEPROM.read(EEP_DATE_DISP) && !EEPROM.read(EEP_TEMP_DISP) && !EEPROM.read(EEP_DATE_DISP))
        {
            if (!game_on)
                myLED.print("------", INVERT_DISPLAY);
            delay(tempo * 3000);
        }
        printLocalTime();
        while (millis() - oldTime < tempo * 1000 && EEPROM.read(EEP_TIME_DISP))
        {
            digitalWrite(LED, LOW);
            if (!game_on)
                displayocalTime(0x80);
            delay(500);
            digitalWrite(LED, HIGH);
            if (!game_on)
                displayocalTime(0);
            delay(500);
        }

        if (EEPROM.read(EEP_DATE_DISP)) {
            digitalWrite(LED, LOW);
            displayocalDate();
            delay(tempo * 1000);
        }

        if (EEPROM.read(EEP_TEMP_DISP)) {
            sensors.requestTemperatures();
            float temperature = sensors.getTempCByIndex(0);
            if (temperature > -50)
            {
                char buf[10] = { 0 };
                itoa(temperature, buf, 10);

                if (DIGITS_DISPLAY >= 8)
                {
                    Display_buf[0] = buf[0];
                    Display_buf[1] = buf[1];
                    if (!buf[2]) {
                        Display_buf[2] = '*'; // 'º';
                        Display_buf[3] = 'C';
                    }
                    else {
                        Display_buf[2] = buf[2];
                        Display_buf[3] = '*';
                    }
                    if (!game_on)
                        myLED.print(Display_buf, INVERT_DISPLAY);
                }
                else if (DIGITS_DISPLAY == 6)
                {
                    Display_buf[1] = buf[0];
                    Display_buf[2] = buf[1];
                    if (!buf[2]) {
                        Display_buf[3] = '*'; // 'º';
                        Display_buf[4] = 'C';
                    }
                    else {
                        Display_buf[3] = buf[2];
                        Display_buf[4] = '*';
                    }
                    if (!game_on)
                        myLED.print(Display_buf, INVERT_DISPLAY);
                }
            }
            else {
                Display_buf[0] = '-'; // 'º';
                Display_buf[1] = '-';
                Display_buf[2] = '*'; // 'º';
                Display_buf[3] = 'C';
                Display_buf[4] = ' ';
                Display_buf[5] = ' ';
                if (!game_on)
                    myLED.print(Display_buf, INVERT_DISPLAY);
            }

            Serial.println("Ampient Temperature is:");
            Serial.print(temperature);
            Serial.println("ºC");

            delay(tempo * 1000);
        }
    }

    /*if (!mb.slave() && BrighT_Update_Count > 60) {
        mb.writeHreg(SLAVE_ID, REG_BRIGHTNESS, Brightness, cbWrite);
        BrighT_Update_Count = 0;
    }
    else {
        BrighT_Update_Count++;
    }*/

    /*
    while ((WiFi.status() != WL_CONNECTED))
    {
        WiFi.disconnect();
        WiFi.reconnect();
        Serial.println("No internet Try to reconnect...");
    }
    */
    // Check if the connection is lost
    // Check if the connection is lost


    if (WiFi.getMode() != WIFI_AP && WiFi.status() != WL_CONNECTED) {
        Serial.println("Connection lost. Reconnecting...");
        WiFi.reconnect();
        // delay(2000); // Προσθέστε μια μικρή καθυστέρηση
    }

    if (millis() - photo_delay > PHOTO_SAMPLE_TIME)
    {
        photo_sample();
        photo_delay = millis();
    }

}

String printTimeString(void) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return "--";
    }
    //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    char time_str[10];
    strftime(time_str, 3, "%H:%M:%S", &timeinfo);
    return time_str;
}

void displayocalDate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    char date[3];
    strftime(date, 3, "%d", &timeinfo);
    char month[3];
    strftime(month, 3, "%m", &timeinfo);
    char Displaybuf[6];
    if (DIGITS_DISPLAY >= 8)
    {
        Displaybuf[0] = date[0];
        Displaybuf[1] = date[1] | 0x80;
        Displaybuf[2] = month[0];
        Displaybuf[3] = month[1];
    }
    else if (DIGITS_DISPLAY == 6)
    {
        Displaybuf[0] = ' ';
        Displaybuf[1] = date[0];
        Displaybuf[2] = date[1];
        Displaybuf[3] = '-';
        Displaybuf[4] = month[0];
        Displaybuf[5] = month[1];
    }
    myLED.print(Displaybuf, INVERT_DISPLAY);
}

void displayocalTime(uint8_t dots_on) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        myLED.print("------", INVERT_DISPLAY);
        Serial.println("Failed to obtain time to display");
        return;
    }

    char timeHour[3];
    strftime(timeHour, 3, "%H", &timeinfo);
    char timeMinute[3];
    strftime(timeMinute, 3, "%M", &timeinfo);
    //char Displaybuf[6];
    if (DIGITS_DISPLAY >= 8)
    {
        Display_buf[0] = timeHour[0];
        Display_buf[1] = timeHour[1] | dots_on;
        Display_buf[2] = timeMinute[0] | dots_on;
        Display_buf[3] = timeMinute[1];
    }
    else if (DIGITS_DISPLAY == 6)
    {
        char timeSecond[3];
        strftime(timeSecond, 3, "%S", &timeinfo);
        Display_buf[0] = timeHour[0];
        Display_buf[1] = timeHour[1] | dots_on;
        Display_buf[2] = timeMinute[0];
        Display_buf[3] = timeMinute[1] | dots_on;
        Display_buf[4] = timeSecond[0];
        Display_buf[5] = timeSecond[1];
    }



    int conv_min = atoi(timeMinute);
    int send_data = atoi(timeHour) | dots_on;
    send_data = (send_data << 8) | conv_min;
    //mb.task();
    ////yield();
    //if (!mb.slave()) {
    //    mb.writeHreg(SLAVE_ID, REG1, send_data, cbWrite);
    //}

    myLED.print(Display_buf, INVERT_DISPLAY);
}

void printLocalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    char timeStringBuff[50]; //50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%a, %d %h %Y %H:%M:%S", &timeinfo);
    //print like "const char*"
    Serial.println(timeStringBuff);
    //Optional: Construct String object 
    String asString(timeStringBuff);

    ESPUI.getControl(status)->value = "Ver.: " + soft_version + "<br>Display Time: " + asString + "<br>Photo sensor = " + String(photoValue);
    //Serial.println(&timeinfo, "%H:%M:%S"); "Photo sens. = " + String(photoValue);
}

void SetExtRTC() {

    Serial.println("Setting the external RTC");
    setTimezone("GMT0");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        get_time_externalRTC();
        return;
    }
    setTimezone("EET-2EEST,M3.5.0/3,M10.5.0/4");
    /*
        Serial.print("Time to set DS1302: ");
        Serial.print(timeinfo.tm_year); Serial.print(" ");
        Serial.print(timeinfo.tm_mon); Serial.print(" ");
        Serial.print(timeinfo.tm_mday); Serial.print(" ");
        Serial.print(timeinfo.tm_hour); Serial.print(" ");
        Serial.print(timeinfo.tm_min); Serial.print(" ");
        Serial.println(timeinfo.tm_sec);
        Serial.println("--------------------------------------");*/

    Ds1302::DateTime dt = {
           .year = timeinfo.tm_year,
           .month = timeinfo.tm_mon + 1,
           .day = timeinfo.tm_mday,
           .hour = timeinfo.tm_hour,
           .minute = timeinfo.tm_min,
           .second = timeinfo.tm_sec,
           .dow = timeinfo.tm_wday
    };
    rtc.setDateTime(&dt);

    Serial.println(&timeinfo, "%A, %d %B %Y %H:%M:%S");
    Serial.println("----------------------------------------");
}

void get_time_externalRTC() {
    // get the current time
    Ds1302::DateTime now;
    rtc.getDateTime(&now);

    Serial.println();
    Serial.println("Read and set the external Time from DS1302:");

    Serial.print("20");
    Serial.print(now.year);    // 00-99
    Serial.print('-');
    if (now.month < 10) Serial.print('0');
    Serial.print(now.month);   // 01-12
    Serial.print('-');
    if (now.day < 10) Serial.print('0');
    Serial.print(now.day);     // 01-31
    Serial.print(' ');
    Serial.print(now.dow); // 1-7
    Serial.print(' ');
    if (now.hour < 10) Serial.print('0');
    Serial.print(now.hour);    // 00-23
    Serial.print(':');
    if (now.minute < 10) Serial.print('0');
    Serial.print(now.minute);  // 00-59
    Serial.print(':');
    if (now.second < 10) Serial.print('0');
    Serial.print(now.second);  // 00-59
    Serial.println();
    Serial.println("---------------------------------------------");

    setTimezone("GMT0");
    setTime(now.year + 2000, now.month, now.day, now.hour, now.minute, now.second, 1);
    setTimezone("EET-2EEST,M3.5.0/3,M10.5.0/4");
}
//----------------------------------------------- Fuctions used for WiFi credentials saving and connecting to it which you do not need to change
bool testWifi(void)
{
    int c = 0;
    Serial.println("Waiting for Wifi to connect");
    while (c < 20) {
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("");
            return true;
        }
        delay(500);
        Serial.print("*");
        c++;
    }
    Serial.println("");
    Serial.println("Connect timed out, opening AP");
    return false;
}

void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst) {
    struct tm tm;

    tm.tm_year = yr - 1900;   // Set date
    tm.tm_mon = month - 1;
    tm.tm_mday = mday;
    tm.tm_hour = hr;      // Set time
    tm.tm_min = minute;
    tm.tm_sec = sec;
    tm.tm_isdst = isDaylightSavingTime(&tm);  // 1 or 0
    time_t t = mktime(&tm);
    Serial.printf("Setting time: %s", asctime(&tm));
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);
}

bool isDaylightSavingTime(struct tm* timeinfo) {
    // Implement your logic to check if DST is in effect based on your timezone's rules
    // For example, you might check if the current month and day fall within the DST period.

    int month = timeinfo->tm_mon + 1; // Month is 0-11, so add 1
    int day = timeinfo->tm_mday;

    // Add your DST logic here
    // For example, assuming DST is from April to October
    if ((month > 3) && (month < 11)) {
        return true;
    }

    return false;
}

void score_to_buf()
{
    //"00 - 00 0 - 0"
    char buf[6];
    itoa(Score_host, buf, 10);
    if (!buf[1]) {
        Display_buf[4] = ' ';
        Display_buf[5] = buf[0];
    }
    else {
        Display_buf[4] = buf[0];
        Display_buf[5] = buf[1];
    }

    itoa(Score_guest, buf, 10);
    if (!buf[1]) {
        Display_buf[6] = buf[0];
        Display_buf[7] = ' ';
    }
    else {
        Display_buf[6] = buf[0];
        Display_buf[7] = buf[1];
    }

    Display_buf[12] = 0;
}

void IPdisplay() {

    char Displaybuf[7];
    IPAddress localIp;
    char arrIp[17];
    const char* strLocalIp;

    if (WiFi.getMode() == WIFI_AP) {
        memcpy(Displaybuf, "AP Display:", 11);
        localIp = WiFi.softAPIP();
        strLocalIp = WiFi.softAPIP().toString().c_str();
    }
    else {
        memcpy(Displaybuf, "IP Display:", 11);
        localIp = WiFi.localIP();
        strLocalIp = WiFi.localIP().toString().c_str();
    }
    myLED.print(Displaybuf, INVERT_DISPLAY);
    delay(1000);

    myLED.print("      ", INVERT_DISPLAY);
    Displaybuf[7] = { ' ' };
    itoa(localIp[0], Displaybuf, 10);
    if (Displaybuf[1] == 0) { Displaybuf[1] = ' '; Displaybuf[2] = 0; }
    myLED.print(Displaybuf, INVERT_DISPLAY);
    delay(1000);
    myLED.print(".   ", INVERT_DISPLAY);
    delay(500);
    itoa(localIp[1], Displaybuf, 10);
    if (Displaybuf[1] == 0) { Displaybuf[1] = ' '; Displaybuf[2] = 0; }
    myLED.print(Displaybuf, INVERT_DISPLAY);
    delay(1000);
    myLED.print(".   ", INVERT_DISPLAY);
    delay(500);
    itoa(localIp[2], Displaybuf, 10);
    if (Displaybuf[1] == 0) { Displaybuf[1] = ' '; Displaybuf[2] = 0; }
    myLED.print(Displaybuf, INVERT_DISPLAY);
    delay(1000);
    myLED.print(".   ", INVERT_DISPLAY);
    delay(500);
    itoa(localIp[3], Displaybuf, 10);
    if (Displaybuf[1] == 0) { Displaybuf[1] = ' '; Displaybuf[2] = 0; }
    myLED.print(Displaybuf, INVERT_DISPLAY);
    delay(1000);
    myLED.print(".   ", INVERT_DISPLAY);
    delay(500);
}

void buzzer_ring(uint8_t time_last) {
    buzzer_cnt = time_last;
    digitalWrite(BUZZER, HIGH);
}

void photo_sample(void) {
    // Reading potentiometer value
    if (!Sensor_on) { return; }

    photoValue = analogRead(photoPin);
    if (photoValue < 200) { photoValue = 1; }
    photoValue = (4095 - photoValue);
    photoValue = 0.06 * photoValue; //(255 / 4095) = 0.06
    if (photoValue > 240) { photoValue = 255; }
    Brightness = photoValue;
    if (Brightness < 20) { Brightness = 20; }

    if (samples_metter == -1) {
        samples_metter = 0;
        myLED.displayBrightness(Brightness);
        Serial.print("First Sensor value = ");
        Serial.println(Brightness);
    }
    photo_samples[samples_metter] = Brightness;
    samples_metter++;

    /*Serial.print("Sample metter nummber = ");
    Serial.println(samples_metter);
    Serial.print("Sensor value =  ");
    Serial.println(Brightness);*/

    if (samples_metter >= SAMPLES_MAX)
    {
        uint16_t A = 0;
        uint8_t i;
        for (i = 0; i < SAMPLES_MAX; i++) { A += photo_samples[i]; }
        samples_metter = 0;
        Brightness = A / SAMPLES_MAX;
        //uint8_t digits = eeprom_read_byte((uint8_t*)FAV_eep + DISPLAY_DIGITS);
        //TLC_config_byte(sram_brigt,digits);
        myLED.displayBrightness(Brightness);
        Serial.print("Average Sensor value to brightness = ");
        Serial.println(Brightness);

		//Update mainSlider with the brightness average value
		ESPUI.getControl(mainSlider)->value = String(Brightness);       
		ESPUI.updateControl(mainSlider);

    }
}

//========================================================================================================
void handleIRCommand(uint32_t command) {
	if (command == KEY_COUNTDOWN_1) {
		Serial.println("IR: COUNTDOWN BEGIN:");
		
        compare_elapsed = -1;
        offset_time = 0;
        uint8_t cntint = EEPROM.read(EEP_COUNTDOWN);
        if (cntint > 99)
            cntint = COUNTDOWN_INIT_TIME;
        countdown_time = chrono.elapsed() + cntint * 60;
        chrono.restart();
        chrono.stop();
        countdown_on = true;
        game_on = true;

		return;
	}

    switch (command) {
    case KEY_EXIT:
        Serial.println("IR: EXIT");
        chrono.stop();
        countdown_on = false;
        game_on = false;
        break;
    case KEY_RESET:
        Serial.println("IR: RESET");
        //Reset the device
        ESP.restart();
        break;
    case KEY_UP_COUNTER:
        Serial.println("IR: UP COUNTER");
        compare_elapsed = -1;
        offset_time = 0;
        chrono.restart();
        chrono.stop();
        countdown_on = false;
        game_on = true;
        break;
    case KEY_COUNTDOWN_1:
        Serial.println("IR: COUNTDOWN");
        
        break;
    case KEY_PLAY:
        chrono.resume();
        //Send Start instruction to the shoot clock
        Serial1.write(0XA1); Serial1.write(0X53);
		Serial.println("IR: PLAY");
        buzzer_ring(1);
		break;
    case KEY_STOP:		
        //Send Start instruction to the shoot clock
        Serial1.write(0XA1); Serial1.write(0X54);
		chrono.stop();
        Serial.println("IR: STOP");
        buzzer_ring(1);
		break;
	case KEY_HOME_UP:
		Serial.println("IR: HOST +1");
        Score_host++;
        if (Score_host > 99) { Score_host = 0; }
        Serial.println(Score_host);
        score_to_buf();
        myLED.print(Display_buf, INVERT_DISPLAY);
        updateStatusBoard();
		break;
	case KEY_HOME_DOWN:
		Serial.println("IR: HOST -1");
		Score_host--;
		if (Score_host > 99) { Score_host = 0; }
		Serial.println(Score_host);
		score_to_buf();
		myLED.print(Display_buf, INVERT_DISPLAY);
		updateStatusBoard();
		break;
	case KEY_GUEST_UP:
		Serial.println("IR: GUEST +1");
		Score_guest++;
		if (Score_guest > 99) { Score_guest = 0; }
		Serial.println(Score_guest);
		score_to_buf();
		myLED.print(Display_buf, INVERT_DISPLAY);
		updateStatusBoard();
		break;
	case KEY_GUEST_DOWN:
		Serial.println("IR: GUEST -1");
		Score_guest--;
		if (Score_guest > 99) { Score_guest = 0; }
		Serial.println(Score_guest);
		score_to_buf();
		myLED.print(Display_buf, INVERT_DISPLAY);
		updateStatusBoard();
		break;
	case KEY_P1:
        //Send Reset 24 instruction to the shoot clock
        Serial1.write(0XA1); Serial1.write(0X51);
		Serial.println("IR: P1");
		break;
	case KEY_P2:
        //Send Reset 14 instruction to the shoot clock
        Serial1.write(0XA1); Serial1.write(0X52);
		Serial.println("IR: P2");
		break;
    case KEY_CLR:
        Serial.println("IR: CLEAR SCORES");
        Score_host = 0;
        Score_guest = 0;
        score_to_buf();
        myLED.print(Display_buf, INVERT_DISPLAY);
        updateStatusBoard();
		break;
    default:
        Serial.print("IR: Unknown command 0x");
        Serial.println(command, HEX);
        break;
    }
}