#include <Pinger.h>
#include <PingerResponse.h>
#include <OneWire.h> //Librairie du bus OneWire
#include <math.h>
#include <DallasTemperature.h> //Librairie du capteur
#include "FS.h"
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include "ESP8266WiFi.h"
#include <ArduinoJson.h>
#include <ESP8266FtpServer.h>
FtpServer ftpSrv;
#include "meteo.h"

#include <ESP8266HTTPClient.h>
#include "bsec.h" // Librairie Bosh BME680
#include "boutton.h"
#define BOUNCE_DELAY  5 // pause anti-rebonds = 5 ms
#define Vert D6
#define Rouge D7
#define Bleu  D8
#define ds18b20 D5
#define PIN_RESET_BUTTON A0
#define RELAIS D3



Boutton ResetBoutton(10, BOUNCE_DELAY);
Boutton PlusBoutton(64, BOUNCE_DELAY);
Boutton MoinsBoutton(115, BOUNCE_DELAY);

// Create an object of the class Bsec https://github.com/BoschSensortec/BSEC-Arduino-library
Bsec iaqSensor;

// Set global to avoid object removing after setup() routine
Pinger pinger;

ESP8266WebServer server(80); //server web
OneWire oneWire(ds18b20); //Bus One Wire sur la pin 2 de l'arduino
DallasTemperature sensors(&oneWire); //Utilistion du bus Onewire pour les capteurs
DeviceAddress sensorDeviceAddress; //Vérifie la compatibilité des capteurs avec la librairie
WiFiManager wifiManager;

/* Couleurs (format RGB) */
byte COLOR_BLACK[3] = {0, 0, 0};
byte COLOR_RED[3] = {255, 0, 0};
byte COLOR_GREEN[3] = {0, 100, 0};
byte COLOR_BLUE[3] = {0, 0, 255};
byte COLOR_MAGENTA[3] = {255, 0, 255};
byte COLOR_CYAN[3] = {0, 100, 255};
byte COLOR_YELLOW[3] = {255, 100, 0};
byte COLOR_WHITE[3] = {255, 100, 255};
byte *couleur_fonction = &COLOR_GREEN[0];
byte *couleur_mesure = &COLOR_BLUE[0];
byte *couleur_error = &COLOR_RED[0];
byte *couleur_forcee = &COLOR_MAGENTA[0];
byte *couleur_haut = &COLOR_YELLOW[0];
byte *couleur_statut;
const byte luminosite = 20; //pourcentage


const unsigned long appuilong = 3000; //délais d'appui sur le boutton reset en ms
unsigned long millisStart; // calcul du délais du boutton reset

WiFiUDP ntpUDP; // Define NTP Client to get time
NTPClient timeClient(ntpUDP, "pool.ntp.org" ); //, utcOffsetInSeconds); heure

unsigned long DateMesure;
unsigned long PrevDateMesure;
short int temp=0; // mesure de la température

short int tab_temp[2];
bool tab_statut[2];
short int tab_mesure[10];
byte nbenr = 0;
bool StatutWifi = true ;
//IPAddress gateway;
bool shouldSaveConfig = false;
String URL_MAJ;
char domaine[100] ="votre domaine";
char user_web[12] = "Utilisateur";
char mdp_web[13] = "Mot de passe";
 
const unsigned long DelaisTestWifi = 5*60*1000; // Délais de contrôle du wifi
const short int DelaisSecu = 15*60; // Délais de sécurité entre un allumage et l'extinction en seconde
const unsigned long DelaisMesure = 10 * 1000; // ecart entre deux mesures
unsigned long MillisTestWifi = millis();
unsigned long MillisMesure = MillisTestWifi; // calcul des délais mesures

#define FILE_STATUT "/statut.json"
const size_t capacity_statut = JSON_OBJECT_SIZE(9) + 100;
bool Mf, S = 0;
unsigned short int Tc = 200;
signed short int Tob = -5;
byte Toh = 5; // décalage de déclanchement en °
unsigned long Secu = 0;

#define HISTORY_FILE_TOTAL "/historytotal.json"
const byte TailleEnregistrement = 19;//taille de la chaine + 1 (retour chariot)
const short int NbEnregistrement =  10080; // 7j x 24h x 60min ; 
const unsigned long DelaisEnr = 1000 * 60; // ecart entre deux sauvegardes
unsigned long MillisEnr =  MillisTestWifi; // calcul des délais d'enregistrement

