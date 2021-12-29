#ifndef BOUTTON_H
#define BOUTTON_H
#include <Arduino.h> // indispensable ici, le type boolean n'est pas implanté
class Boutton{
private:
  int _adcValue;
  boolean _buttonPrevious;
  int _buttonHysteresis;
  int _bounceDelay; 
public:
  Boutton(int,int);
  boolean pressed();
  boolean pressedlong();
  ~Boutton();  
};
#endif
