#ifdef LOAD_TEST

#include "HX711.h"
#define DT_PIN   26
#define SCK_PIN  25

HX711 scale;

void setup(){
    Serial.begin(115200);
    scale.begin(DT_PIN, SCK_PIN);
}

void loop(){
    if (scale.is_ready()){
        Serial.println(String(millis()/1000, 3) + " " +String(scale.read()));
    }
}

#endif