#ifndef DATA_H
#define DATA_H

//=====================
// DEFINES
//=====================
#define RADAR_ARRAY_SIZE             70 // Number of ultrasonic measurements per sweep of the radar
#define STEPS_BETWEEN_SAMPLES        50 // Number of steps between ultrasonic measurements 
#define MICROSECONDS_BETWEEN_STEPS 1000 // Number of µs between steps

#define CHALLENGE_MAX 3                 // Number of challenges
#define NUM_BOTTLE_LOCATIONS 7

#define BOTTLE_PRESENT_THRESHOLD 350.0                  // A bottle should be nearer than 350 mm to be detected

// ======== DATA TYPES ============= 

class tPixel {
  public:
    int x;
    int y;

    tPixel()             { this->x=0; this->y=0; };
    tPixel(int x, int y) { this->x=x; this->y=y; };
    //tPixel(tPixel& p)     { this->x=p->x; this->y=p->y; };
};

enum tMainState {msFirstChallenge, msConfirmChallenge, msRadar, msAnswerCorrect };

typedef struct {
  char titleLine1[25];                   // First line of title
  char titleLine2[25];                   // Second line of title
  bool hasBottle[NUM_BOTTLE_LOCATIONS];  // At which positions a bottle should be present?
} tChallenge;

// ======== CONSTANTS ================

static const uint8_t bottleLocations[] = { 20, 25, 30, 35, 40, 45, 50 }; // Indices with predefined locations of bottles

// List the challenges, including the final one
static const tChallenge challenges[] = {
  // titleLine1              titleLine2         pos0   pos1   pos2   pos3   pos4   pos5   pos6
  {  "01", "02", {  true,  false, true,  false, false, false, false } },
  {  "02", "02", {  false, true,  false, false, true,  false, false } },
  {  "JE HEBT",              "GEWONNEN!",    {  false, false, false, false, false, false, false } },
};


// ======== GLOBAL VARIABLES ============= 

float radarMeasurements[RADAR_ARRAY_SIZE]; // Ultrasonic distance measurements, millimeters from radar
tMainState mainState = msFirstChallenge;   // Main state of the program
int challengeID = 0;                       // Which of the challenges is currently playing
bool radarSpinning = false;                // Radar spinning or not
bool updateScreen = true;                  // Screen must be updated

unsigned long keyBoardWatchDog = 0;
portMUX_TYPE radarMux = portMUX_INITIALIZER_UNLOCKED;

#endif
