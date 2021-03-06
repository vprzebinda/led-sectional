#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <vector>
#include <map>
using namespace std;

#define FASTLED_ESP8266_RAW_PIN_ORDER

#define WIND_THRESHOLD 25 // Maximum windspeed for green
#define LIGHTNING_INTERVAL 5000 // ms - how often should lightning strike; not precise because we sleep in-between
#define DO_LIGHTNING true // Lightning uses more power, but is cool.
#define DO_WINDS true // color LEDs for high winds

#define SERVER "www.aviationweather.gov"
#define BASE_URI "/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&hoursBeforeNow=3&mostRecentForEachStation=false&stationString="

#define DEBUG false

const char ssid[] = "";        // your network SSID (name)
const char pass[] = "";    // your network password (use for WPA, or use as key for WEP)
boolean ledStatus = true; // used so leds only indicate connection status on first boot, or after failure

int status = WL_IDLE_STATUS;

#define READ_TIMEOUT 15 // Cancel query if no data received (seconds)
#define WIFI_TIMEOUT 60 // in seconds
#define RETRY_TIMEOUT 15000 // in ms

#define REQUEST_INTERVAL 900000 // in ms (15 min is 900000)

// Define the array of leds
CRGB* leds;
#define DATA_PIN 5
#define LED_TYPE WS2812B
#define COLOR_ORDER RGBb
#define BRIGHTNESS 50

const char* airportString PROGMEM = "KSAN,KLAX,KSBA,KSBP,KMRY,KSFO,KSTS,KACV,KCEC,KOTH,KEUG,KAST,KUIL,KBLI,KSEA,KPDX,KRDM,KMFR,KLMT,KRDD,KCIC,KSMF,KMOD,KFAT,KBFL,KSLC,KPSP,KNYL,KIFP,KLAS,KMMH,KRNO,KBNO,KBKE,KALW,KPSC,KYKM,KEAT,KGEG,KLWS,KBOI,KWMC,KEKO,KCDC,KSGU,1G4,KPHX,KTUS,KFLG,KGCN,KPGA,KPVU,KTWF,KSUN,KMSO,KGPI,KGTF,KHLN,KBTM,KPIH,KIDA,KJAC,KBZN,KBIL,KCOD,KRIW,KRKS,KGJT,KMTJ,KDRO,KTCS,KELP,KABQ,KSAF,KGUC,KASE,KHDN,KCPR,KGCC,KMLS,KSDY,KXWA,KMOT,KDIK,KRAP,KBFF,KLAR,KCYS,KFNL,KDEN,KCOS,KROW,KHOB,KMAF,KLBB,KAMA,KGCK,KPIR,KBIS,KABR,KFAR,KGFK,KBRD,KBJI,KFSD,KSUX,KOMA,KLNK,KGRI,KMHK,KICT,KOKC,KLAW,KSPS,KABI,KSJT,KLRD,KMFE,KBRO,KCRP,KSAT,KAUS,KGRK,KACT,KDFW,KTUL,KFOE,KMCI,KDSM,KALO,KRST,KMSP,KSTC,KINL,KHIB,KDLH,KEAU,KLSE,KDBQ,KCID,KMLI,KUIN,KCOU,KSGF,KJLN,KXNA,KFSM,KTXK,KTYR,KCLL,KIAH,KSHV,KLIT,KSTL,KSPI,KPIA,KRFD,KMSN,KCWA,KRHI,KIWD,KCMX,KIMT,KGRB,KOSH,KMKE,KORD,KBMI,KCMI,KMWA,KPAH,KMEM,KMLU,KAEX,KBPT,KLCH,KLFT,KBTR,KJAN,KCKV,KEVV,KIND,KSBN,KAZO,KGRR,KLAN,KFNT,KMBS,KTVC,KESC,KSAW,KCIU,KPLN,KAPN,KDTW,KTOL,KFWA,KSDF,KBNA,KGTR,KMEI,KMSY,KGPT,KMOB,KPNS,KMGM,KBHM,KHSV,KCHA,KLEX,KCVG,KDAY,KCMH,KCLE,KERI,KFKL,KYNG,KHTS,KTYS,KATL,KCSG,KDHN,KECP,KTLH,KVLD,KABY,KMCN,KGSP,KAVL,KTRI,KCRW,KMGW,KPIT,KLBE,KBUF,KROC,KART,KBTV,KPQI,KBGR,KRKD,KPWM,KLEB,KALB,KSYR,KBGM,KELM,KIPT,KUNV,KHGR,KIAD,KCHO,KLYH,KROA,KCLT,KCAE,KAGS,KGNV,KTPA,KSRQ,KRSW,KEYW,MYNN,KMIA,KPBI,KMLB,KMCO,KDAB,KSGJ,KJAX,KBQK,KSAV,KCHS,KMYR,KFLO,KFAY,KRDU,KGSO,KRIC,KBWI,KMDT,KAVP,KSWF,KBDL,KORH,KMHT,KBOS,KPVD,KMVY,KHVN,KJFK,KABE,KPHL,KACY,KSBY,KORF,KMQI,KPGV,KOAJ,KILM";

