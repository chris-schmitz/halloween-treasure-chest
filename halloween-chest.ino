#include <FastLED.h>
#include <Servo.h>

// * Ultrasonic config
#define TRIGGER_PIN 4
#define ECHO_PIN 3

// * Servo config
#define LID_PIN 0

// * LED strip config
#define LED_PIN 2
#define NUM_LEDS 30
#define COLOR_ORDER GRB
#define CHIPSET WS2811
#define BRIGHTNESS 200
#define FRAMES_PER_SECOND 30

bool gReverseDirection = false;
CRGB leds[NUM_LEDS];

Servo lid;
int startingAngle = 30;
int endingAngle = 120;

void setup()
{
    Serial.begin(9600);
    Serial.println("Starting up");

    // * Servo setup
    lid.write(startingAngle);
    lid.attach(LID_PIN);

    // * Ultrasonic setup
    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // * LED strip setup
    FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
}

unsigned long servoInterval = 50;
unsigned long servoIntervalLastChecked = 0;
unsigned int currentAngle = startingAngle;
unsigned int targetAngle = startingAngle;

unsigned long ultrasonicInterval = 500;
unsigned long ultrasonicIntervalLastChecked = 0;
float lidActivationThreshold = 30.0; // * in cm

unsigned long ledStripInterval = 1000 / FRAMES_PER_SECOND;
unsigned long ledStripIntervalLastChecked = 0;

void loop()
{

    unsigned long now = millis();

    if (now - ultrasonicIntervalLastChecked >= ultrasonicInterval)
    {
        digitalWrite(TRIGGER_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIGGER_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIGGER_PIN, LOW);

        float duration = pulseIn(ECHO_PIN, HIGH);
        float distance = (duration * 0.0343) / 2;

        if (distance < lidActivationThreshold)
        {
            long angle = map(distance, lidActivationThreshold, 0, startingAngle, endingAngle);

            Serial.print("Setting target angle to:");
            Serial.println(angle);
            targetAngle = angle;
        }

        ultrasonicIntervalLastChecked = now;
    }

    if (now - servoIntervalLastChecked >= servoInterval)
    {
        servoStepTowardsTarget();
    }

    if (now - ledStripIntervalLastChecked >= ledStripInterval && targetAngle > startingAngle)
    {
        Fire2012();
        FastLED.show();
        ledStripIntervalLastChecked = now;
    }
}

void servoStepTowardsTarget()
{
    if (targetAngle - currentAngle == 0)
    {
        // Serial.println("At the target angle, early exit");
        return;
    }

    int difference = targetAngle - currentAngle;

    Serial.print("Target angle: ");
    Serial.print(targetAngle);
    Serial.print(" Current angle: ");
    Serial.print(currentAngle);
    Serial.print(" difference: ");
    Serial.println(difference);

    if (difference > 0)
    {
        currentAngle++;
        currentAngle = min(currentAngle, endingAngle);
    }
    else
    {
        // Serial.println("Negative move towards the target");
        currentAngle--;
        currentAngle = max(currentAngle, startingAngle);
    }

    lid.write(currentAngle);
}

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
////
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation,
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking.
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
#define COOLING 55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

void Fire2012()
{
    // Array of temperature readings at each simulation cell
    static byte heat[NUM_LEDS];

    // Step 1.  Cool down every cell a little
    for (int i = 0; i < NUM_LEDS; i++)
    {
        heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int k = NUM_LEDS - 1; k >= 2; k--)
    {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKING)
    {
        int y = random8(7);
        heat[y] = qadd8(heat[y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (int j = 0; j < NUM_LEDS; j++)
    {
        CRGB color = HeatColor(heat[j]);
        int pixelnumber;
        if (gReverseDirection)
        {
            pixelnumber = (NUM_LEDS - 1) - j;
        }
        else
        {
            pixelnumber = j;
        }
        leds[pixelnumber] = color;
    }
}
