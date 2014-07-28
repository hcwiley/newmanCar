/*
 Copyright (C) 2014 H. Cole Wiley <cole@wileycousins.com>
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the MIT LICENSE.
 */

#define USE_SERIAL 1

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(7,8);

// global variable to store distance in
int distance = 0;

// pin definitions
const int trigger = 2;
const int echo = 4;

const int motor1A = A0;
const int motor1B = A1;
const int motor2A = A2;
const int motor2B = A3;

void forward() {
  if(USE_SERIAL)
    printf("forward\n\r");
  digitalWrite(motor1A, LOW);
  digitalWrite(motor1B, HIGH);
  digitalWrite(motor2A, LOW);
  digitalWrite(motor2B, HIGH);
}

void backward() {
  if(USE_SERIAL)
    printf("backward\n\r");
  digitalWrite(motor1A, HIGH);
  digitalWrite(motor1B, LOW);
  digitalWrite(motor2A, HIGH);
  digitalWrite(motor2B, LOW);
}

void halt() {
  if(USE_SERIAL)
    printf("halt\n\r");
  digitalWrite(motor1A, LOW);
  digitalWrite(motor1B, LOW);
  digitalWrite(motor2A, LOW);
  digitalWrite(motor2B, LOW);
}

void left() {
  if(USE_SERIAL)
    printf("left\n\r");
  digitalWrite(motor1A, LOW);
  digitalWrite(motor1B, HIGH);
  digitalWrite(motor2A, HIGH);
  digitalWrite(motor2B, LOW);
}

void right() {
  if(USE_SERIAL)
    printf("right\n\r");
  digitalWrite(motor1A, HIGH);
  digitalWrite(motor1B, LOW);
  digitalWrite(motor2A, LOW);
  digitalWrite(motor2B, HIGH);
}

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 
  0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void setup(void)
{
  if(USE_SERIAL){
    Serial.begin(57600);
    printf_begin();
  }
  // setup motors
  pinMode(motor1A, OUTPUT);
  pinMode(motor1B, OUTPUT);
  pinMode(motor2A, OUTPUT);
  pinMode(motor2B, OUTPUT);
  
  // setup sonar
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
  
  // setup RF
  //
  // Setup and configure rf radio
  //
  delay(500);
  radio.begin();

  // set the channel
  radio.setChannel(10);
  
  // optionally, increase the delay between retries & # of retries
//  radio.setRetries(15,15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
//  radio.setPayloadSize(8);
  radio.enableDynamicPayloads();

  // Open pipes to other nodes for communication
  //
  delay(500);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  

  //
  // Start listening
  //
  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  if(USE_SERIAL){
    printf("\n\r***** CONFIG *****\n\r");
    radio.printDetails();
    printf("\n\r***** END CONFIG *****\n\r");
  }
  
  forward();
  delay(500);
  halt();
  delay(200);
  backward();
  delay(500);
  halt();
  delay(200);

}

void loop(void)
{
  // if there is data ready
  if ( radio.available() )
  {
    // Dump the payloads until we've gotten everything
    char data;
    bool done = false;
    while (!done)
    {
      // Fetch the payload, and see if this was the last one.
      done = radio.read( &data, sizeof(char) );

      // Spew it
      printf("Got payload: %c...\n\r",data);

      if (data == 'F' )
        forward();
      else if (data == 'L' )
        left();
      else if (data == 'R' )
        right();
      else if (data == 'S' ) {
        halt();

        // Delay just a little bit to let the other unit
        // make the transition to receiver
        delay(20);

        // First, stop listening so we can talk
        radio.stopListening();

        // Send the final one back.
        getDistance();
        radio.write( &distance, sizeof(int) );
        if(USE_SERIAL)
          printf("Sent distance: %d.\n\r", distance);

        // Now, resume listening so we catch the next packets.
        radio.startListening();
      }
    }
  }
}

void getDistance() {
  // establish variables for duration of the ping, 
  // and the distance result in inches and centimeters:
  long duration, inches, cm;

  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigger, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH
  // pulse whose duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  duration = pulseIn(echo, HIGH);

  // convert the time into a distance
  distance = (int)microsecondsToInches(duration);
  
//  cm = microsecondsToCentimeters(duration);
  
  if(USE_SERIAL){
    Serial.print(distance);
    Serial.println("in");
//    Serial.print(cm);
//    Serial.print("cm");
//    Serial.println();
  }
}

int microsecondsToInches(long microseconds)
{
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74 / 2;
}

int microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}

