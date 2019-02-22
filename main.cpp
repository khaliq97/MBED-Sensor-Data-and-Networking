#include "mbed.h"
#include "C12832.h"
#include "EthernetInterface.h"
#include "LM75B.h"
#include <string>
#include <cstdlib>

/**
  *Using an external library.
  *Name: CRC-8-CCITT
  *Site: https://3dbrew.org/wiki/CRC-8-CCITT
  *Files: crc8.h and crc8.cpp
  *Author: Socram8888 (Only alias that could be found)
  *I am using this library to get the checksum of the data packet that I'm sending, which is then sent to the server for it to verify it has calculated the same checksum.
  *This comment is to show that I used someone else's library, and they have been referenced.
**/
#include "crc8.h"


LM75B sensor(D14, D15);
C12832 lcd(D11, D13, D12, D7, D10);


InterruptIn upJoystick(A2);
InterruptIn downJoystick(A3);
InterruptIn leftJoystick(A4);
InterruptIn righttJoystick(A5);
InterruptIn fireJoystickButton(D4);

InterruptIn sw2(SW2);
InterruptIn sw3(SW3);

//Two 8 bit values for the sender ID.
uint8_t senderIDFirst = 0xD;
uint8_t senderIDSecond = 0xE3;

volatile uint8_t temp = sensor.read();

//Two 8 bit values for the temperature.
volatile uint8_t tempFirst = temp;
volatile uint8_t tempSecond = temp >> 8;

volatile int sequenceNumber = 0;

//The base number for the jitter.
volatile int jitterBaseNumber = 2;

const char* host = "lora.kent.ac.uk";
int port = 1789;

Ticker sendPacketTicker;

Ticker recievePacketTicker;

volatile int sendPacketFlag = 0;
volatile int receivePacketFlag = 0;
volatile int upJoystickPressedFlag = 0;
volatile int downJoystickPressedFlag = 0;
volatile int leftJoystickPressedFlag = 0;
volatile int rightJoystickPressedFlag = 0;
volatile int fireJoystickPressedFlag = 0;

//Button bit mask
const unsigned char sw2BitMask = 0x1; //0
const unsigned char sw3BitMask = 0x2; //1
const unsigned char upJoystickBitMask = 0x4; //2
const unsigned char downJoystickBitMask = 0x8; //3
const unsigned char leftJoystickBitMask = 0x10; //4
const unsigned char rightJoystickBitMask = 0x20; //5
const unsigned char fireJoystickButtonBitMask = 0x40; //6


//Option flag bit mask
const unsigned char ackFlag = 0x01; //0
const unsigned char citFlag = 0x2; //0

volatile int ackFlagBool = 0;

volatile uint8_t buttonFlags = 0;

volatile uint8_t packetOptionFlags = 0;

uint8_t packet[8];

EthernetInterface ethernetInterface;
UDPSocket udpSocket;

void receivePacketNeeded()
{
    receivePacketFlag = 1;
}   

void sendPacketNeeded()
{
    sendPacketFlag = 1;
}

void sw2ButtonPressed()
{
    buttonFlags |= sw2BitMask;
}

void sw3ButtonPressed()
{
    buttonFlags |= sw3BitMask;
}

void upJoystickPressed()
{
    buttonFlags |= upJoystickBitMask;
}

void downJoystickPressed()
{
    buttonFlags |= downJoystickBitMask;
}

void leftJoystickPressed()
{
    buttonFlags |= leftJoystickBitMask;
}

void rightJoystickPressed()
{
    buttonFlags |= rightJoystickBitMask;
}

void fireJoystickButtonPressed()
{
    buttonFlags |= fireJoystickButtonBitMask;
}

/**
  *Checks for an acknowledgement packet from the server.
  *Creates a random wait time if a packet is not recieved.
**/
void recievePacket()
{  

if(ackFlagBool)
{
    int jitterRand = 0;

    uint8_t recvPacket[3];

    udpSocket.set_timeout(2.0);
    int recievePacketSize = udpSocket.recvfrom(NULL, recvPacket, sizeof(recvPacket));
    if(recievePacketSize < 0)
    {
        wait(jitterBaseNumber);
        
        jitterBaseNumber = jitterBaseNumber * 2;
        
        //Produces a random number for the jitter
        jitterRand = (rand() % 30) +1;
      
        jitterBaseNumber = jitterBaseNumber + jitterRand;
    
    }
        ackFlagBool = 0;
    } 
}

/**
  *Sends an 8 byte packet to the server.
  *Checks for acknowledgement packet after every sent packet.
**/
void sendPacket()
{

//Set CIT flag to true.
packetOptionFlags |= citFlag;

packet[0] = senderIDFirst; //Switch this around!!!!!!!
packet[1] = senderIDSecond;
packet[2] = sequenceNumber; 
packet[3] = packetOptionFlags;
packet[4] = tempFirst;
packet[5] = tempSecond;
packet[6] = buttonFlags;

/**
  *This is where the crc8.h library is used. 
  *Calculates and stores the checksum of the packet in the footer of the packet.
  *Only the header's and payload's checksum is calculated.
  *crc8ccitt() function used from "crc8.h" library
**/
packet[7] = crc8ccitt(packet, 7);

udpSocket.sendto(host, 1789, packet, sizeof(packet));

recievePacket();
    
packetOptionFlags = 0;
buttonFlags = 0;

sequenceNumber++;
}


/**
  *Main method, sets up the connection to the server and manages sending and receiving packets.
**/
int main()
{

ethernetInterface.connect();
udpSocket.open(&ethernetInterface);

sendPacketTicker.attach(sendPacketNeeded, 10.0);
recievePacketTicker.attach(receivePacketNeeded, 60.0);

sw2.fall(sw2ButtonPressed);
sw3.fall(sw3ButtonPressed);
upJoystick.fall(upJoystickPressed);
downJoystick.fall(downJoystickPressed);
leftJoystick.fall(leftJoystickPressed);
righttJoystick.fall(rightJoystickPressed);
fireJoystickButton.fall(fireJoystickButtonPressed);


while(true) 
{
    
    if(receivePacketFlag)
    {
        packetOptionFlags |= ackFlag;
        ackFlagBool = 1;
        receivePacketFlag = 0;
    }
        
    if(sendPacketFlag)
    {
        //Update and bit shift temp before sending packet
        temp = sensor.read();
        tempFirst = temp;
        tempSecond = temp >> 8;
        
        sendPacket();
        sendPacketFlag = 0;
    }
    

}

}

