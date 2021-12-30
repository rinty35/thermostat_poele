# Thermostat DIY Poêle à pellets
![Photos du thermostat](https://github.com/rinty35/thermostat_poele/blob/master/Photos/Vue_ensemble.jpg)
## Le concept
Il s’agit d’un thermostat connecté pour tout chauffage nécessitant un contact sec pour forcer le déclanchement. Son usage premier était donc à destination d’un poêle à pellets Il pourrait servir à déclencher toutes sortes de chauffage. A cette fonction principale a été ajoutée une fonctionnalité de station météo connectée.
## Le matériel nécessaire
#### Electronique module principal
-	Alimentation a découpage HILink / fusible / varistance
-	Circuit Relais 5v / transistor / résistance / diode de roue libre
-	Puce principale ESP8266
-	Capteur météo BME680
-	Diode RVB / bouton poussoir
#### Electronique module de controle
-	Capteur de température DS18B20
-	Boutons poussoir de réglage de température / marche forcée
-	Affichage LCD
## Librairies utilisées
-	Onwire
-	DallaTemperature
-	ESP8266Webserver
-	WifiManager
-	ArduinoJson
-	ESP8266FtpServer
## Modélisation 3D
Conception sous Freecad
## Fonctionnement
1.	Sur le boitier principal : appui long pour réinitialiser le boitier (clignotement de la LED du boitier en rouge)
2.	A la première connexion : portail de connexion wifi et paramétrage accès web / client OVH Dyndns
3.	Au second démarrage affichage de l’IP sur le réseau local lors du démarrage (LED couleur blanche)
4.	Passage de la LED en couleur verte une fois le démarrage finalisée / bleu lors d’une mesure / rouge en cas de disfonctionnement / violet en cas déclanchement marche forcée / jaune en cas de chauffage sur consigne thermostat. 

![Photos panneau de controle](https://github.com/rinty35/thermostat_poele/blob/master/Photos/panneau_détail.jpg)

Le panneau de contrôle affiche l’heure, la température de consigne, la température actuelle, l’hygrométrie, la pression atmosphérique. A cela s’ajoute dans la partie supérieure droite une étoile quand le chauffage est allumé, le message « sécu » quand le démarrage a eu lieu dans les 15minutes pour éviter des cycles de démarrage/extinction trop rapprochés du poêle, le message « Mf » pour marche forcée

Les boutons de droites servent à régler la consigne celui de gauche a allumer le poêle même si la consigne ne le nécessite pas (marche forcée)

## Portail WEB
Le portail web offre les mêmes fonctionnalités que le panneau de contrôle en ajoutant des options de réglage et la consultation d’historiques.

![Photos page accueil]( https://github.com/rinty35/thermostat_poele/blob/master/Screenshot/accueil.jpg)

L’accueil affiche si le poêle chauffe (flamme rouge) ou non, la température courante, la possibilité d’activer la marche forcée ainsi que le réglage de la température de consigne. Un historique de la température est disponible avec en rouge les périodes de chauffage

![Photos réglages]( https://github.com/rinty35/thermostat_poele/blob/master/Screenshot/config.jpg)

La page de réglage affiche si le mode protection (interdisant un allumage de moins de 15min) est activé et si oui pour combien de temps.

Elle permet le réglage des seuils de déclanchement par 0,5°C en dessous et au-dessus de la température de consigne (pour éviter des cycles d’allumage/extinction trop fréquent)
-	Allumage quand la T° est sous la consigne + seuil bas
-	Extinction quand la T° est au-dessus de la consigne + seuil haut

Elle affiche les informations concernant le wifi (IP, SSID du réseau, qualité du signal)

![Photos meteo]( https://github.com/rinty35/thermostat_poele/blob/master/Screenshot/m%C3%A9teo1.jpg)

L’onglet météo affiche les informations actuelles et historiques sur la température, hygrométrie, la pression atmosphérique et la qualité de l’air. L’historique est lissé pour gagner en volumétrie de stockage sur la puce et surtout en temps de traitement pour l’affichage.

