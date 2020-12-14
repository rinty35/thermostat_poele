#include "meteo.h"

Meteo::Meteo(int nbenr, String nomfichier) /* : _Tab_Meteo(JSON_ARRAY_SIZE(nbenr) + nbenr*JSON_OBJECT_SIZE(4) + 20)*/{
  _SizeHistMeteo= nbenr; 
//  _nbenrmeteo = 0;
  _nomfichier = nomfichier;
}

void Meteo::Update(unsigned long datedemesure, float pressionmesure,float txHumidite, float qualiteAir){
 int i ;
 DynamicJsonDocument _Tab_Meteo (JSON_ARRAY_SIZE(_SizeHistMeteo) + _SizeHistMeteo*JSON_OBJECT_SIZE(4) + 20);
 File file = SPIFFS.open(_nomfichier, "r");
  if (!file){
    Serial.println("Aucun historique existe - " + _nomfichier);
  } else {
    if ( file.size() == 0 ) {
      Serial.println("Fichier historique vide !");
    } else {
      DeserializationError error = deserializeJson(_Tab_Meteo, file);
      if (error) {
        Serial.println("Impossible de lire le JSON");
        Serial.println(error.c_str());
      } else {
  //      Serial.println("Historique charge " + _nomfichier);
       // serializeJson(_Tab_Meteo, Serial);
        Serial.println("date : " + (String)datedemesure + " Pression : " + (String)pressionmesure + " Tx humidite : " + (String)txHumidite + " IAQ : " + (String)qualiteAir);
        /*if (_Tab_Meteo.size()== _SizeHistMeteo){
          _nbenrmeteo = 0;
        }else{
          _nbenrmeteo = _Tab_Meteo.size();
        } */   
      }
    }
    
  }
 file.close();
 if (_Tab_Meteo.size()>= _SizeHistMeteo){
   //Serial.println("Suppression de la première valeur");
   for (i =1 ; i< _SizeHistMeteo ; i++){
     _Tab_Meteo[i-1]["d"] = _Tab_Meteo[i]["d"];
     _Tab_Meteo[i-1]["p"] = _Tab_Meteo[i]["p"];
     _Tab_Meteo[i-1]["h"] = _Tab_Meteo[i]["h"];
     _Tab_Meteo[i-1]["iaq"] = _Tab_Meteo[i]["iaq"];
   }
   //Serial.println("stockage en position : " + (String)(_SizeHistMeteo-1));
   _Tab_Meteo[(_SizeHistMeteo-1)]["d"] = datedemesure;
   _Tab_Meteo[(_SizeHistMeteo-1)]["p"] = pressionmesure;
   _Tab_Meteo[(_SizeHistMeteo-1)]["h"] = txHumidite;
   _Tab_Meteo[(_SizeHistMeteo-1)]["iaq"] = qualiteAir;
 } else {
   //Serial.println("stockage en position : " + (String)_nbenrmeteo);
   i = _Tab_Meteo.size();
   _Tab_Meteo[i]["d"] = datedemesure;
   _Tab_Meteo[i]["p"] = pressionmesure;
   _Tab_Meteo[i]["h"] = txHumidite;
   _Tab_Meteo[i]["iaq"] = qualiteAir;
 }
// Serial.println("Enregistrement : " + (String)(nbenrmeteo+1) + " Timestamp : " + (String)datedemesure + " pression : " + (String)pressionmesure);
/* _nbenrmeteo++;
 if ( _nbenrmeteo > (_SizeHistMeteo -1) ) {
   Serial.println("maximum historique atteint");
   _nbenrmeteo = 0;
 }*/
   // serializeJson(_Tab_Meteo,Serial);
 file  = SPIFFS.open(_nomfichier, "w");
 Serial.println("Sauvegarde des données " + _nomfichier);
// serializeJson(_Tab_Meteo, Serial);
 serializeJson(_Tab_Meteo, file);
 file.close();

}

Meteo::~Meteo(){
}
