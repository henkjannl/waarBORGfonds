#ifndef DATA_H
#define DATA_H

//=====================
// DEFINES
//=====================
#define RADAR_ARRAY_SIZE             70 // Number of ultrasonic measurements per sweep of the radar
#define STEPS_BETWEEN_SAMPLES        45 // Number of steps between ultrasonic measurements 
#define MICROSECONDS_BETWEEN_STEPS 1000 // Number of µs between steps

#define NUM_CHALLENGES                7 // Number of challenges

#define BOTTLE_PRESENT_THRESHOLD 365.0  // A bottle should be nearer than this distance to be detected

// ======== DATA TYPES ============= 

class tPixel {
  public:
    int x;
    int y;

    tPixel()             { this->x=0; this->y=0; };
    tPixel(int x, int y) { this->x=x; this->y=y; };
    //tPixel(tPixel& p)     { this->x=p->x; this->y=p->y; };
};

enum tMainState {msWelcomeScreen, msConfirmChallenge, msRadar, msAnswerCorrect, msCompleted };
enum tAnswer {anNoBottles, anA, anB, anC };

typedef struct {
  char title[25];  // First line of title
  tAnswer answer;  // At which position should a bottle be present?
} tChallenge;

// ======== CONSTANTS ================


// List the challenges, including the final one
static const tChallenge challenges[] = {
  { "1", anB }, // Formule van PEEK 
  { "2", anC }, // Kosten van het part
  { "3", anA }, // Gezondste manier om op je werk te komen
  { "4", anA }, // Fietsroute
  { "5", anB }, // Hardware rules
  { "6", anC }, // Altijd hard werken 
  { "7", anC }  // Softwarebugs
};


// ======== GLOBAL VARIABLES ============= 

float radarMeasurements[RADAR_ARRAY_SIZE]; // Ultrasonic distance measurements, millimeters from radar
tMainState mainState = msWelcomeScreen;    // Main state of the program
int challengeID = 0;                       // Which of the challenges is currently playing
bool radarSpinning = false;                // Radar spinning or not
bool updateScreen = true;                  // Screen must be updated
bool radarSweepFinished = false;
tAnswer bottleLocations=anNoBottles;
String debug=String("-");

unsigned long keyBoardWatchDog = 0;
portMUX_TYPE radarMux = portMUX_INITIALIZER_UNLOCKED;

#endif