#define HISTORY_METEO_HEURE "/historymeteoheure.json"
#define HISTORY_METEO_QUOTIDIEN "/historymeteoquotidien.json"
#define HISTORY_METEO_HEBDO "/historymeteohebdo.json"
//const short int     SizeHistMeteoHeure = (60 * 60 * 1000) / DelaisEnr;
Meteo Meteoheure(60, HISTORY_METEO_HEURE); //60 enregistrement
Meteo Meteoquotidien(96, HISTORY_METEO_QUOTIDIEN); //96 un enregistrement toutes les 15mins
Meteo Meteohebdo(10, HISTORY_METEO_HEBDO);
const unsigned long DelaisEnrQuo = 1000 * 60 * 15; // ecart entre deux sauvegardes
unsigned long MillisEnrQuo =  MillisTestWifi; // calcul des délais d'enregistrement
const unsigned long DelaisEnrHebdo = 1000 * 60 * 60; // Toutes les heures
unsigned long MillisEnrHebdo =  MillisTestWifi; // calcul des délais d'enregistrement
unsigned int humidite,  humiditehebdo = 0;
unsigned int pression, pressionhebdo = 0 ;
unsigned int IAQ, IAQhebdo = 0;
byte nbenrmeteo, nbenrmeteohebdo = 0;


bool RESET = true; // statut du boutton RESET
bool StateBoutton = false; // statut wifi 


void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void sendMesures() {
    String message = "[{\"d\":" + (String)DateMesure + ",\"t\":";
    message += inttofloat(temp) ;
    message += ",\"iaq\":" + (String)(iaqSensor.iaq);
    message += ",\"pression\":" + (String)(iaqSensor.pressure/100);
    message += ",\"h\":" + (String)(iaqSensor.humidity);
    message += "}]";
    server.send(200, "application/json", message);
    //Serial.println("Mesures envoyees");
    //Serial.println(message);
}

void loadStatut(){
  StaticJsonDocument<capacity_statut> Tab_Statut; //calcul via https://arduinojson.org/v6/assistant/
  File file = SPIFFS.open(FILE_STATUT, "r");
  if (!file){
    Serial.println("Aucun statut existe");
    updatestatut ();
  } else {
    if (  file.size()== 0 ) {
      Serial.println("Fichier statut vide !");
      updatestatut ();
    } else {
      DeserializationError error = deserializeJson(Tab_Statut, file);
      if (error) {
        Serial.println("Impossible de lire le JSON");
        Serial.println(error.c_str());
      } else {
        Mf= Tab_Statut["mf"];
        Tc=floattoint(Tab_Statut["tc"]);
        Tob=floattoint(Tab_Statut["b"]);
        Toh=floattoint(Tab_Statut["h"]);
        Secu= 0;
        S = 0; // au chargement on suppose que le poele est arreté (coupure de courant)
        Tab_Statut["s"] = 0;
        serializeJson(Tab_Statut,file);
        Serial.println("Paramètre : \n  Marche Forcée : " + (String)Mf + "\n  Température Consigne : " + (String)inttofloat(Tc) +"\n  Température Offset bas : " + (String)inttofloat(Tob) +"\n  Température Offset Haut : " + (String)inttofloat(Toh) + "\n  Statut : " + (String)S + "\n  Sécurité : " + (String)Secu );
      }
    }
  }
  file.close();
}
void updatestatut () {
  StaticJsonDocument<capacity_statut> Tab_Statut; //calcul via https://arduinojson.org/v6/assistant/
  File file = SPIFFS.open(FILE_STATUT, "w");
  Tab_Statut["mf"] = Mf;
  if (Tc <100) {
    Tc = 100;
  } else if (Tc>300){
    Tc=300;
  }
  Tab_Statut["tc"] = inttofloat(Tc);
  Tab_Statut["b"] = inttofloat(Tob);
  Tab_Statut["h"] = inttofloat(Toh);
  Tab_Statut["s"] = S;
  Tab_Statut["se"] = Secu;
 // Tab_Statut["ip"] = IpAdress2String (WiFi.localIP());
  Tab_Statut["ip"] = WiFi.localIP().toString();
  Tab_Statut["rssi"] = WiFi.RSSI();
  Tab_Statut["ssid"] = WiFi.SSID();
  serializeJson(Tab_Statut,file);  
  file.close();
}


