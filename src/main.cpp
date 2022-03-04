#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <config.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <WebServer.h>
#include <AsyncTCP.h>

#define ONE_WIRE_BUS 4
#define TEMPERATURE_PRECISION 10

//---------------------------- sonde température -----------------------------------------

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

// Tableaux contenant l'adresse de chaque sonde OneWire

DeviceAddress insideThermometer = {0x28, 0xAA, 0x32, 0x4, 0x1B, 0x13, 0x2, 0xA};

//----------------------------------------------------------------------------------------

#define wifi_max_try 20        // Nombre de tentative au resau WiFI
#define duration_deep_sleep 20 // sinon active le deep-sleep jusqu a la prochaine tentative de reconnexion

HTTPClient httpclient;
WiFiClient client;
AsyncWebServer server(80);

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void print_wifi_error()
{
  switch (WiFi.status())
  {
  case WL_IDLE_STATUS:
    Serial.println("WL_IDLE_STATUS");
    break;
  case WL_NO_SSID_AVAIL:
    Serial.println("WL_NO_SSID_AVAIL");
    break;
  case WL_CONNECT_FAILED:
    Serial.println("WL_CONNECT_FAILED");
    break;
  case WL_DISCONNECTED:
    Serial.println("WL_DISCONNECTED");
    break;
  default:
    Serial.printf("No know WiFi error");
    break;
  }
}

//-------------------------------------- Déclaration de pin en sortie ----------------------------------

const int RelaisStart = 26;
const int RelaisJeton = 27;
const int RelaisIR = 25;
const int RelaisReset = 33;
const int EtatWifi = 2;

//----------------------------------------Déclaration des pin en entrée -----------------------------------

const int Etat = 16;
const int DetectJeton1 = 17;
const int Optocoupleur = 13;

//----------------------------------------Déclaration des variables -------------------------------------------

String OnOff = "";
signed char varessorage = 0;
signed char varjeton = 0;
signed char jeton1 = 0;
signed char temp1 = 0;
signed char etat1 = 0;
signed char varReset = 0;
signed char varEtat = 0;
signed char varAjoutJeton = 0;
bool temperatures = false;

// **************************************Déclaration variables jeedom********************************************
bool varJeedom = true;

//****************************************Déclaration des variables antirebond ***********************************

bool didStatus = false;
bool oldDidStatus = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

//----------------------------------------Fonction ajout jetons --------------------------------------------------

void jeton()
{
  for (int j = 0; j <= NbJETON; j++)

  {
    jeton1 = jeton1 + 1;
    Serial.print("Jeton inserer ");
    Serial.println(j + 1);
    digitalWrite(RelaisJeton, HIGH);
    delay(100);
    digitalWrite(RelaisJeton, LOW);
    delay(2000);
  }

  delay(100);
  Serial.println();
  Serial.println("===== Fin ajout jeton ===== ");
  Serial.println();
  delay(2000);
  varjeton = 0;
}

//----------------------------------------Fonction reset --------------------------------------------------

void Reset()

{
  Serial.println("Reset  activer");
  Serial.println("Attendre 5 secondes ");
  digitalWrite(RelaisReset, HIGH);
  delay(5000);
  digitalWrite(RelaisReset, LOW);
  Serial.println("Fin du Reset  ");
}

//----------------------------------------Fonction essorage --------------------------------------------------

void essorage1()

{

  Serial.println("===== Départ du programme essorage =====");
  Serial.println();

  jeton();

  delay(2000);
  Serial.println("===== Appui de la touche start ====== ");
  Serial.println();
  digitalWrite(RelaisStart, HIGH);
  delay(100);
  digitalWrite(RelaisStart, LOW);
  delay(10000);

  Serial.println("===== Activation mode infrarouge ===== ");
  Serial.println();
  digitalWrite(RelaisIR, HIGH);
  delay(2000);

  Serial.println("===== debut boucle impulsion =====");
  Serial.println();
  for (int j = 0; j <= 8; j++)

  {
    Serial.print("impulsion ");
    Serial.println(j + 1);
    digitalWrite(RelaisStart, HIGH);
    delay(IMPULSE);
    digitalWrite(RelaisStart, LOW);
    delay(2000);
  }

  Serial.println();
  Serial.println("===== Arret mode infrarouge ====== ");
  Serial.println();
  digitalWrite(RelaisIR, LOW);
  delay(2000);
  Serial.println("===== Fin du programme essorage ======");

  varessorage = 0;
}

