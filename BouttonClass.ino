#include "boutton.h"

Boutton::Boutton(int adcValue, int bounceDelay){
  _adcValue=adcValue; // valeur issue de la conv. analogique-numérique
  _bounceDelay=bounceDelay; // pause anti-rebonds
  _buttonPrevious=false;
  _buttonHysteresis=10;  // fourchette de détection: adcValue + ou - hysteresis
}

boolean Boutton::pressed(){ // retourne true si basculement détecté
  int buttonValue = analogRead(A0);   // lecture de l'entrée analogique A0
  boolean buttonJustPressed = 
    ( buttonValue >= _adcValue - _buttonHysteresis )
    && ( buttonValue <= _adcValue + _buttonHysteresis );  
  boolean toggle = buttonJustPressed && !_buttonPrevious; // bascule si bouton pressé ET état précédent NON pressé  
  _buttonPrevious = buttonJustPressed;  // mémorise l'état du bouton
  if (toggle){
   delay(_bounceDelay); // pause anti-rebonds
   buttonValue = analogRead(A0);   // nouvelle lecture de l'entrée analogique A0
   buttonJustPressed = 
    ( buttonValue >= _adcValue - _buttonHysteresis )
    && ( buttonValue <= _adcValue + _buttonHysteresis );
   if (buttonJustPressed){
     return (true);
   }
  }
  return (false);   
}

boolean Boutton::pressedlong(){ // retourne true si basculement détecté
  int buttonValue = analogRead(A0);   // lecture de l'entrée analogique A0
  boolean buttonJustPressed = 
    ( buttonValue >= _adcValue - _buttonHysteresis )
    && ( buttonValue <= _adcValue + _buttonHysteresis );  
  boolean toggle = buttonJustPressed && _buttonPrevious; // bascule si bouton pressé ET état précédent OUI pressé  
  _buttonPrevious = buttonJustPressed;  // mémorise l'état du bouton
  if (toggle){
   delay(_bounceDelay); // pause anti-rebonds
   buttonValue = analogRead(A0);   // nouvelle lecture de l'entrée analogique A0
   buttonJustPressed = 
    ( buttonValue >= _adcValue - _buttonHysteresis )
    && ( buttonValue <= _adcValue + _buttonHysteresis );
   if (buttonJustPressed){
     return (true);
   }
  }
  return (false);   
}

Boutton::~Boutton(){
}