void majstatut(){
  char json[1000];
  File file = SPIFFS.open(FILE_STATUT, "w");
  StaticJsonDocument<capacity_statut> Tab_Statut_temp; //calcul via https://arduinojson.org/v6/assistant/
  float tmp;
  if (server.hasArg("plain")== false){ //Check if body received
    server.send(200, "text/plain", "Body not received");
    return;
  }
  DeserializationError error = deserializeJson(Tab_Statut_temp, server.arg("plain"));
  if (error) {

    Serial.println("Impossible de lire le JSON - Impossible to read JSON file");
    Serial.println(error.c_str());
    server.send(200, "text/plain", "Json incorecte");
  } else {

    tmp = Tab_Statut_temp["tc"];
    if (tmp != 0){
     Tc = floattoint(Tab_Statut_temp["tc"]);
    }
    Mf= Tab_Statut_temp["mf"] | Mf;
    if (Tab_Statut_temp["b"]<=-0.5){
      Tob = floattoint(Tab_Statut_temp["b"]);
    }  
    if (Tab_Statut_temp["h"]>=0.5){
      Toh = floattoint(Tab_Statut_temp["h"]);
    } 
    S=Tab_Statut_temp["s"] | S;
    Serial.println("Paramètre : \n  Marche Forcée : " + (String)Mf + "\n  Température Consigne : " + (String)inttofloat(Tc) +"\n  Température Offset bas : " + (String)inttofloat(Tob) +"\n  Température Offset Haut : " + (String)inttofloat(Toh) + "\n  Statut : " + (String)S + "\n  Sécurité : " + (String)Secu );
    
    Tab_Statut_temp["mf"] = Mf;
    Tab_Statut_temp["tc"] = inttofloat(Tc);
    Tab_Statut_temp["b"] = inttofloat(Tob);
    Tab_Statut_temp["h"] = inttofloat(Toh);
    Tab_Statut_temp["s"] = S;
    Tab_Statut_temp["se"] = Secu;
//    Tab_Statut_temp["ip"] = IpAdress2String (WiFi.localIP());
    Tab_Statut_temp["ip"] = WiFi.localIP().toString();
    Tab_Statut_temp["rssi"] = WiFi.RSSI();
    Tab_Statut_temp["ssid"] = WiFi.SSID();
    serializeJson(Tab_Statut_temp,json);
    server.send(200, "application/json", json);
    updatestatut ();
  }
  file.close();
}

void insmesure(){
// Format du fichier Timestamp date mesure, temperature , statut poele
  File file ;
  String buffer;
  int dateenr;
  char char_array[TailleEnregistrement];

  if (tab_statut[0] == tab_statut[1] && tab_statut[1] == S && tab_temp[0] == tab_temp[1] && tab_temp[1] == temp ){ // test pour MAJ ou insertion
    file = SPIFFS.open(HISTORY_FILE_TOTAL, "r+");
    //maj fichier
    file.seek((TailleEnregistrement), SeekEnd); //positionnement sur l'enregistrement avant dernier.
    Serial.print("valeur mise à jour : ");
  } else {
    tab_statut[0] = tab_statut[1]; // remplacement de l'enregistrement N+2 par N+1
    tab_temp[0] = tab_temp[1];  
    file = SPIFFS.open(HISTORY_FILE_TOTAL, "a+");
    buffer = file.readStringUntil('\n');// récupération de la premère ligne
    if (!buffer.equals("")){
      buffer.toCharArray(char_array,TailleEnregistrement);
      dateenr = atoi(strtok(char_array, ","));
      Serial.println ("premier enregistrement : " + (String)dateenr);
    }

    // insérer fichier
    if (file.size() >= ((TailleEnregistrement) * NbEnregistrement) || dateenr < (DateMesure - NbEnregistrement * 60) ){ // test taille maximale ou premier enregistrement datant de plus de 7j
      Serial.println("taille du fichier max ou 7J d'enregistrement présent - rotation du fichier");
      File filetemp = SPIFFS.open("/filetemp.txt", "w");
      //file.seek((TailleEnregistrement), SeekSet);
      Serial.println("reecriture du fichier");
      //filetemp.println(buffer);
      while (file.available()) {
        filetemp.print(file.readStringUntil('\n')+"\n");
      }
      file.close();
      Serial.println("Fin de reecriture du fichier et suppression du fichier total");
      SPIFFS.remove(HISTORY_FILE_TOTAL);
      filetemp.close();
      Serial.println("Renommage du fichier");
      SPIFFS.rename("/filetemp.txt",HISTORY_FILE_TOTAL );
      file = SPIFFS.open(HISTORY_FILE_TOTAL, "a+");
    }
    Serial.print("valeur insérée : ");
  }
  // Mise à jour de la date mesure
  tab_statut[1] = S;
  tab_temp[1] = temp;
  
  String message =  (String)DateMesure + ",";
  if (temp >= 100) { // sécurisation de l'écriture du fichier pour avoir toujours la meme taille
    message += inttofloat(temp) ;
  } else if (temp >= 0){ //température entre 0 et 9.99
    message += "0";
    message += inttofloat(temp) ;
  } else {
    message += inttofloat(temp) ;
  }
  message += "," + (String)S;
  message += "\n";

 // message += "}\n";
  if (message.length()!=TailleEnregistrement){ // controle que la taille de l'enregistrment est corercte
    Serial.println("Taille d'enregistrement incorecte : " + (String)message.length() + " - " + (String)TailleEnregistrement);
  }else{
    Serial.println( message);
    file.print(message);
  //Serial.println (file.size ());
  }
  file.close();
}

