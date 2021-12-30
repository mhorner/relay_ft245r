
/*

MIT License

Copyright (c) 2019 Marcelo Varanda

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
https://osdn.net/projects/mingw/releases/
https://osdn.net/projects/mingw/downloads/68260/mingw-get-setup.exe/

*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "FTD2XX.H"

#define FT245R_DESCRIPTION "FT245R USB FIFO"

#define HELLO \
"\nRelay FT245R - ver 0.2\n\
Forked source: https://github.com/mhorner/relay_ft245r\n\
Copyright (c) 2021 Matt Horner - MIT License\n\
Original source: https://github.com/mvaranda/relay_ft245r\n\
Copyright (c) 2019 Marcelo Varanda - MIT License\n\n"


#define USAGE \
"\nMenu:\n\
   r N on|off    - where N is the relay number (1~8) or \'all\'\n\
   list          - show all FTDI devices\n\
   quit          - quit this app\n\
   quitoff       - turn all relays off and quit this app\n\
   help          - this help\n\
   ?             - same than help\n\n"

#define MAX_NUM_PARAMS 5
#define RELAY_NUMBER_ARG "r"
#define RELAY_STATE_ARG "s"
#define DEBUG 0

char cmdBuffer[256];
char * par[MAX_NUM_PARAMS];
int num_par = 0;
FT_HANDLE ftHandle; 
byte gpio = 0;


static void list(void)
{
  //********************************************************
  //List Devices
  //********************************************************

  FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;
  DWORD numDevs;
  DWORD i;

  // create the device information list 
  int status = FT_CreateDeviceInfoList(&numDevs);

  if (status != FT_OK) {
    printf("FT_CreateDeviceInfoList status not ok %d\n", status);
  }
  else
  {
    printf("Number of devices is %d\n", (int) numDevs);
    if (numDevs > 0) {
      // allocate storage for list based on numDevs 
      devInfo =
        (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
      // get the device information list 
      status = FT_GetDeviceInfoList(devInfo, &numDevs);
      if (status == FT_OK) {
        for (i = 0; i < numDevs; i++) {
          printf("Dev %d:\n", (int) i);
          printf("Flags=0x%x\n", (int) devInfo[i].Flags);
          printf("Type=0x%x\n", (int) devInfo[i].Type);
          printf("ID=0x%x\n", (int) devInfo[i].ID);
          printf("LocId=0x%x\n", (int) devInfo[i].LocId);
          printf("SerialNumber=%s\n", devInfo[i].SerialNumber);
          printf("Description=%s\n", devInfo[i].Description);
          printf("\n");
        }
      }
      free(devInfo);
    }
  }
}

static FT_STATUS openDevice (int devIdx, FT_HANDLE * ftHandle)
{
  FT_STATUS ftStatus;
  UCHAR Mask = 0xFF; // all output (output (1) and input (0)) 
  UCHAR Mode = 0x01; // 0x01 = asynchronous bit-bang

  FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;
  DWORD numDevs;
  DWORD i;

  // create the device information list 
  int status = FT_CreateDeviceInfoList(&numDevs);

  if (status != FT_OK) {
    printf("FT_CreateDeviceInfoList status not ok %d\n", status);
    return status;
  }
  
  if (devIdx >= numDevs) {
    printf("FT245R not found\n");
    return -1;
  }
  

  printf("Number of FTDI devices is %d\n", (int) numDevs);

  // allocate storage for list based on numDevs 
  devInfo =
    (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
  // get the device information list 
  status = FT_GetDeviceInfoList(devInfo, &numDevs);
  if (status == FT_OK) {
    for (i = 0; i < numDevs; i++) {
      if (DEBUG) {
        printf("Dev %d:\n", (int) i);
        printf("Flags=0x%x\n", (int) devInfo[i].Flags);
        printf("Type=0x%x\n", (int) devInfo[i].Type);
        printf("ID=0x%x\n", (int) devInfo[i].ID);
        printf("LocId=0x%x\n", (int) devInfo[i].LocId);
        printf("SerialNumber=%s\n", devInfo[i].SerialNumber);
        printf("Description=%s\n", devInfo[i].Description);
        printf("\n");
      }
      if (strcmp(devInfo[i].Description, FT245R_DESCRIPTION) == 0) {
        printf("found FT245R at idx = %d, Serial Number: %s\n", (int) i, devInfo[i].SerialNumber);
        break;
      }
    }
  }

  free(devInfo);
  
  if (i >= numDevs) {
    printf("Could not find any FT245R device\n");
    return -2;
  }

  ftStatus = FT_Open(i, ftHandle);
  ftStatus |= FT_SetUSBParameters(*ftHandle, 4096, 4096); // Set USB transfer sizes
  ftStatus |= FT_SetChars(*ftHandle, 0, 0, 0, 0); // Disable event characters
  ftStatus |= FT_SetTimeouts(*ftHandle, 5000, 5000); // Set read/write timeouts to 5 sec
  ftStatus |= FT_SetLatencyTimer(*ftHandle, 16); // Latency timer at default 16ms
  ftStatus |= FT_SetFlowControl(*ftHandle, FT_FLOW_NONE, 0x11, 0x13); 
  ftStatus |= FT_SetBaudRate(*ftHandle, 62500); //bit rate is x16 this value = 1M if
  ftStatus |= FT_SetBitMode(*ftHandle,Mask,Mode);
  return ftStatus;
}

static void printUsage(void)
{
  printf(USAGE);
}

static void parseInput(void)
{
  char * ptr = cmdBuffer;
  //printUsage();
  printf("\n>");
  memset(cmdBuffer, 0, sizeof(cmdBuffer));
  fgets ( cmdBuffer, sizeof(cmdBuffer) - 1, stdin );

  num_par = 0;
  
  while (num_par < MAX_NUM_PARAMS) {
    while ( *ptr == ' ') { 
      *ptr = 0;
      ptr++; // skip spaces
    }
    if ((*ptr == 0) || (*ptr == '\n') || (*ptr == '\r')) {
      *ptr = 0;
      return;
    }
    par[num_par] = ptr;
    while ( (*ptr != ' ') && (*ptr != '\n') && (*ptr != '\r') ) ptr++
    ;
    
    if (*ptr == ' ') {
      *ptr = 0;
      ptr++;
    }
    num_par++;
  }
}
void cmd_r(void)
{
  printf("command r\n");
  DWORD data_written;
  FT_STATUS ftStatus;
  unsigned int n = 0;
  
  if (num_par < 3) {
    printf("r command requires two parameters: relay_number|all and on|off\n");
    return;
  }
  n = par[1][0] - 0x30;
  
  if (strcmp(par[1], "all") == 0) {
    n = 255;
  }
  else {
    if (n > 8) {
      printf("relay_number must be 1~8\n");
      return;    
    }
    n = 1 << (n-1);
  }
  
  if (strcmp(par[2], "on") == 0) {
    gpio |= n;
  }
  else if (strcmp(par[2], "off") == 0) {
    gpio &= ~n;
  }
  else {
    printf("second parameter must be on or off\n");
    return;    
  }
  
  ftStatus = FT_Write(ftHandle, &gpio, 1, &data_written); 
  if (ftStatus != FT_OK) {
    printf("FT_Write fail, code: %d\n", (int) ftStatus); //check for error 
  }
}

void processCommandLine(int numberOfArgs, char *args[]) {
  printf("\nWe'll process command line arguments here and control the relay.\n");
  char* relayNumber = "0";
  char* relayState = "off";

  for (int i = 1; i < numberOfArgs; i++) {
    if (strcmp(args[i], RELAY_NUMBER_ARG) == 0) {
      if (DEBUG) printf("Setting relay number: %s\n", args[i + 1]);
      relayNumber = args[++i];
    }
    else if (strcmp(args[i], RELAY_STATE_ARG) == 0) {
      if (DEBUG) printf("Setting relay state: %s\n", args[i + 1]);
      relayState = args[++i];
    }
  }

  par[0] = "";
  par[1] = relayNumber;
  par[2] = relayState;
  num_par = 3;
  cmd_r();
}

int main(int argc, char *argv[])
{
  FT_STATUS ftStatus; 
  DWORD data_written; // number of bytes written
  int quitoff = 0;
  
  printf(HELLO);
    
  ftStatus = openDevice(0, &ftHandle);
  if (ftStatus != FT_OK) {
    printf("openDevice fail, code: %d\n", (int) ftStatus); //check for error
    return 0;
  }

  printf("Number of arguments provided %d", argc);

  if (argc > 1) {
    processCommandLine(argc, argv);
    
    ftStatus = FT_Close(ftHandle);

    if (DEBUG) printf("Close status, %i", (int) ftStatus);
    return 0;
  }

  printUsage();
  
  while(1) {
    parseInput();
    if (num_par == 0) {
      continue;
    }
    if (strcmp(par[0], "r") == 0)
      cmd_r();
    else if (strcmp(par[0], "list") == 0)
     list();
    else if (strcmp(par[0], "quit") == 0)
      break;
    else if (strcmp(par[0], "quitoff") == 0) {
      quitoff = 1;
      break;
    }
    else if (strcmp(par[0], "?") == 0)
      printUsage();
    else if (strcmp(par[0], "help") == 0)
      printUsage();
    else {
      printf("Invalid command.\n\n");
      printUsage();
    }
  }

  if (quitoff) {
    gpio = 0x00;
    FT_Write(ftHandle, &gpio, 1, &data_written); 
  }

  ftStatus = FT_Close(ftHandle); 

  return 1;
}

