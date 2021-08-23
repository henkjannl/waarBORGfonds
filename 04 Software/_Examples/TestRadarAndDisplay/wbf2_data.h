#ifndef DATA_H
#define DATA_H

//=====================
// DEFINES
//=====================
#define RADAR_ARRAY_SIZE             71 // Number of ultrasonic measurements per sweep of the radar
#define STEPS_BETWEEN_SAMPLES        48 // Number of steps between ultrasonic measurements 
#define MICROSECONDS_BETWEEN_STEPS 1000 // Number of ms between steps
#define DISTANCE_MAX                400 // Largest measurable distance in mm

// ======== CONSTANTS ================

// ======== DATA TYPES ============= 
enum tScreen          {scRadar};
enum tGameplayStep    {gpIdle,    gpQ1,            gpQ2};

typedef struct _tCanPositions
{
  int pos1;
  int pos2;
} tCanPositions;

class tPixel {
  public:
    int x;
    int y;

    tPixel()             { this->x=0; this->y=0; };
    tPixel(int x, int y) { this->x=x; this->y=y; };
    //tPixel(tPixel& p)     { this->x=p->x; this->y=p->y; };
};

class tData 
{
  public:
  
    //screen
    tScreen screen=scRadar;
    bool btnPlayTouched=false; // button 1
    bool btnOkTouched=false;   // button 2
        
    //gameplay
    tGameplayStep gameStep=gpIdle;
    bool allAnswersOk=false;

};    





// ======== GLOBAL VARIABLES ============= 
tData data;     // Data, lost if device is switched off

// ultrasonic distance measurements
int64_t radarMeasurements[RADAR_ARRAY_SIZE]; // Microseconds passed

portMUX_TYPE dataAccessMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE configAccessMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE radarMux = portMUX_INITIALIZER_UNLOCKED;

#endif