void sendMesuresHisto (){
  long histodelais;
  String buffer, message;
  char* dateenr;
  bool sortie = false;
  char char_array[TailleEnregistrement];
  long taillelu = 0;
  
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); //envoi d'une réponse pour préciser que la taille est inconnu
  message = "[";
  server.send(200, "application/json", message);
  File file = SPIFFS.open(HISTORY_FILE_TOTAL, "r");
  
  if ( server.arg("hist").toInt() > 0){ //récupération du paramètre envoyé par l'API temps en seconde d'enregistrement
    histodelais = server.arg("hist").toInt();
    if (file.available()&&file.size()>TailleEnregistrement){ // si fichier dispo et au moins un enregistrement
      file.seek((TailleEnregistrement), SeekEnd);
    }
    while (!sortie && (taillelu)<= file.size()){
      message = "";
      buffer = file.readStringUntil('\n');
      if (!buffer.equals("")){
        buffer.toCharArray(char_array,TailleEnregistrement);
        dateenr = strtok(char_array, ",");
          if (atol(dateenr)>(DateMesure - (histodelais))){
            if (taillelu>0){
              message += ",";
            }
            taillelu += buffer.length()+1;
           // mesuretemp = atof(strtok(0,","));
            message += "{\"d\":" + (String)dateenr + ",\"t\":" + (String)atof(strtok(0,",")) + ",\"s\":" + (String)strtok(0,",") + "}" ;
            file.seek(-(TailleEnregistrement + buffer.length()+1), SeekCur);
            server.sendContent(message);
          }else{
            sortie=true;
          }
      } else { 
        taillelu += buffer.length()+1;
        file.seek(-(TailleEnregistrement + buffer.length()+1), SeekCur); 
      }
    }
    
  } else{
    while (file.available()){
      message = "";
      buffer = file.readStringUntil('\n');
      if (!buffer.equals("")){
        if (taillelu>0){
              message += ",";
        }
        buffer.toCharArray(char_array,TailleEnregistrement);
        dateenr = strtok(char_array, ",");
        message += "{\"d\":" + (String)dateenr + ",\"t\":" + (String)atof(strtok(0,",")) + ",\"s\":" + (String)strtok(0,",") + "}" ;
        server.sendContent(message);
        taillelu = 1;
      }
    }
  }
  message = "]";
  server.sendContent(message);
  file.close();
}


void ctrlchauff(){
  bool S_prec = S;
  StaticJsonDocument<capacity_statut> Tab_Statut;
  Serial.println("Temp = " + (String)temp + "seuil bas = " + (String)(Tc+Tob));
  if(DateMesure > Secu && S == true){
    Serial.println("Sécurisation levée");
    Secu=0;
  }
  if (Mf==true && (S == false)){
    //mettre en marche
    S = true;
    couleur_statut=&couleur_forcee[0];
    Secu=DateMesure + DelaisSecu;
    digitalWrite(RELAIS, HIGH);
    Serial.println("Allumage forcée");
  }else if (Mf==true && (S == true)){
    S = true;
    couleur_statut=&couleur_forcee[0];
  }   else if ((temp<Tc+Tob) && (S == false) ) {
    // sous le seuil de consigne on allume le poele et pas de marche force
    S = true;
    couleur_statut=&couleur_haut[0];
    Secu=DateMesure + DelaisSecu;
    digitalWrite(RELAIS, HIGH);
    Serial.println("Allumage - Temp " + (String)inttofloat(temp) + " < Consigne " + (String)(inttofloat(Tc+Tob)));
  } else if ( (temp>Tc+Toh) && (S == true) && (DateMesure > Secu) ) {
    //dépassement de la température de consigne arrêt du poele Pas de Marche focee
    S = false;
    couleur_statut=&couleur_fonction[0];
    digitalWrite(RELAIS, LOW);
    Secu=0;
    Serial.println("Arret - Temp " + (String)inttofloat(temp) + " > Consigne " + (String)inttofloat(Tc+Toh));
  } else if ((S ==true) && (Mf==false)) {
    couleur_statut=&couleur_haut[0];
  }

  if (S_prec != S){
  // changement d'état du poele on pet à jour le json
    Serial.println("changement de statut : " +(String)S);
  }
  updatestatut ();
  displayColor(&couleur_statut[0], luminosite);
}