void setup()

{

  //------------------- Affectation IP Fixe-----------------

  IPAddress ip(192, 168, 1, y);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(8, 8, 8, 8);

  //---------------------Config wifi ------------------------

  //Explique la raison du reveil de l ESP32

  print_wakeup_reason();

  // Mode de connexion
  // WiFi.hostname("Machine 1");

  WiFi.mode(WIFI_STA);

  // Configuration pour l'IP Fixe
  WiFi.config(ip, gateway, subnet, dns);

  // Démarrage de la connexion

  WiFi.begin(ssid, password);
  Serial.print("Tentative de connexion...");
  //int count_try = 0;

  //----------------------------------------------------Serial------------------------------------------------------
  Serial.begin(115200);
  Serial.println("\n");

  //----------------------------------------------------OneWire-----------------------------------------------------

  sensors.begin();
  sensors.setResolution(insideThermometer, TEMPERATURE_PRECISION);

  //------------Configuration GPIO-----------------

  pinMode(RelaisStart, OUTPUT);
  pinMode(RelaisJeton, OUTPUT);
  pinMode(RelaisIR, OUTPUT);
  pinMode(RelaisReset, OUTPUT);
  pinMode(Etat, INPUT_PULLUP);
  pinMode(DetectJeton1, INPUT_PULLUP);
  pinMode(Optocoupleur, INPUT_PULLUP);
  pinMode(EtatWifi, OUTPUT),

      //--------------Affectations état GPIO--------------

      digitalWrite(RelaisStart, LOW);
  digitalWrite(RelaisJeton, LOW);
  digitalWrite(RelaisIR, LOW);
  digitalWrite(RelaisReset, LOW);
  digitalWrite(EtatWifi, LOW);

  //----------------------------------------------------SPIFFS-------------------------------------------------------

  if (!SPIFFS.begin())

  {
    Serial.println("Erreur SPIFFS...");
    return;
  }

  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file)
  {
    Serial.print("File: ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile();
  }

  //----------------------------------------------------SERVER -----------------------------------

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/w3.css", "text/css"); });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/script.js", "text/javascript"); });

  //----------------------------------Etat Wifi---------------------------------------------------------

  Serial.println("\n");
  Serial.println("Connexion etablie!");
  Serial.println("Adresse IP : " + WiFi.localIP().toString());
  Serial.println("Passerelle IP : " + WiFi.gatewayIP().toString());
  Serial.println("DNS IP : " + WiFi.dnsIP().toString());
  Serial.print("Puissance de réception : ");
  Serial.println(WiFi.RSSI());
  digitalWrite(EtatWifi, HIGH);

  //--------------------------------  Affichage des jetons dans index.html ------------------------------

  server.on("/NombreJeton", HTTP_GET, [](AsyncWebServerRequest *request)

            {
              int val = (jeton1);
              String Jeton = String(val);
              request->send(200, "text/plain", Jeton);
            });

  //--------------------------------  Affichage de l'état de la machine dans index.htms' ------------------------------

  server.on("/NombreEtat", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              // int val = (varEtat);
              //String OnOff = String(val);
              request->send(200, "text/plain", OnOff);
            });

  //---------------------------------- Commande du bouton esssorage -----------------------------------------------

  server.on("/essorage", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              varessorage = 1;
              request->send(204);
            });

  //---------------------------------- Commande du bouton ajout de jetons -----------------------------------------------

  server.on("/jeton", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              varjeton = 1;
              request->send(204);
            });

  //---------------------------------- Commande du bouton reset -----------------------------------------------

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              varReset = 1;
              request->send(204);
            });

  //---------------------------------- Commande du bouton stop -----------------------------------------------

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              digitalWrite(RelaisReset, HIGH);
              request->send(204);
            });

  server.begin();
  Serial.println("===== Serveur actif! ======");
  Serial.println();
  digitalWrite(2, HIGH);
}