#define SSLBUFSIZE 512
class SSLBuffer {
  public:
    SSLBuffer(BearSSL::WiFiClientSecure* client) {
      client_ = client;
      i_ = 0;
      total_read_ = 0;
    }
    int connected() {
      return client_->connected();
    }
    bool available() {
      return client_->available() > 0;  
    }
    char read() {
      while (!available() && connected()) {
        yield();
      }
      if (i_ >= total_read_) {
        i_ = 0;
      }
      if (i_ == 0) {
        total_read_ = client_->read(buffer_, SSLBUFSIZE);
        if (total_read_ < 0) {
          Serial.println("Read bogus data");
        }
      }
      //Serial.write(buffer_[i_]);
      return buffer_[i_++];
    }
    BearSSL::WiFiClientSecure* client_;
    int i_;
    int total_read_;
    uint8_t buffer_[SSLBUFSIZE];
};

class AirportState {
  public:
    AirportState() {
      Clear();
    }

    void Clear() {
      iaco_.clear();
      time_.clear();
      condition_ = NONE;
      wind_ = 0;
      gusts_ = 0;
      lightning_ = false;
    }

    void setCategory(const String& cat) {
      if (cat == "VFR") {
        condition_ = VFR;
      }
      if (cat == "MVFR") {
        condition_ = MVFR;
      }
      if (cat == "IFR") {
        condition_ = IFR;
      }
      if (cat == "LIFR") {
        condition_ = LIFR;
      }
    }
    void setWind(const String& w) {
      wind_ = w.toInt();
    }
    void setGust(const String& w) {
      gusts_ = w.toInt();
    }

    bool complete() {
      return condition_ != NONE;
    }

    CRGB getColor() {
      if (lightning_ && (millis() / 500 % 10) == 0) {
        return CRGB::Black;
      }
      CRGB color;
      if (condition_ == LIFR) color = CRGB::Magenta;
      else if (condition_ == IFR) color = CRGB::Red;
      else if (condition_ == MVFR) color = CRGB::Blue;
      else if (condition_ == VFR) {
        if ((wind_ > WIND_THRESHOLD || gusts_ > WIND_THRESHOLD) && DO_WINDS) {
          color = CRGB::Yellow;
        } else {
          color = CRGB::Green;
        }
      } else color = CRGB::Black;

      return color;
    }

    String condition() {
      switch (condition_) {
        case VFR: return "VFR";
        case MVFR: return "MVFR";
        case IFR: return "IFR";
        case LIFR: return "LIFR";
      }
      return "Unknown";
    }

    String wind() {
      return String(wind_, DEC);
    }

    String gust() {
      return String(gusts_, DEC);
    }

    enum {
      NONE,
      VFR,
      MVFR,
      IFR,
      LIFR
    } condition_;
    String iaco_;
    int wind_;
    int gusts_;
    bool lightning_;
    String time_;
};

int numAirports() {
  int commas = 0;
  for (const char* a = airportString; *a; ++a) {
    if (*a == ',') {
      commas++;
    }
  }
  return commas + 1;
}

int airportIndex(const String& iaco) {
  int commas = 0;
  for (const char* a = airportString; *a; ++a) {
    if (*a == ',') {
      commas++;
    } else {
      // optimize me.
      if (strncmp(iaco.c_str(), a, iaco.length()) == 0) {
        //Serial.println("Index for " + iaco + String(commas, DEC));
        return commas;
      }
    }
  }
  Serial.println("Can't find airport index for " + iaco);
  return -1;
}


AirportState* wx;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT); // give us control of the onboard LED
  digitalWrite(LED_BUILTIN, LOW);

  leds = new CRGB[numAirports()];
  wx = new AirportState[numAirports()];

  // Initialize LEDs
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, numAirports());
  FastLED.setBrightness(BRIGHTNESS);
}

unsigned long next_update = 0;
void loop() {
  digitalWrite(LED_BUILTIN, LOW); // on if we're awake
  int c;

  // Connect to WiFi. We always want a wifi connection for the ESP8266
  if (WiFi.status() != WL_CONNECTED) {
    if (ledStatus) fill_solid(leds, numAirports(), CRGB::Orange); // indicate status with LEDs, but only on first run or error
    FastLED.show();
    WiFi.mode(WIFI_STA);
    WiFi.hostname("LED Sectional " + WiFi.macAddress());
    //wifi_set_sleep_type(LIGHT_SLEEP_T); // use light sleep mode for all delays
    Serial.print("WiFi connecting..");
    WiFi.begin(ssid, pass);
    // Wait up to 1 minute for connection...
    for (c = 0; (c < WIFI_TIMEOUT) && (WiFi.status() != WL_CONNECTED); c++) {
      Serial.write('.');
      delay(1000);
    }
    if (c >= WIFI_TIMEOUT) { // If it didn't connect within WIFI_TIMEOUT
      Serial.println("Failed. Will retry...");
      fill_solid(leds, numAirports(), CRGB::Orange);
      FastLED.show();
      ledStatus = true;
      return;
    }
    Serial.println("OK!");
    if (ledStatus) fill_solid(leds, numAirports(), CRGB::Purple); // indicate status with LEDs
    FastLED.show();
    ledStatus = false;
  }

  if (millis() > next_update) {
    getMetars();
    next_update = millis() + REQUEST_INTERVAL;
  }
  for (int i = 0; i < numAirports(); ++i) {
    leds[i] = wx[i].getColor();
  }
  FastLED.show();
}