bool handleFileRead(String path){
  Serial.print(F("handleFileRead: "));
  Serial.println(path);
 
  if (!server.authenticate(user_web, mdp_web)) {
      server.requestAuthentication();
  }
 
  if(path.endsWith("/")) path += F("index.html"); 
    String contentType = getContentType(path);  
    if( SPIFFS.exists(path)){     // If the file exists, either as a compressed archive, or normal
      fs::File file = SPIFFS.open(path, "r");                 // Open the file
      size_t sent = server.streamFile(file, contentType); // Send it to the client
      file.close();                                           // Close the file again
      Serial.println(String(F("\tSent file: ")) + path + String(F(" of size ")) + sent);
    return true;
  }
  Serial.println(String(F("\tFile Not Found: ")) + path);
  return false;                                             // If the file doesn't exist, return false
}

String getContentType(String filename){
  if(filename.endsWith(F(".htm")))          return F("text/html");
  else if(filename.endsWith(F(".html")))    return F("text/html");
  else if(filename.endsWith(F(".css")))     return F("text/css");
  else if(filename.endsWith(F(".js")))      return F("application/javascript");
  else if(filename.endsWith(F(".json")))    return F("application/json");
  else if(filename.endsWith(F(".png")))     return F("image/png");
  else if(filename.endsWith(F(".gif")))     return F("image/gif");
  else if(filename.endsWith(F(".jpg")))     return F("image/jpeg");
  else if(filename.endsWith(F(".jpeg")))    return F("image/jpeg");
  else if(filename.endsWith(F(".svg")))    return F("image/svg+xml");
  else if(filename.endsWith(F(".ico")))     return F("image/x-icon");
  else if(filename.endsWith(F(".xml")))     return F("text/xml");
  return F("text/plain");
}

