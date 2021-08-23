#ifndef DATA_H
#define DATA_H

//=====================
// DEFINES
//=====================
#define RADAR_ARRAY_SIZE             70 // Number of ultrasonic measurements per sweep of the radar
#define STEPS_BETWEEN_SAMPLES        50 // Number of steps between ultrasonic measurements 
#define MICROSECONDS_BETWEEN_STEPS 1000 // Number of µs between steps

#define CHALLENGE_MAX 3                 // Number of challenges

// ======== CONSTANTS ================

// ======== DATA TYPES ============= 
enum tScreen          {scStartChallenge, scDisplayChallenge, scRadarRed, scRadarGreen };


typedef struct {
  char titleLine1[25];           // First line of title
  char titleLine2[25];           // Second line of title
  int CanPos1                    // Position of first can  (-1 is not needed)
  int CanPos2                    // Position of second can (-1 is not needed)
  int CanPos3                    // Position of third can  (-1 is not needed)
} tChallenge;


// List the challenges, including the final one
static const tChallenge challenges[] = {
  {  "1) Welke programma's", "zijn correct", 600, 1200, 1800 },
  {  "2) Welke programma's", "zijn correct", 600, 1200,   -1 },
  {  "JE HEBT",              "GEWONNEN!"   ,  -1,   -1,   -1 }
};

class tPixel {
  public:
    int x;
    int y;

    tPixel()             { this->x=0; this->y=0; };
    tPixel(int x, int y) { this->x=x; this->y=y; };
    //tPixel(tPixel& p)     { this->x=p->x; this->y=p->y; };
};





// ======== GLOBAL VARIABLES ============= 

// ultrasonic distance measurements
float radarMeasurements[RADAR_ARRAY_SIZE]; // Millimeters from radar

tScreen screen = scStartChallenge; // State machine for different screens
int challengeID = 0;               // Which of the challenges is currently playing
bool radarSpinning = false;

portMUX_TYPE dataAccessMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE configAccessMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE radarMux = portMUX_INITIALIZER_UNLOCKED;

#endif