bool readNextFmt(SSLBuffer* client, char* f, String* result) {
  *result = String();
  char* startover = f;
  bool copying = false;
  char c;
  for (; *f && client->connected() && (c = client->read());) {
    if (*f == 'X') {
      copying = true;
      f++;
    }
    if (c == *f) {
      f++;
      copying = false;
    } else if (copying) {
      *result += c;
    } else {
      // start over
      f = startover;
      copying = false;
      *result = String();
    }
  }
  if (*f == '\0') {
    return true;
  }
  
  return false;
}

void getAirportState(SSLBuffer* client, AirportState* ret) {
  String token;
  bool found_station = false;
  for (; readNextFmt(client, "<X>", &token);) {
    if (token == "/METAR") {
      break;
    }
    if (token == "station_id") {
      readNextFmt(client, "X<", &ret->iaco_);
      if (found_station) {
        Serial.println("Found station twice " + ret->iaco_);
      }
      found_station = true;
    }
    if (token == "flight_category") {
      String category;
      readNextFmt(client, "X<", &category);
      ret->setCategory(category);
    }
    // not enough memory for this
//    if (token == "observation_time") {
//      readNextFmt(client, "X<", &ret->time_);
//    }
    if (token == "wind_speed_kt") {
      String w;
      readNextFmt(client, "X<", &w);
      ret->setWind(w);
    }
    if (token == "wind_gust_kt") {
      String w;
      readNextFmt(client, "X<", &w);
      ret->setGust(w);
    }
    if (token == "raw_text") {
      String w;
      readNextFmt(client, "X<", &w);
      int ts_idx = w.indexOf("TS");
      int rmk_idx = w.indexOf("RMK") == -1;
      if (ts_idx != -1 && (rmk_idx == -1 || ts_idx < rmk_idx)) {
        ret->lightning_ = true;
      }
    }
  }
}

#define LINE_BUFFER_SIZE 300
bool getMetars() {
  //lightningLeds.clear(); // clear out existing lightning LEDs since they're global
  //fill_solid(leds, numAirports(), CRGB::Black); // Set everything to black just in case there is no report
  uint32_t t;

  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  //Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (!client.connect(SERVER, 443)) {
    Serial.println("Connection failed!");
    client.stop();
    return false;
  } else {
    Serial.println("Connected ...");
    Serial.print("GET ");
    Serial.print(BASE_URI);
    Serial.print(airportString);
    Serial.println(" HTTP/1.1");
    Serial.print("Host: ");
    Serial.println(SERVER);
    Serial.println("Connection: close");
    Serial.println();
    // Make a HTTP request, and print it to console:
    client.print("GET ");
    client.print(BASE_URI);
    client.print(airportString);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(SERVER);
    client.println("Connection: close");
    client.println();
    client.flush();
    t = millis(); // start time
    FastLED.clear();

    //Serial.println("Getting data");

    while (!client.connected()) {
      if ((millis() - t) >= (READ_TIMEOUT * 1000)) {
        Serial.println("---Timeout---");
        client.stop();
        return false;
      }
      Serial.print(".");
      delay(1000);
    }

    Serial.println("parsing data");
    SSLBuffer client_wrapper(&client);

    for (int i = 0; i < numAirports(); ++i) {
      wx[i].Clear();
    }
    AirportState state;
    while (client.connected()) {
      String tmp;
      if (readNextFmt(&client_wrapper, "<METAR>", &tmp)) {
        state.Clear();
        getAirportState(&client_wrapper, &state);
        int idx = airportIndex(state.iaco_);
        //Serial.println("Considering airport state for " + state.iaco_ + " " + state.condition() + " complete? " + String(state.complete(), DEC) + " existing complete? " + String(wx[idx].complete(), DEC));
        if (state.complete() && !wx[idx].complete() && idx >= 0) {
          Serial.println("Setting airport state for " + state.iaco_ + " index " + String(idx, DEC));
          wx[idx] = state;
        }
      }
      wdt_reset();
    }
    
    Serial.println("done");
  }

  // Dump results
  Serial.println(numAirports());
  for (int i = 0; i < numAirports(); ++i) {
    wdt_reset();
    AirportState& as = wx[i];
    Serial.println(as.iaco_ + " " + as.condition() + " " + as.wind() + " " + as.gust());
  }
  client.stop();
  return true;
}