void setup() {
  StaticJsonDocument<400> configuration; //json temporaire de récupération des parametre Dyndns
  char user[40]="Utilisateur";
  char mdp[13]="Mot de passe";
  //char domainetemp[40]="votre domaine";
  char url[50]="www.ovh.com/nic/update?system=dyndns&hostname=";
 
  Serial.begin(115200); //Permet la communication en serial
  Serial.println("Port série initialisé");
  analogWriteRange(256);
  Serial.println("Nouveau Range PWM initialisé");



  for (int i = 0 ; i<2 ; i++){
    tab_temp[i]=0;
    tab_statut[i]=0;
  }
 
  pinMode(Rouge, OUTPUT);
  pinMode(Vert, OUTPUT);
  pinMode(Bleu, OUTPUT);
  pinMode(PIN_RESET_BUTTON, INPUT);
  pinMode(RELAIS, OUTPUT);
  displayColor(&COLOR_WHITE[0], luminosite);
  digitalWrite(RELAIS, LOW); // on s'assure que le relais est ouvert

  // Démarre le système de fichier SPIFFS
  if (!SPIFFS.begin()){
    Serial.println("Echec du montage SPIFFS");
  } else {
    Serial.println("Succes du montage SPIFFS");

    loadStatut();
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile){
      Serial.println("Aucun statut existe");
    } else {
       if (  configFile.size() == 0 ) {
          Serial.println("Fichier statut vide !");
       } else {
          
          DeserializationError error = deserializeJson(configuration, configFile); 
          if (error) {
            Serial.println("Impossible de lire le JSON");
            Serial.println(error.c_str());
          } else { 
            Serial.println("chargement config wifi");
            serializeJson(configuration, Serial);
            strcpy (user, configuration["user"]);
            strcpy (mdp, configuration["mdp"]);
            strcpy (domaine, configuration["domaine"]);
            strcpy (user_web,configuration["userweb"]);
            strcpy (mdp_web, configuration["mdpweb"]);            
          }
       }
       
    }
  configFile.close();
  //Création du point d'accès
  WiFiManagerParameter custom_text2("<p>Parametres d'acces au thermostat</p>");
  WiFiManagerParameter userweb("Utilisateur_web", "Utilisateur", user_web, 12);
  WiFiManagerParameter mdpweb("Motdepasse_web", "Mot de passe", mdp_web, 13);
  WiFiManagerParameter custom_text("<p>Parametres de mise a jour du dynhost</p>");
  WiFiManagerParameter user_dyndns("Utilisateur", "Utilisateur", user, 40);
  WiFiManagerParameter mdp_dyndns("Motdepasse", "Mot de passe", mdp, 13);
  WiFiManagerParameter domaine_dyndns("Nomdedomaine", "domaine", domaine, 40);
  WiFiManagerParameter url_dyndns("URLdyndns", "URL dyndns" , url, 50);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_text2);
  wifiManager.addParameter(&userweb);
  wifiManager.addParameter(&mdpweb);
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&user_dyndns);
  wifiManager.addParameter(&mdp_dyndns);
  wifiManager.addParameter(&domaine_dyndns);
  wifiManager.addParameter(&url_dyndns);
  wifiManager.autoConnect("Thermostat_connecte");
  Serial.println("Coonnection au WIFI avec succès");
  strcpy(user, user_dyndns.getValue());
  strcpy(mdp, mdp_dyndns.getValue());
  strcpy(domaine, domaine_dyndns.getValue());
  strcpy(url, url_dyndns.getValue());
  strcpy(user_web, userweb.getValue());
  strcpy(mdp_web, mdpweb.getValue());
  //domaine=(String)domainetemp;
  URL_MAJ = "http://" + (String)user + ":" + (String)mdp + "@" + (String)url + (String)domaine ;
  if (shouldSaveConfig) {
    Serial.println("saving config");
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    } else {
      configuration["user"] = String(user);
      configuration["mdp"]= String(mdp);
      configuration["domaine"]=String(domaine);
      configuration["url"]=String(url);
      configuration["userweb"]=String(user_web);
      configuration["mdpweb"]=String(mdp_web);
      serializeJson(configuration,configFile);
      serializeJson(configuration,Serial);
    }
    configFile.close();
  }
  //gateway = WiFi.gatewayIP();
  Serial.print("Ip local : ");
  //Serial.println(IpAdress2String (WiFi.localIP()));
  Serial.println(WiFi.localIP().toString());
  Serial.print("Gateway : ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Signal Strength : ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.println("SSID : " + WiFi.SSID());

  }
  //Initialisation de la récupération du temps
  Serial.println("récupération du temps");
  timeClient.begin();
  timeClient.update();
  DateMesure = timeClient.getEpochTime();
  PrevDateMesure = DateMesure;

  //Activation des capteurs
  Serial.println("Paramétrage du capteur");
  sensors.begin(); 
  sensors.getAddress(sensorDeviceAddress, 0); //Demande l'adresse du capteur à l'index 0 du bus
  sensors.setResolution(sensorDeviceAddress, 11); //Résolutions possibles: 9,10,11,12
  sensors.requestTemperatures(); //Demande la température aux capteurs
  temp = floattoint (sensors.getTempCByIndex(0));
  
  Wire.begin();
  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  Serial.println( "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix));
  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
 };
  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  if(!iaqSensor.run()){
    checkIaqSensorStatus();
  }
   // serveur FTP
  ftpSrv.begin(user_web, mdp_web); 
  
   //Paramétrage du serveur web
  Serial.println("Démarrage du serveur web");
  server.onNotFound([]() {                              // If the client requests any URI
          Serial.println(F("On not found"));
        if (!handleFileRead(server.uri())){                  // send it if it exists
            server.send(404, "text/plain", "Not Found");; // otherwise, respond with a 404 (Not Found) error
        }
      });
  
  /*server.serveStatic("/", SPIFFS, "/index.html");*/
  server.serveStatic("/css", SPIFFS, "/css");
  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/img", SPIFFS, "/img");
  server.serveStatic("/api/historymeteoheure.json", SPIFFS, "/historymeteoheure.json");
  server.serveStatic("/api/historymeteoquotidien.json", SPIFFS, "/historymeteoquotidien.json");
  server.serveStatic("/api/historymeteohebdo.json", SPIFFS, "/historymeteohebdo.json");
  server.serveStatic("/api/historytotal.json", SPIFFS, "/historytotal.json");
  server.serveStatic("/api/statut.json", SPIFFS, "/statut.json");
  server.on("/api/mesure.json", sendMesures);
  server.on("/api/histo", sendMesuresHisto);
  server.on("/api/update", majstatut);
  server.begin();

  couleur_statut=&couleur_fonction[0];
  displayColor(&couleur_statut[0], luminosite);


}

