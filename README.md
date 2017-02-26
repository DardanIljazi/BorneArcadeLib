
Introduction
===================

La librairie Arcade Lib a été créée lors d'un projet moyen du Module Complémentaire Technique du Centre Professionnel du Nord Vaudois en février 2017.

Le but était de faire une malette d'arcade avec des boutons et un joystick qui pouvait, par la suite, communiquer et intéragir avec d'autres composants externes.


![Malette Arcade](http://vpictu.re/uploads/1e9db51594bc0e0dd7dd548eb940d2d16f784438.png)



----------


Fonctionnement général
----------------------

La librairie intègre et fonctionne avec le protocole SPI, tant en mode Master qu'en mode Slave.

Pour cela, un Arduino nano est utilisé en mode Slave à l'intérieur de la Malette et répond aux demandes du Master (celui-ci étant le composant externe).

Le fonctionnement général de la librairie est le suivant:

 1. Master --(DEMANDE)-->SLAVE
 2. Master --(ENVOIE SANS DONNÉES)--> SLAVE
     Master<--(REPONSE A LA DEMANDE)-- SLAVE

> **Note:**
> A la première demande du Master, le Slave ne répond rien
> Au second envoi, le Master envoie 8 bits "vides" valant tous 0. Le Slave répond en même temps à la demande faite précédemment


Parmi les demandes que le Master peut faire au Slave on compte:

 1. L'état actuel des boutons (tel ou tel bouton est-il appuyé ?)
 2. L'état actuel du joystick (est-ce que le joystick est contre le haut, gauche bas droite ? )
 3. L'activation du mode interrupt
 3. *Un test de communication (utilisé seulement à l'initialisation de la librairie pour savoir si le Master et le Slave communiquent bien*

Fonctionnement et fonctions
---------------------------------
###SPI
Il faut savoir que le protocole SPI communique en 8 bits grâce à l'aide de 4 pins/fils:

<img src="http://vpictu.re/uploads/9e248af30b852e2e18794151b251b848152d6633.png" alt="SPI communication" style="text-align: center; margin-left: 150px;" />

A chaque état HAUT (état 1) du clock (SCK), les états du MOSI (Master-->Slave)/MISO (Master<--Slave) sont mis à 1 ou 0 selon ce que le Master ou le Slave envoient/répondent.
###Requêtes
Parmi les demandes que le Master peut faire au Slave on compte:


|Demande|Fonction utilisée pour cette demande|Bits envoyés depuis le Master au Slave pour la demande (MOSI)|Réponse du Slave (MISO)|
|:---|---|:---:|---|
|L'état des boutons|void getActualButtonStates()|0x01 (00000001)| Selon l'état des boutons|
|L'état du joystick|void getActualJoystickStates()|0x02 (00000010)|  Selon l'état du joystick|
|Test de la communication|bool communicationTest()|0x03 (00000011)|  0xAA (10101010)|
|Activation d'interrupt|void activateInterrupt(...)|0x04 (00000100)|  Si interrupt  activé: 0x01 (00000001), si interrupt désactivé 0x00 (00000000)|



> **Note:**
> Le test de communication (communicationTest()) est utilisé pour savoir si le slave arrive à répondre 0xAA (10101010) au Master. Si le Master ne reçoit pas cette valeur, cela veut dire qu'il y a un problème de communication entre le Slave et le Master.
> Concernant l'activation des Interrupts, dès que le Slave reçoit le byte 0x04 (00000100), il active les interrupts si celui-ci était désactivé ou le désactive si celui-ci était activé. Dans le premier cas, il répond au Master par 0x01 (00000001), dans le deuxième 0x00 (00000000)


####État des boutons retourné

L'état des boutons et du joystick retourné par le Slave est stocké dans des variables (8 bits, globales et volatiles) du Master: 
```C
volatile uint8_t buttonStatesMaster, joystickStatesMaster;
```
Ces variables sont utilisées dans les fonctions (la colonne Fonction) ci-dessous.
####Fonction pour accéder à l'état des boutons retourné sur le Master
Afin de savoir si tel ou tel bouton/joystick est appuyé, les fonctions suivantes sont disponibles:

|Fonction à appeler pour récupérer l'état|Fonction|N° sur l'image|
|-------------|-------------|-------------:|
|void getActualButtonStates()| bool isStartButtonPressed() | 1|
|void getActualButtonStates()| bool isStopButtonPressed()  | 2|
|void getActualButtonStates()| bool isButton1Pressed()     | 3|
|void getActualButtonStates()| bool isButton2Pressed()     | 4|
|void getActualButtonStates()| bool isButton3Pressed()     | 5|
|void getActualButtonStates()| bool isButton4Pressed()     | 6|
|void getActualButtonStates()| bool isButton5Pressed()     | 7|
|void getActualButtonStates()| bool isButton6Pressed()     | 8|
|void getActualJoystickStates()| bool isJoystickTop()      | 9|
|void getActualJoystickStates()| bool isJoystickLeft()     |10|
|void getActualJoystickStates()| bool isJoystickBottom()   |11|
|void getActualJoystickStates()| bool isJoystickRight()    |12|



<img src="http://vpictu.re/uploads/83b48fbb17ab8f42b69d0fbca193625f3acab6ae.png" alt="SPI Arcade Lib" />

> **Note:**
> Si le joystick est exactement mis en diagonale en haut à droite (entre 9 et 12), les fonctions 
> ```bool isJoystickTop(), bool isJoystickRight()``` renvoient toutes les deux **true**. 
> Si c'est exactement mis en diagonale en bas à gauche (entre 10 et 11), les fonctions
>  ```bool isJoystickLeft(), bool isJoystickBottom()``` renvoient toutes les deux **true**.
> Ainsi de suite..
> 
> Il faut savoir qu'il est difficile, avec ce joystick, d'être parfaitement en diagonale. En effet, le Joystick est mécanique (avec contacts) et non analogique ou numérique. Il y a 4 pins sorties (HAUT, GAUCHE, BAS, DROITE). Si la diagonale est atteinte, deux de ces pins sont en état HAUT.
> Étant donné cette spécificité, aucune fonction gérant l'état "diagonale" n'a été créée dans la librairie. Il est cependant possible de regarder si 2 états renvoient vrai pour savoir si le joystick est en diagonale.

#####Exemple de code (Code 1)
```C
// MASTER exemple
// Inclure la librairie
#include "arcadeLib.h"

void setup (void)
{
  if(initArcadeLib(MASTER_MODE)){
    if(communicationTest()){
      // Communication Ok
    }else{
     // Impossible d'initialiser la communication (erreur)
    }
  }else{
    // Impossible d'initialiser la librairie
  }

}

void loop (void)
{
  // Demande l'état actuel des boutons et du joystick au Slave
  getActualButtonStates();
  getActualJoystickStates();
  
  // Si le bouton Start est appuyé
  if(isStartButtonPressed()){
     // On initialise le jeu..
  }
  
  // Si le bouton Stop est appuyé
  if(isStopButtonPressed()){
     // On stop le jeu..
  }
  
  // Si le bouton 1 est appuyé
  if(isButton1Pressed()){
    // On fait sauter Mario..
  } 
  
  // Si le joystick est contre le haut
  if(isJoystickTop()){
    // ...
  } 
  
  // Si le joystick est en diagonale en haut à droite 
  if(isJoystickTop() && isJoystickRight()){
    // ...
  } 
  
}

```

####État des boutons par appel de fonction
Il est possible d'appeler directement une fonction interne au Master lorsque l'un des boutons/joystick est appuyé.
Cela évite dans ce cas de devoir passer par une condition dans le loop.


La fonction
```C
void attachActionToFunc(Button_Joystck button_joystick, void(*f)());
```
Le premier paramètre est un enum pouvant prendre ces valeurs:
|Valeur ENUM|N° sur l'image plus haut|
|-------------|:-------------:|
|START_BUTTON|1|
|STOP_BUTTON|2|
|BUTTON_1|3|
|BUTTON_2|4|
|BUTTON_3|5|
|BUTTON_4|6|
|BUTTON_5|7|
|BUTTON_6|8|
|JOYSTCK_TOP|9|
|JOYSTCK_LEFT|10|
|JOYSTCK_BOTTOM|11|
|JOYSTCK_RIGHT|12|

Le second paramètre est le nom de votre fonction.

#####Exemple de code (Code 2)
```C
// MASTER exemple
// Inclure la librairie
#include "arcadeLib.h"

void setup (void)
{
  if(initArcadeLib(MASTER_MODE)){
    if(communicationTest()){
      // Communication Ok
      
      /**On définit les actions à rattacher aux fonctions**/
      // Si le bouton Start est appuyé, la fonction initGame est appelée
      attachActionToFunc(START_BUTTON, initGame);
      // Si le bouton Stop est appuyé, la fonction stopGame est appelée
      attachActionToFunc(STOP_BUTTON,  stopGame);
      // Si le bouton 1 est appuyé, la fonction sauter est appelée
      attachActionToFunc(BUTTON_1,     sauter);
    }else{
     // Impossible d'initialiser la communication (erreur)
    }
  }else{
    // Impossible d'initialiser la librairie
  }

}

void loop (void)
{
  // Demande l'état actuel des boutons et du joystick au Slave
  getActualButtonStates();
  getActualJoystickStates();
}

void initGame(){
  // On initialise le jeu..
}

void stopGame(){
 // On stop le jeu..
}

void sauter(){
 // On fait sauter Mario..
}
```

> **Attention:**
> A première vue,  dans le code ci-dessus (code 2) on pourrait croire que initGame(), stopGame() ou sauter() sont appelées qu'une seule fois (lorsque le bouton Start, Stop ou 1 sont appyués) car ils ne sont pas dans le loop.
> **Ceci est faux**: A vraie dire, à l'intérieur de la fonction getActualButtonStates() et getActualJoystickStates() se trouve les conditions qui renvoient à ces fonctions (initGame(), stopGame(), sauter()) et vu qu'elles sont dans le loop, les conditions sont tout le temps testées.

###Différence entre le code 1 et 2 ?
Quelle est la différence entre le code 1 et 2 ci-dessus et qu'apporte l'un par rapport à l'autre ?
A vraie dire, aucune. Cela est plutôt lié à un aspect esthétique/propreté du code plus qu'autre chose. 
Dans l'un, les conditions sont directement dans le loop, alors que dans l'autre les conditions se trouvent "cachées" dans la librairie (dans l'appel des fonctions getActualButtonStates() et getActualJoystickStates() plus précisément).



Mais le résultat est le même.
D'ailleurs, le résultat ci-dessous est aussi le même que les deux codes précédents.

On aurait très bien pu faire ceci :
#####Exemple de code (Code 3)
```C
// MASTER exemple
// Inclure la librairie
#include "arcadeLib.h"

void setup (void)
{
  if(initArcadeLib(MASTER_MODE)){
    if(communicationTest()){
      // Communication Ok
    }else{
     // Impossible d'initialiser la communication (erreur)
    }
  }else{
    // Impossible d'initialiser la librairie
  }

}

void loop (void)
{
  // Demande l'état actuel des boutons et du joystick au Slave
  getActualButtonStates();
  getActualJoystickStates();
  
  initGame();
  stopGame();
  sauter();
}

void initGame(){
  // On regarde si le bouton Start a été appuyé
  if(isStartButtonPressed()){
    // On initialise le jeu..
  }
}

void stopGame(){
  // On regarde si le bouton Stop a été appuyé
  if(isStopButtonPressed()){
    // On stop le jeu..
  }
}

void sauter(){
  if(isButton1Pressed()){
    // On fait sauter Mario..
  } 
}
```

##Loop() et appui sur boutons

Dans les codes précédents, les fonctions getActualButtonStates() et getActualJoystickStates() sont continuellement appelées dans le loop.

Cela veut dire que le Slave est tout le temps solicité en retournant l'état des boutons actuels comme dans le code suivant:
```C
void loop (void)
{
  // Demande l'état actuel des boutons et du joystick au Slave
  getActualButtonStates();
  getActualJoystickStates();

  // Si le bouton Start est appuyé
  if(isStartButtonPressed()){
     // On initialise le jeu..
  }
}
```
###Ce que cela implique
Cela fera certainement entrer le code plusieurs fois dans la condition isStartButtonPressed() car ce dernier renvoi vrai tant que le bouton n'est pas lâché, et même s'il est appuyé et lâché très vite,  il y a un "risque" que la condition isStartButtonPressed() soit entrée plusieurs fois dans la condition car le loop est très rapide et aura été recommencé plusieurs fois.

Pour certains cas, cela ne pose pas de problèmes car cela est voulu, mais il peut arriver qu'on veuille, par exemple, initialiser un jeu une seule fois (au début du jeu et ne plus s'en occuper). Dans ce cas-là, il faudrait rajouter une variable de condition qui devient vrai dès qu'on entre dans la condition pour ne plus y rentrer de nouveau:
#####Exemple de code (Code 4)
```C
bool isEnteredIntoCondition = false;

void loop (void)
{
  // Demande l'état actuel des boutons et du joystick au Slave
  getActualButtonStates();
  getActualJoystickStates();

  // Si le bouton Start est appuyé et qu'on est pas encore entré dans la condition
  if(isStartButtonPressed() && isEnteredIntoCondition == false){
     isenteredIntoCondition = true;
     // On initialise le jeu..
  }
}
```





##Autre solution, les interrupts

###Qu'est-ce qu'un interrupt ?
Les interrupts, comme son nom l’indique, consiste à interrompre momentanément le programme que l’Arduino exécute pour qu’il effectue un autre travail. Quand cet autre travail est terminé, l’Arduino retourne à l’exécution du programme et reprend à l’endroit exact où il l’avait laissé.

###Les interrupts dans la librairie
Les interrupts peuvent être activés dans la librairie en utilisant la fonction:

```C
void activateInterrupt(Interrupt interrupt, InterruptMode interrupt_mode);
```

####Premier argument interrupt
Le premier argument peut contenir les valeures suivantes:

|Valeur ENUM|Explication|
|-------------|-------------|
|EXTERNAL_PIN_INTERRUPT|L'interrupt est activé dès qu'un bouton sur le Slave change d'état (appuie ou relâchement du bouton/joystick)|
|TIMER_INTERRUPT|Le Master demande l'état des boutons/joystick au Slave toutes les 100ms|


> **Petite explication:**
> L'arduino Master et l'Arduino Slave sont reliés par les pins 13 (SCK), 12 (MISO), 11 (MOSI) et le pin SS (10 de base mais peut être changé sur le Master) et le pin 2 (le pin d'interrupt).
> 
> Dans le premier cas, quand EXTERNAL_PIN_INTERRUPT est activé, le Slave change d'état sur le pin 2 (HAUT-BAS, BAS-HAUT) dès qu'un changement d'état des boutons ou du joystick apparaît (par exemple quand un bouton est appuyé ou relâché). Le Master s'aperçoit de ce changement d'état sur le pin 2, arrête ce qu'il était en train de faire et demande l'état des boutons et du joystick au Slave.
> 
> Dans le deuxième cas, si TIMER_INTERRUPT est utilisé, le Master arrête ce qu'il fait toutes les 100 ms et demande l'état des boutons au Slave


####Deuxième argument interrupt_mode
Le deuxième argument peut contenir les valeures suivantes:

|Valeur ENUM|Explication|
|-------------|-------------|
|NORMAL_INTERRUPT_MODE|Ne fait rien de spécial (est activé de base)|
|REMAIN_HIGH_UNTIL_NEXTCALL|Garde l'état HAUT (appuyé) des boutons/joystick (même après que ceux-ci soient relâchés) jusqu'à l'appel de la fonction récupérant l'état du bouton en question|


> **Petite explication:**
> L'argument NORMAL_INTERRUPT_MODE ne fait rien de spécial. Celui-ci est activé de base
> 
> Concernant l'argument REMAIN_HIGH_UNTIL_NEXTCALL **celui-ci peut être très intéressant**.
> En effet, quand on appuie sur un bouton, le Master envoie une demande pour avoir l'état des boutons et le Slave lui répond. Mais cela arrive aussi quand le bouton est relâché. C'est-à-dire que si on appuie un bouton et qu'on le relâche très vite et que les fonctions isStartButtonPressed(), isStopButtonPressed() sont appelées plus tard, l'état des boutons apparaîtra comme non appuyé. Imaginons le code suivant:


#####Exemple de code
```C
// MASTER exemple
#include "arcadeLib.h"

void setup (void)
{
  if(initArcadeLib(MASTER_MODE)){
    if(communicationTest()){
      // Communication Ok
      // Active l'interrupt
      activateInterrupt(EXTERNAL_PIN_INTERRUPT);
    }else{
     // Impossible d'initialiser la communication (erreur)
    }
  }else{
    // Impossible d'initialiser la librairie
  }
}

void loop (void)
{
  if(isStartButtonPressed()){
     // On initialise le jeu..
  }
  
  // LONG CALCUL QUI PREND 10 SECONDES A SE FINIR ICI..
  
}
```

Le code ci-dessus active l'interrupt dans le setup et regarde dans le loop si le bouton Start est appuyé et ensuite (pour l'exemple) fait un long calcul qui dure 10 secondes.

Si le logiciel se trouve à l'étape  if(isStartButtonpressed()) et que le bouton est appuyé à ce moment-là, la condition renverra vraie. (voir image ci-dessous):

![Malette Arcade](http://vpictu.re/uploads/6a1efd0f879d725e6208fd4bf8fb0128b8c61eca.png)

Mais imaginons maintenant si le bouton est appuyé et relâché alors que le programme se trouve à l'étape du long calcul qui prend 10 secondes. Quand le loop reviendra sur la condition isStartButtonPressed(), la condition ne sera jamais exécutée car renverra faux (l'état des boutons à ce moment-là sera à l'état bas) comme le montre l'image ci-dessous:

![Malette Arcade](http://vpictu.re/uploads/2ec2f464c6d54c450da6cc68474a15afd6b5ca6c.png)

La solution à ceci est le deuxième argument REMAIN_HIGH_UNTIL_NEXTCALL qui gardera à l'état HAUT tous les boutons appuyés jusqu'à ce que la fonction demandant si ce dernier est appuyé est appelée comme ceci:

#####Exemple de code
```C
// MASTER exemple
#include "arcadeLib.h"

void setup (void)
{
  if(initArcadeLib(MASTER_MODE)){
    if(communicationTest()){
      // Communication Ok
      /** REMAIN_HIGH_UNTIL_NEXTCALL rajouté */
      activateInterrupt(EXTERNAL_PIN_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL);
    }else{
     // Impossible d'initialiser la communication (erreur)
    }
  }else{
    // Impossible d'initialiser la librairie
  }
}

void loop (void)
{
  if(isStartButtonPressed()){
     // On initialise le jeu..
  }
  
  // LONG CALCUL QUI PREND 10 SECONDES A SE FINIR ICI..
  
}
```

> **Petite explication:**
>Si le bouton est appuyé et relâché alors que le long calcul de 10 secondes est effectué, l'état du bouton gardera l'état HAUT jusqu'à l'appel de la fonction du bouton correspondant.
>Par exemple si le bouton start est appuyé pendant le calcul, le isStartButtonPressed() renverra vrai dès qu'il sera appelé. Il se remettra à zéro suite à cela.
>Si un autre bouton est appuyé (par exemple le bouton 1), l'état de ce bouton sera HAUT jusuq'à l'appel de la fonction isButton1Pressed(). **Si la fonction du bouton appuyé n'est jamais appelée, l'état du bouton restera HAUT tout du long.**

![Malette Arcade](http://vpictu.re/uploads/80550ea182a15e02b39037975329f17f44418a2a.png)


####**Les interrupts dans la librairie permettent concrètement d'avoir l'état des boutons dès que celui-ci change sans devoir appeler getActualButtonStates et getActualJoystickStates.**

####Mais alors il n'y a pas besoin d'utiliser getActualButtonStates et getActualJoystickStates ?
Oui et non. En fait, il existe beaucoup de fonctions qui utilisent les Interrupts sur l'Arduino. Par exemple, les Serial.print utilisent les Interrupts, le protocole SPI en lui-même utilise les interrupts, l'écran TFT qu'on rattache à l'Arduino (pour afficher du texte/images) utilise lui le protocole SPI (qui utilise les interrupts),  le bouton (blanc) Reset sur l'Arduino est un Interrupt en lui-même..

Ce qu'il faut savoir des interrupts, c'est que s'il y en a déjà un en cours, un autre interrupt ne pourra pas arrêter l'interrupt en cours et si deux interrupts **surviennent en même temps**, le plus important prendra le dessus et sera exécuté.
Par plus important, on entend celui qui se trouve le plus haut dans la liste suivante:

| N°| Action| ISR |
|-------------|-------------|-------------|
| 1 | Reset | |
| 2 | External Interrupt Request 0  (pin D2) |         (INT0_vect)
| 6 | Pin Change Interrupt Request 2 (pins D0 to D7)  | (PCINT2_vect)
|...| .... | ...
|18 | SPI Serial Transfer Complete                    |(SPI_STC_vect)
######Voir la liste complète sur le site de http://gammon.com.au/interrupts


Actuellement, le Slave est connecté au Master avec 5 fils: le SCK, le MISO, le MOSI, le SS, et le fil Interrupt (connecté entre le pin 2 de l'Arduino Slave et le pin 2 de l'Arduino Master).

####Code avec interrupts

Étant donné que la librairie intègre les interrupts en appelant la fonction activateInterrupt sur le pin 2, il est donc tout à fait possible d'éviter d'appeler les fonctions getActualButtonStates() et getActualJoystickStates() comme ceci:

#####Exemple de code (Code 5)
```C
// MASTER exemple without getActualButtonStates() and getActualJoystickStates()
// Inclure la librairie
#include "arcadeLib.h"

void setup (void)
{
  if(initArcadeLib(MASTER_MODE)){
    if(communicationTest()){
      // Communication Ok
      activateInterrupt(EXTERNAL_PIN_INTERRUPT);
    }else{
     // Impossible d'initialiser la communication (erreur)
    }
  }else{
    // Impossible d'initialiser la librairie
  }
}

void loop (void)
{
  if(isStartButtonPressed()){
     // On initialise le jeu..
  }
}
```


> **Attention, à prendre note:**
> Dans ce cas-là, le Master interroge seulement le Slave lorsque l'état sur le pin Interrupt 2 change (de HAUT à BAS ou de BAS à HAUT).
> Et comme le Slave change d'état (HAUT vers BAS ou BAS vers HAUT) lorsqu'il y a un changement d'état au niveau des boutons, la fonction isStartButtonPressed() est vraie (si le bouton Start est appuyé).
> Si par exemple, 2 boutons sont appuyés en même temps: le bouton Start et le bouton 1 et qu'ensuite le bouton 1 est relâché, la condition isStartButtonPressed() sera appelée 2 fois (une fois quand les deux boutons ont été touché et une autre fois quand le bouton 1 a été relâché).


##Les limites des interrupts dans la librairie
###Pratique mais..
Les interrupts, ça peut sembler très pratique à première vue mais dans certains cas, cela peut ne pas faire ce que l'on veut.

Comme dit plus haut, il ne peut y avoir 2 interrupts en même temps. Si un est déjà exécuté et qu'un se présente alors qu'un s'exécute, ce dernier ne sera pas pris en compte.

Cela ne pose pas de problèmes si votre code n'utilise, à priori, pas les interrupts comme le ferait le code 5 plus haut. Mais cela peut être tout autre dans certains cas.

###Écran TFT, SPI, I2C
Dès lors que vous branchez un écran TFT au Master ou que vous discutez avec plusieurs Slaves depuis le Master ou tout simplement en i2c (un autre protocole que le SPI), il risque d'y avoir des soucis de cohésion. Dans ce cas-là, les autres interrupts risquent d'empiéter sur les interrupts de la librairie et donc les interrupts de la librairie risquent de ne pas fonctionner.

**Dans ce cas-là, il vaut mieux garder les appels de fonction getActualButtonStates() et getActualJoystickState() ** 
#####Exemple de code (Code 6)
```C
// MASTER exemple
// Inclure la librairie
#include "arcadeLib.h"
#include <TFT.h>

#define CS   9
#define DC   8
#define RESET  7

TFT TFTscreen = TFT(CS, DC, RESET);

void setup (void)
{
  if(initArcadeLib(MASTER_MODE)){
    if(communicationTest()){
      // Communication Ok
    }else{
     // Impossible d'initialiser la communication (erreur)
    }
  }else{
    // Impossible d'initialiser la librairie
  }
  TFTscreen.begin();

  TFTscreen.stroke(255,255,255);
  TFTscreen.setTextSize(1.5);
}

void loop() {
  TFTscreen.stroke(255,255,255);
  TFTscreen.text("StartButton: ", 0, 10);
  
  getActualButtonStates();
  
  TFTscreen.text((isStartButtonPressed()) ? "Pressed" : "Not pressed", 80, 10);

  TFTscreen.stroke(0,0,0);
  TFTscreen.fill(0, 0, 0);
  TFTscreen.rect(80, 0, 100, 20);

}

```

> **Note:**
> Il est tout à fait possible d'utiliser l'appel à la fonction getActualButtonStates() ou getActualJoystickStates() plus bas dans le code. Il est même **conseillé** des les appeler avant juste qu'on vérifie l'état des boutons.

##Différent Slave Select

Il peut arriver qu'une librairie utiliser le pin 10 pour quelque chose et qu'il ne soit pas possible d'utiliser ce pin pour le Slave Select. Pour cela, il suffit de rajouter le numéro du pin au deuxième argument de la fonction initArcadeLib:

```C
int initArcadeLib(Mode mode, int SlaveSelectPin);
```

De base, le pin 10 est utilisé mais si un numéro est fourni au deuxième argument de la fonction initArcadeLib, c'est ce pin qui sera utilisé comme pin SS.


##Multiples Slave
Il peut arriver qu'il y ait d'autres Slave qui soient rattachés au Master. Par exemple l'écran TFT Arduino utilise le protocole SPI (l'écran est un Slave).

Comme on le sait par le protocole SPI, pour communiquer avec un Slave précis, il faut mettre le pin SS de celui-ci en LOW et celui de tous les autres Slave en HIGH.

La librairie met en LOW lorsqu'elle doit parler au Slave mais elle ne sait pas qu'il y a d'autres Slave. Pour lui indiquer ceci, il faut appeler la fonction:

```C
void addSSPInToDesactivate(int listToPutOnHigh[]);
```

La fonction attend une liste de pins SS à mettre en HIGH. Il peut y avoir jusqu'à 6 Slave/pins à mettre en état HAUT (pour signaler qu'on ne va pas communiquer avec ces Slaves).

Voici un cas concret de l'utilisation de ceci:


```C
// MASTER exemple

#include "arcadeLib.h"
#include <TFT.h>

#define CS    10
#define DC     9
#define RESET  8

#define SS 5

TFT TFTscreen = TFT(CS, DC, RESET);

int pinToPutHigh[] = {CS}; // liste des pins à mettre en état HIGH

void setup (void)
{

  if(initArcadeLib(MASTER_MODE, SS)){ // Le pin SS du Master est sur le pin 5 
    if(communicationTest()){
      // Communication Ok
	  
      addSSPInToDesactivate(pinToPutHigh);
    }else{
     // Impossible d'initialiser la communication (erreur)
    }
  }else{
    // Impossible d'initialiser la librairie
  }

  
  TFTscreen.begin();
  TFTscreen.background(0, 0, 0);
  TFTscreen.stroke(255,255,255);
  TFTscreen.setTextSize(1.5);
}

void loop() {
  getActualButtonStates();
  
  TFTscreen.stroke(255,255,255);
  TFTscreen.text("StartButton: ", 0, 10);

  TFTscreen.text((isStartButtonPressed()) ? "Pressed" : "Not pressed", 80, 10);
  
  TFTscreen.stroke(0,0,0);
  
  TFTscreen.fill(0, 0, 0);
  TFTscreen.rect(80, 0, 100, 20);
}

```

##Slave

Le slave lui aussi utilise la librairie, cependant contrairement au Master, son code est fixé. En effet, mis à part répondre aux demandes et à regarder l'état des boutons, le Slave ne fait pas grand chose d'autre.

Le seul code dont le Slave a besoin pour fonctionner est le suivant:
```C
// Slave exemple
#include "arcadeLib.h"


// Those variables are in the Borne_Arcade_Lib.cpp. We use "extern" to say that these variables are declared into an other file
extern volatile uint8_t lastButtonState;
extern volatile uint8_t lastJoystickState;
extern volatile bool    isInterruptActivated;

uint8_t invert = LOW;
uint8_t actualButtonState = 0x00;
uint8_t actualJoystickState = 0x00;

void setup() {

  if (initArcadeLib(SLAVE_MODE)) {
    // All is ok
  }

  pinMode(2, OUTPUT);
  digitalWrite(2, invert);
}

void loop() {
  // Get the button pin states
  actualButtonState = getButtonPinStates();
  // Get the joystick pin states
  actualJoystickState = getJoystickPinStates();

  // If there is something different between last and actual button states
  if (actualButtonState != lastButtonState) {
    noInterrupts(); // Use noInterrupts: We don't want this to be interrupted (by SPI communication)
    lastButtonState = actualButtonState;
    interrupts(); // We allow interrupts again

    /* Change the pin state of pin 2 (if it was HIGH it will be LOW, 
    if it was LOW it will be HIGH) */
    // This is only done if
    invert = (invert == LOW) ? HIGH : LOW;
    if (isInterruptActivated)
      digitalWriteFast(2, invert);

  } else if (actualJoystickState != lastJoystickState) {
    noInterrupts();
    lastJoystickState = actualJoystickState ;
    interrupts();

    invert = (invert == LOW) ? HIGH : LOW;

    if (isInterruptActivated)
      digitalWriteFast(2, invert);
  }

}

```

###Fonctions utilisables dans le Slave

| Fonction| Action| 
|-------------|-------------|
| void getButtonPinStates() | Lit toutes les pins où se trouvent les boutons 1 par 1 et stocke tout ceci dans la variable **volatile uint8_t lastButtonState** |
| void getJoystickPinStates() | Lit toutes les pins où se trouvent le joystick 1 par 1 et stocke tout ceci dans la variable **volatile uint8_t lastJoystickState** |         (INT0_vect)

###Concaténation des valeurs de pins

Étant donné que le protocole SPI communique en 8 bits, il a fallu trouver un moyen rapide de communiquer l'état des boutons et du joystick au Master.

Nous avons 8 boutons et 4 états pour le joystick. Afin d'être le plus rapide possible, nous avons décidé d'affecter un bit à chaque bouton. Si le bit était à 1, le bouton est appuyé, si le bit est à 0, le bouton n'est pas appuyé.

Pour les boutons, on stocke tout ceci dans une variable uint8_t (8 bits) nommée lastButtonState avec les valeurs suivantes:

|Position dans la variable lastButtonState | Bouton| N° sur l'image ci-dessous| 
|-------------|:-------------:|-------------:|
|0|Start|1|
|1|Stop|2|
|2|Bouton1|3|
|3|Bouton2|4|
|4|Bouton3|5|
|5|Bouton4|6|
|6|Bouton5|7|
|7|Bouton6|8|

> **Note:**
> Par exemple si seulement le bouton Start est appuyé, la variable lastButtonState vaudra: 10000000 (0x80).
> Si le bouton start et le bouton stop sont appuyés, cela donnera: 11000000 (0xC0)
> Si tous les boutons sont appuyés en même temps la variable lastButtonState vaudra: 11111111 (0xFF)
> etc..
> 

<img src="http://vpictu.re/uploads/83b48fbb17ab8f42b69d0fbca193625f3acab6ae.png" alt="SPI Arcade Lib" />

Pour le Joystick, on stocke tout ceci dans une variable uint8_t (8 bits) nommée lastJoystickState. Cependant, étant donné que le Joystick peut avoir 4 états (TOP, LEFT, BOTTOM, RIGHT), il n'y aura que 4 bits utilisés sur les 8. Ces 4 bits significatifs seront les 4 premiers de la variable. Les 4 autres bits ne sont pas pris en compte.

Comme ceci:

|Position dans la variable lastJoystickState | Joystick| N° sur l'image ci-dessus| 
|-------------|:-------------:|-------------:|
|0|Top|9|
|1|Left|10|
|2|Bottom|11|
|3|Right|12|
|4|X|X|
|5|X|X|
|6|X|X|
|7|X|X|


##Timing

Voici le timing que prennent différentes actions, tant au niveau du Master qu'au niveau du Slave (ou de l'un à l'autre).

|Slave/Master|Départ | Arrivée| Temps|Commentaire|
|-------------|:-------------|-------------:|-------------:|-------------|
|Master|getActualButtonStates()|loop()|175μs|Le temps que prend toute la demande, de la demande à la réponse du Slave
|Master|getActualJoystickStates()|loop()|160μs|Le temps que prend toute la demande, de la demande à la réponse du Slave
|Slave|Interrupt pin (D2)|getActualButtonStates()| 8μs|Du moment où le Slave change l'état de la pin 2 (interrupt) jusqu'au moment où le Master détecte le changement d'état et atteint la fonction getActualButtonStates()
|Slave|Interrupt SPI (ISR)|Envoi de réponse au Master|40μs|Du moment où le Slave reçoit une demande du Master et va chercher l'état des pins



Boris GAUDARD, Dardan ILJAZI
Février 2017

