#ifndef METEO_H
#define METEO_H
#include <Arduino.h> // indispensable ici, le type boolean n'est pas implanté
#include <ArduinoJson.h>
class Meteo{
private:
  short int  _SizeHistMeteo;
  //DynamicJsonDocument _Tab_Meteo;
  byte _nbenrmeteo;
  String _nomfichier; 

public:
  Meteo(int, String);
  void Update(unsigned long,float,float,float);
 // void Load();
  ~Meteo();  
};
#endif