// the loop function runs over and over again forever
void loop() {
  server.handleClient();
  ftpSrv.handleFTP(); 


  //Gestion du boutton Reset
  bouttonReset ();

  //controle du wifi
  if (millis()-MillisTestWifi>=DelaisTestWifi){
    updatestatut ();
    Serial.print("test de la connexion wifi : ");
    if (!pinger.Ping( WiFi.gatewayIP())){
      Serial.println("wifi KO");
      couleur_statut=&couleur_error[0];
      displayColor(&couleur_statut[0], luminosite);
      WiFi.reconnect();
      StatutWifi = false;
    } else if (StatutWifi = false) {
      Serial.println("Wifi de nouveau OK");
      StatutWifi = true;
      if (Mf==true){ //marche forcée 
        couleur_statut=&couleur_forcee[0];
      }
      else if ( S ==true){ // en marche sans forçage
        couleur_statut=&couleur_haut[0];
      } else { // éteint
        couleur_statut=&couleur_fonction[0];
      }
      displayColor(&couleur_statut[0], luminosite);
    }else{
      Serial.println("Wifi OK");
      dyndns ();
    }
    MillisTestWifi = millis();
  }    

  //mesure
  if (millis()-MillisMesure > DelaisMesure){
    MillisMesure = millis();
    displayColor(&couleur_mesure[0], luminosite);  // turn the LED on (HIGH is the voltage level)
    timeClient.update();
    //delay(100);

    sensors.requestTemperatures(); //Demande la température aux capteurs
    temp = floattoint (sensors.getTempCByIndex(0));
    DateMesure = timeClient.getEpochTime();
    
    
    if (temp != -127 && DateMesure > PrevDateMesure){
      PrevDateMesure = DateMesure;
      ctrlchauff(); // controle d'enclenchement chauffage
      tab_mesure[nbenr]=temp;
      nbenr++;
      if (MillisMesure - MillisEnr > DelaisEnr ){
        MillisEnr = MillisMesure;
        if (iaqSensor.run()) { // If new data is available
          IAQ += floattoint(iaqSensor.iaq);
          pression += floattoint(iaqSensor.pressure/100);
          humidite += floattoint(iaqSensor.humidity);
          nbenrmeteo++;
          Meteoheure.Update(DateMesure,iaqSensor.pressure/100,iaqSensor.humidity,iaqSensor.iaq);
        } else {
          checkIaqSensorStatus();
        }
        int mesure_total = 0 ;  // calcul de la moyenne des précédentes mesures
        for (int i = 1 ; i<nbenr-1 ; i++){
           mesure_total +=  tab_mesure[i];
        }
        temp = roundf(mesure_total/(nbenr-2));
        insmesure();
        nbenr = 0;
      }
      if (MillisMesure - MillisEnrQuo > DelaisEnrQuo ){
        MillisEnrQuo = MillisMesure;
        Meteoquotidien.Update(DateMesure,round(inttofloat(pression)/nbenrmeteo*100)/100,round(inttofloat(humidite)/nbenrmeteo*100)/100,round(inttofloat(IAQ)/nbenrmeteo*100)/100);
        IAQhebdo += IAQ;
        pressionhebdo += pression;
        humiditehebdo += humidite;
        nbenrmeteohebdo++;
        nbenrmeteo=0;
        pression = 0;
        humidite=0;
        IAQ=0;
      }
      if (MillisMesure - MillisEnrHebdo > DelaisEnrHebdo ){
        MillisEnrHebdo = MillisMesure;
        Meteohebdo.Update(DateMesure,round(inttofloat(pressionhebdo)/nbenrmeteohebdo*100)/100,round(inttofloat(humiditehebdo)/nbenrmeteohebdo*100)/100,round(inttofloat(IAQhebdo)/nbenrmeteo*100)/100);
        nbenrmeteohebdo=0;
        pressionhebdo = 0;
        humiditehebdo=0;
        IAQhebdo=0;
      }
    }
    displayColor(&couleur_statut[0], luminosite);    // turn the LED off by making the voltage LOW
  }
  delay(100);                       // wait for a 0,1 second
}


void displayColor(byte *couleur,byte lum) {
  if (lum>100){lum=100;}
//Serial.println("Rauge : " + (String)(couleur[0]*lum/100));
  analogWrite(Rouge, couleur[0]*lum/100);
  //digitalWrite(Rouge,HIGH);
  analogWrite(Vert, couleur[1]*lum/100);
  analogWrite(Bleu, couleur[2]*lum/100);
}