//----------------------------------------Fonction température --------------------------------------------------

void printTemperature(String label, DeviceAddress deviceAddress)

{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print(label);

  if (tempC == -127.00)

  {
    Serial.print("Erreur lors de l'obtention de la température");
  }

  else

  {

    Serial.print(tempC);
    Serial.println(" C°");
    temp1 = tempC;
  }
}

//---------------------------------------------------Fonction antirebon ---------------------------------

void Antirebond()

{

  int reading = digitalRead(DetectJeton1);
  //int reading = digitalRead(Optocoupleur);

  if (reading != oldDidStatus)
  {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) >= debounceDelay)
  {
    if (reading != didStatus)
    {
      didStatus = reading;
      Serial.print(F("Detect jetons  : "));
      Serial.println(didStatus);

      if (reading == 1)
      {
        jeton1 = jeton1 + 1;
        Serial.println(jeton1);
        String url = ("http://" + String(HOST_JEEDOM) + "/core/api/jeeApi.php?apikey=" + API_KEY + "&type=virtual&type=virtual&id=" + ID_JETON + "&value=" + (jeton1)); //"&value=" + (etat? "1":"0");

        httpclient.begin(client, url);
        httpclient.GET();
      }
    }
  }
  oldDidStatus = reading;
}

//--------------------------------Boucle lecture de la température --------------------------------------

void temperature()

{

  if (temperatures == true)

  {

    sensors.requestTemperatures();

    printTemperature("  ", insideThermometer);

    delay(1000);
  }
}

void loop()

{

  Antirebond();

  temperature();

  //********************************* condition pour changement d'état ON/OFF *******************************

  varEtat = digitalRead(Etat);

  if (varEtat == 1)

  {

    jeton1 = 0;
    OnOff = "Machine en service";
    if (varJeedom == 1)

    //****************** Connexion a API Jeedom si la machine est en service ******************************

    {
      String url = ("http://" + String(HOST_JEEDOM) + "/core/api/jeeApi.php?apikey=" + String(API_KEY) + "&type=virtual&type=virtual&id=" + String(ID) + "&value=1");

      httpclient.begin(client, url);
      httpclient.GET();

      Serial.println(url);

      varJeedom = 0;

      String url2 = ("http://" + String(HOST_JEEDOM) + "/core/api/jeeApi.php?apikey=" + String(API_KEY) + "&type=virtual&type=virtual&id=" + String(ID_JETON) + "&value=" + (jeton1)); //"&value=" + (etat? "1":"0");

      httpclient.begin(client, url2);
      httpclient.GET();
    }
  }

  else

  {

    OnOff = "Machine en Veille";

    if (varJeedom == 0)

    //*************** Connexion a API Jeedom si la machine est en veille *****************************

    {
      String url = ("http://" + String(HOST_JEEDOM) + "/core/api/jeeApi.php?apikey=" + String(API_KEY) + "&type=virtual&type=virtual&id=" + String(ID) + "&value=0");

      httpclient.begin(client, url);
      httpclient.GET();

      Serial.println(url);

      varJeedom = 1;
    }
  }

  //********************************* condition pour lancer l'essorage *******************************

  if (varessorage == 1)

  {
    essorage1();
    Serial.println(varessorage);
    varessorage = 0;
  }

  //********************************* condition ajouter des Jetons *******************************

  if (varjeton == 1)

  {

    jeton();

    varjeton = 0;
  }

  //********************************************** condition Reset *******************************

  if (varReset == 1)

  {
    Reset();

    varReset = 0;
  }
}
