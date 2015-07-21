#include <jack/jack.h>
#include <jack/midiport.h>
#include "common.h"
#include <string>
#include <sstream>
#include <stdio.h>
#include <queue>

using namespace std;

queue<unsigned char *> cc_queue;

void queue_up_single_cc(int controller, int value)
{
   unsigned char *buffer=new unsigned char[3];

   buffer[0]=0xB0;
   buffer[1]=controller;
   buffer[2]=value;

   cc_queue.push(buffer);
}

void send_out_any_waiting_cc_messages(void  *midi_port_buf)
{ 
   while (cc_queue.empty()==false)
   {
      unsigned char *buffer = jack_midi_event_reserve(midi_port_buf, 0, 3);
      if(buffer==NULL) 
      {
         return; //whoops, no more space to send message right now. wait for later.
      }
      
      unsigned char *b=cc_queue.front();
      cc_queue.pop();
 
      buffer[0]=b[0];
      buffer[1]=b[1];
      buffer[2]=b[2];
 
      delete(b);
  }
}
    
   

  