void bouttonReset (){
  unsigned long millisCheck = millis();
  if (ResetBoutton.pressed()||ResetBoutton.pressedlong()) {
    RESET = false;
  } else if (PlusBoutton.pressed()){
    Tc = Tc +5;
    Serial.println("Nouvelle température de consigne " + (String)floattoint(Tc));
    updatestatut ();
  } else if (MoinsBoutton.pressed()){
    Tc = Tc -5;
    Serial.println("Nouvelle température de consigne " + (String)floattoint(Tc));
    updatestatut ();
  } else {
    RESET = true;
  }

  if( StateBoutton == 0 && RESET == false ) { 
    millisStart=millisCheck;
    StateBoutton = 1;
    Serial.println("début appui Reset");
    displayColor(&couleur_error[0], 0);
    }
  if( StateBoutton == 1 && RESET == true ) { 
    StateBoutton = 0;
    displayColor(&couleur_statut[0], luminosite);
    Serial.println("Reset annulé");
    }
  if(StateBoutton == 1 && RESET == false && millisCheck-millisStart <appuilong-2000){ // affichage progressif de la LED
        displayColor(&couleur_error[0], 200*(millisCheck-millisStart)/(appuilong-2000));
        //Serial.println((String)(255*(millisCheck-millisStart)/(appuilong-2000)));
  }
  if(StateBoutton == 1 && RESET == false && millisCheck-millisStart >= appuilong-2000 && millisCheck-millisStart < appuilong){ //Led clignote avant validation du reset
          displayColor(&couleur_error[0], 100);   // turn the LED on (HIGH is the voltage level)
          delay(100);                       // wait for a 0,1 second
          displayColor(&COLOR_BLACK[0], 100);    // turn the LED off by making the voltage LOW
          delay(100);
  }
  if(StateBoutton == 1 && RESET == false && millisCheck-millisStart >=appuilong){ 
      StateBoutton=0;
      Serial.println("Appui de :"+(String)(millisCheck-millisStart)+"ms : Erase settings and restart ...");
      SPIFFS.remove("/config.json");
      wifiManager.resetSettings();  
      ESP.restart();  
  }
}

float inttofloat ( int value) {
  return (float)value/10;
}
 int floattoint (float value) {
  return(int)roundf(value*10);
}

/*String IpAdress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}
*/
void ordonnerTableau(long tableau[], long tailleTableau){
  long i,t,k=0;
  for(t = 1; t < tailleTableau; t++){
    for(i=0; i < tailleTableau - 1; i++){
      if(tableau[i] > tableau[i+1]){
        k= tableau[i] - tableau[i+1];
        tableau[i] -= k;
        tableau[i+1] += k;
      }
    }
  }
}

// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      Serial.println("BSEC error code : " + String(iaqSensor.status));
      couleur_statut=&couleur_error[0];
    } else {
      Serial.println( "BSEC warning code : " + String(iaqSensor.status));
    }
  } else {
    couleur_statut=&couleur_fonction[0];
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      Serial.println("BME680 error code : " + String(iaqSensor.bme680Status));
      couleur_statut=&couleur_error[0];
    } else {
      Serial.println( "BME680 warning code : " + String(iaqSensor.bme680Status));
    }
  }else {
    couleur_statut=&couleur_fonction[0];
  }
}

void dyndns (){
          
  WiFiClient client;
  HTTPClient http;
  IPAddress ippub; 

  if (http.begin(client, "http://ifconfig.co/ip")) {  // récupération de l'ip public
      int httpCode = http.GET();
      if (httpCode > 0) { // httpCode will be negative on error
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString(); // récupération du résultat
          payload.remove(payload.length()-1); // traitement du résultat
          Serial.println("Ip actuelle : " + payload);
        //  Serial.println("Domaine : " + domaine);
          WiFi.hostByName(domaine, ippub); // résolution de nom DNS
          Serial.println("IP dyndns : " + ippub.toString() );
          if ( payload.equals(ippub.toString())) { // comparaison de l'ip du DNS et IP public
            Serial.println("Enregistrement Dyndns à jour");
          } else {
            Serial.println("MAJ enregistrement Dyndns");
            payload = URL_MAJ + "&myip=" + payload ;
            //Serial.println(payload);
            http.end();
            http.begin(payload);
       //     http.addHeader("User-Agent", "curl/7.26.0"); 
            httpCode = http.GET();
            if (httpCode > 0) {
              Serial.println("hhtpCode : " + (String)httpCode);
              if (httpCode == HTTP_CODE_OK) {
                payload = http.getString();
                Serial.println("request OK : " + payload);
              } 
            } else {
              Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }    
          }
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
}
