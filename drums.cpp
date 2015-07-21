#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <list>

#include "../midi-keyboard/audio.h"
#include "../midi-keyboard/midi.h"

#include <sndfile.hh>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "single_drum.h"
#include <Delay.h>
#include <NRev.h>

#include "comb_filter.h"
#include "common.h"
#include "simple_parameter.h"

#include <libconfig.h++>
// #include <linux/getcpu.h>
      #include <sched.h>

#include "echo_effect.h"


float *recording_buffer;
unsigned int total_frames=0;

bool do_record=true;
unsigned int max_recording;

int count=0;
DrumVec dv;

comb_filter *cf;
comb_filter *cf2;

bool pedal1_pressed=false;

stk::NRev *rev;

void* midi_port_buf;
bool first_time=true;
int current_instrument;

int highest_sample=0;
single_drum *active_samples[6];
bool delay_pedal_reverse=false;
echo_effect *ee;

wave_generator *ewg;

SimpleParamVec spv;

#define SP_DELAY_FEEDBACK 0
#define SP_DELAY_TIME 1
#define SP_DELAY_INPUT 2
#define SP_BOOST_POST 3
#define SP_DPED_REVERSE 4

int (*obtain_midi_events)(int);
int (*setup_midi)(std::string);

void (*setup_audio)(int port);
void (*audio_shutdown)();
void (*send_audio_to_card)(float *,int,bool);
void (*audio_loop)();

void map_midi_functions_for_jack()
{
   obtain_midi_events=&obtain_midi_events_jack;
   setup_midi=&setup_midi_jack;
}

void map_midi_functions_for_alsa()
{
   obtain_midi_events=&obtain_midi_events_alsa;
   setup_midi=&setup_midi_alsa;
}

void map_audio_functions_for_jack()
{
   setup_audio=&setup_audio_jack;
   audio_shutdown=&audio_shutdown_jack;
   send_audio_to_card=&send_audio_to_card_jack;
   audio_loop=&audio_loop_jack;
}

void map_audio_functions_for_alsa()
{
   setup_audio=&setup_audio_alsa;
   audio_shutdown=&audio_shutdown_alsa;
   send_audio_to_card=&send_audio_to_card_alsa;
   audio_loop=&audio_loop_alsa;
}


float generated_samples[4096]; //TODO don't hardcode maximum

extern float audio_sample_rate;

#ifdef RASPBERRY
  #include <wiringPi.h>
//for the B+
//#define LIGHT_PIN 26

//for the 2
#define LIGHT_PIN 13

void setup_gpio()
{
   pinMode(LIGHT_PIN,OUTPUT);
}
#endif


void write_config()
{
  static const char *output_file = "last_config.cfg";
  Config cfg;

  Setting &root = cfg.getRoot();

  Setting &effects = root.add("global_effects", Setting::TypeList);
 
  for(int i=0;i<spv.size();i++)
     spv[i]->write_config(effects);

  Setting &pads = root.add("pads", Setting::TypeList);
  for(int i=0;i<dv.size();i++)
     dv[i]->write_config(pads);

  // Write out the new configuration.
  try
  {
    cfg.writeFile(output_file);
    cerr << "New configuration successfully written to: " << output_file
         << endl;

  }
  catch(const FileIOException &fioex)
  {
    cerr << "I/O error while writing file: " << output_file << endl;
    exit(EXIT_FAILURE);
  }
}

static void signal_handler(int sig)
{
   //jack_client_close(client);
   fprintf(stderr, "signal received, exiting ...\n");
 
   write_config();
	
   exit(0);
}

#ifdef RASPBERRY

// enable flush to zero to eliminate denormals
// http://raspberrypisynthesizer.blogspot.com/2014_08_01_archive.html
static inline void enable_runfast(void)
{
//#ifdef RPI
    uint32_t fpscr;
    __asm__ __volatile__ ("vmrs %0,fpscr" : "=r" (fpscr));
    fpscr |= 0x03000000;
    __asm__ __volatile__ ("vmsr fpscr,%0" : :"ri" (fpscr));
//#endif
}

#endif

bool all_ports_connected=false;
bool pedal_pressed=false;



int generate_samples(jack_nframes_t nframes, void *arg)
{
#ifdef RASPBERRY
  enable_runfast();
#endif

   for(int i = 0; i < nframes; i++)
   {
      // printf("in frame: %d\n",i);
      double t=((double)(total_frames))/audio_sample_rate;

      while(is_there_another_midi_event_for_frame(i))
      {
         unsigned int *midi_event=get_next_midi_event();

         int command=midi_event[0];
         int note = midi_event[1];
         int vol = midi_event[2];

         if (command == 0x90) //note on
         {   

            float vol_f=vol/127.0;
            float vol_sq=vol_f*vol_f;
            
            //#ifdef DEBUG
              // printf("port: %d note: %d vol: %d\n",channel,note,vol);
            //#endif

            if(note==46)
            {
               //printf("user pressed pedal!\n");
               pedal_pressed=true;
               
            }
            else
            {
              int index=note-40;
  
              //if(dv[index]->_is_in_active_queue==false)
              //{
              //   active_samples[highest_sample]=dv[index];
              //   highest_sample++;
              //   dv[index]->_is_in_active_queue=true;
              //}
              dv[index]->note_on(vol_sq);
            }
         }
         if(command == 0x80) //note off
         {
            //printf("got note off!\n");
            
            if(note==46) 
            {
               pedal_pressed=false;
               //printf("user released delay pedal\n");
            }
         }
         /*if(command ==0xB0) //cc message
         {
            unsigned char cc= in_event.buffer[1];
            unsigned char val= in_event.buffer[2];
            float val_f=val/127.0;

            if(channel==0x00)//cc message
            {     
               //printf("cc: %d val: %d\n",cc,val);
            }
         }*/
      }
      
     
      float sum=0;

      


      
      
      for(int e=0;e<6;e++)
      {
         float sample=dv[e]->tick();
         #ifdef RECORDING
            out_sub[e][i]=sample;
         #endif

         #ifdef BASSPORT 
            if(e==0)
            {
               bass_sample=sample;
               sample=0.0f;  //dont add bass sample into main mix
            }
         #endif

         sum+=sample; 
      }

      
      cf->set_feedback(spv[SP_DELAY_FEEDBACK]->get_value());
      float dtime=spv[SP_DELAY_TIME]->get_value();
      //if(pedal_pressed) 
      //  dtime=dtime*2.0;

      cf->set_delay_time(dtime);
      //if(sum!=0.0) printf("sum: %f\n",sum);
      if((pedal_pressed && spv[SP_DPED_REVERSE]->get_bool()==false) || pedal_pressed==false && spv[SP_DPED_REVERSE]->get_bool()) 
      {
         //if(sum!=0.0) printf("sum: %f\n",sum);

         float pass_in=sum*spv[SP_DELAY_INPUT]->get_value();
         //if(pass_in!=0.0) printf("cf input: %f\n",pass_in);

         float cf_result=cf->tick(pass_in);
         //if(cf_result!=0.0) printf("cf: %f\n",cf_result);
      
         sum+=cf_result;
         //printf("doing delay: %f\n",spv[SP_DELAY_INPUT]->get_value());
      }
      else    
         sum+=cf->tick(0); 

#ifdef BASSPORT
      cf2->set_feedback(spv[SP_DELAY_FEEDBACK]->get_value());
      

      cf2->set_delay_time(dtime);

      if((pedal_pressed && spv[SP_DPED_REVERSE]->get_bool()==false) || pedal_pressed==false && spv[SP_DPED_REVERSE]->get_bool()) 
         bass_sample+=cf2->tick(bass_sample*spv[SP_DELAY_INPUT]->get_value()); 
      else    
         bass_sample+=cf2->tick(0); 

      bout[i]=atan(bass_sample*spv[SP_BOOST_POST]->get_value());
#endif
   
   // ee->set_read_speed(ewg->tick();
      //ee->set_read_speed(((ewg->tick()*2.0-1)*0.5)+1.0);
    //  float b=ewg->tick()*2.0-1;
    //  if(b>0)
    //     b*=4.14; 

     // ee->set_read_speed((b));

    //  sum+=ee->tick(sum);

      generated_samples[i]=atan(sum*spv[SP_BOOST_POST]->get_value());
      
      total_frames++;
   }

   send_audio_to_card(generated_samples,nframes,false); //TODO make this stereo if we want to send bassdrum seperately
   
   return 0;      
}

void init_from_config_file(string filename)
{
   cout << "trying to setup kit based on configuration file" << endl;
   Config cfg;
 
   // Read the file. If there is an error, report it and exit.
   try
   {
      cfg.readFile(filename.c_str());
   }
   catch(const FileIOException &fioex)
   {
      std::cerr << "I/O error while reading file." << std::endl;
      exit(EXIT_FAILURE);
   }
   catch(const ParseException &pex)
   {
      std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << std::endl;
      exit(EXIT_FAILURE);
   }

   const Setting& root = cfg.getRoot();

   try
   {
      cout << "parsing for global effects" << endl;
      const Setting &effects = root["global_effects"];
      int count = effects.getLength();

      cout << "number of global effects: " << count << endl;

      for(int i = 0; i < count; ++i)
      {
         Setting &effect = effects[i];
   
         simple_parameter *sp=new simple_parameter(effect);
         spv.push_back(sp);
      }

      const Setting &pads = root["pads"];
      count = pads.getLength();

      cout << "number of pads: " << count << endl;

      for(int i = 0; i < count; ++i)
      {
         single_drum *sd=new single_drum(pads[i], audio_sample_rate); 
         dv.push_back(sd);
      }
   }
   catch(const SettingNotFoundException &nfex)
   {
      // Ignore.
   }
}


void init_sounds(string kit_name)
{
   single_drum *d1,*d2, *d3, *d4, *d5, *d6, *d7, *d8;
/*
   if(kit_name.compare("808")==0)
   {
      printf("choosing 808 kit!\n");
      d1=new single_drum("kick",        "../dsamples/Roland_TR808/BD/BD7575.WAV", audio_sample_rate);
      d2=new single_drum("snare",       "../dsamples/Roland_TR808/SD/SD5075.WAV", audio_sample_rate); 
      d3=new single_drum("snare_rim",   "../dsamples/Roland_TR808/LC/LC00.WAV",   audio_sample_rate);
      d4=new single_drum("cymbal",      "../dsamples/Roland_TR808/CH/CH.WAV",     audio_sample_rate); 
      d5=new single_drum("cymbal_bell ","../dsamples/Roland_TR808/CP/CP.WAV",     audio_sample_rate);
      d6=new single_drum("cymbal_crash","../dsamples/Roland_TR808/CY/CY7575.WAV", audio_sample_rate);  
   }
   else if(kit_name.compare("808b")==0)
   {
      printf("choosing 808 kit!\n");
      d1=new single_drum("kick",        "../dsamples/Roland_TR808/BD/BD7575.WAV", sample_rate);
      d2=new single_drum("snare",       "../dsamples/Roland_TR808/CP/CP.WAV", sample_rate); 
      d3=new single_drum("snare_rim",   "../dsamples/Roland_TR808/CH/CH.WAV",   sample_rate);
      d4=new single_drum("cymbal",      "../dsamples/Roland_TR808/CH/CH.WAV",     sample_rate); 
      d5=new single_drum("cymbal_bell ","../dsamples/Roland_TR808/RS/RS.WAV",     sample_rate);
      d6=new single_drum("cymbal_crash","../dsamples/Roland_TR808/CY/CY7575.WAV", sample_rate);  

      d1->setup_wg(36,2);
      d3->setup_wg(60,0.25);
      d4->setup_wg(67,0.25);
      d5->setup_wg(72,0.25);
   }
   else if(kit_name.compare("amen")==0)
   {
      printf("choosing amen kit!\n");
      d1=new single_drum("kick",     "../dsamples/amen/26885_VEXST_Kick_1.wav",sample_rate);
      d2=new single_drum("snare",    "../dsamples/amen/26900_VEXST_Snare_1.wav",sample_rate);
      d3=new single_drum("snare2",   "../dsamples/amen/26903_VEXST_Snare_4.wav",sample_rate);
      d4=new single_drum("cymbal",   "../dsamples/amen/26889_VEXST_Open_hi_hat.wav",sample_rate);
      d5=new single_drum("cymbal2",  "../dsamples/amen/26884_VEXST_Crash_eq2.wav",sample_rate);
      d6=new single_drum("cymbal3",  "../dsamples/amen/26884_VEXST_Crash_eq2.wav",sample_rate);
   }
   else if(kit_name.compare("linn")==0)
   {
      printf("choosing linn kit!\n");
      d1=new single_drum("","../dsamples/Linn/kick.wav",sample_rate);
      d2=new single_drum("","../dsamples/Linn/sd.wav",sample_rate);
      d3=new single_drum("","../dsamples/Linn/tom.wav",sample_rate);
      d4=new single_drum("","../dsamples/Linn/chh.wav",sample_rate);
      d5=new single_drum("","../dsamples/Linn/crash.wav",sample_rate);
      d6=new single_drum("","../dsamples/Linn/ride.wav",sample_rate);
      d1->setup_wg(45,2);
   }
   else if(kit_name.compare("atari")==0)
   {
      printf("choosing atari kit!\n");
      d1=new single_drum("","../dsamples/atari/atari_2.wav",sample_rate);
      d2=new single_drum("","../dsamples/atari/atari_3.wav",sample_rate);
      d3=new single_drum("","../dsamples/atari/atari_8.wav",sample_rate);
      d4=new single_drum("","../dsamples/atari/atari_8.wav",sample_rate);
      d5=new single_drum("","../dsamples/atari/atari_3.wav",sample_rate);
      d6=new single_drum("","../dsamples/atari/atari_8.wav",sample_rate);
   }     
   else if(kit_name.compare("cr78")==0)
   {
      printf("choosing cr78 kit!\n");
      d1=new single_drum("","../dsamples/Roland_CR78/CR78Kick.aif",sample_rate);
      d2=new single_drum("","../dsamples/Roland_CR78/CR78Snare.aif",sample_rate);
      d3=new single_drum("","../dsamples/Roland_CR78/CR78F1.aif",sample_rate);
      d4=new single_drum("","../dsamples/Roland_CR78/CR78HH.aif",sample_rate);
      d5=new single_drum("","../dsamples/Roland_CR78/CR78Woodblock2.aif",sample_rate);
      d6=new single_drum("","../dsamples/Roland_CR78/CR78HH.aif",sample_rate);
   }     
   else
   {
      printf("unable to find kit!\n");
      printf("requested: %s\n",kit_name.c_str());
      exit(1);
   }

*/

   dv.push_back(d1);
   dv.push_back(d2);
   dv.push_back(d3);
   dv.push_back(d4);
   dv.push_back(d5);
   dv.push_back(d6);
  
   spv.push_back(new simple_parameter("delay_feedback",0.5,0,1));
   spv.push_back(new simple_parameter("delay_time",0.1,0,1));
   spv.push_back(new simple_parameter("delay_input_volume",0,0,1));
   spv.push_back(new simple_parameter("post_boost",1,0,100));
   spv.push_back(new simple_parameter("delay_pedal_reverse",0,0,1));
}

Fl_Double_Window *fltk_window;

void init_gui()
{
   printf("initializing gui...\n"); fflush(stdout);
   fltk_window = new Fl_Double_Window(640, 480);

   Fl_Pack *p1 = new Fl_Pack(0,0,640,480);
   p1->spacing(15);
   p1->type(uchar(Fl_Pack::VERTICAL));

   for(int i=0;i<spv.size();i++)
      spv[i]->init_gui();

   Fl_Pack* p = new Fl_Pack(0,0,1000,1000);
   p->spacing(15);
   p->type(uchar(Fl_Pack::HORIZONTAL));
 
   for(int i=0;i<dv.size();i++)
   {
      dv[i]->init_gui();  
   }

   p->end();

   p1->end();
   
   fltk_window->end();
 //  p->resizable(p);
 //  p1->resizable(p1);
   fltk_window->resizable(fltk_window);
}

int main(int argc, char **argv)
{  
#ifdef RASPBERRY
   wiringPiSetupGpio();
   setup_gpio();
#endif

   srand ( time(NULL) );
   
#ifdef USE_ALSA
   map_midi_functions_for_alsa();
   map_audio_functions_for_alsa();
#else
   map_midi_functions_for_jack();
   map_audio_functions_for_jack();
#endif


 
   printf("initing kits\n");
   if(argc>2) 
   {
      if(strcmp(argv[1],"-k")==0)
      {
         printf("using internal kit: %s\n",argv[2]);
         init_sounds(argv[2]);
         
      }
      else if(strcmp(argv[1],"-f")==0)
      {
         printf("initing from file: %s\n",argv[2]);
         init_from_config_file(argv[2]);
      }
      else
      {
         printf("error, got some weird command line options: %s\n",argv[1]);

      }
   }
   else 
   {
      printf("usage: drums -k [kit name]\n");
      printf("       drums -f [config file name]\n");
      exit(0);
   }

   printf("initing comb filter\n");
   cf=new comb_filter(audio_sample_rate,5.0);
   printf("done\n");

   ee=new echo_effect(audio_sample_rate,10.0); 
   ee->force_length(audio_sample_rate*0.1);

#ifdef BASSPORT
    cf2=new comb_filter(audio_sample_rate,5.0);
#endif

   ewg=new wave_generator(audio_sample_rate);
   ewg->set_speed(2);
   //ewg->set_speed(50);

   init_gui(); //TODO check command line flag to see if we should do gui or not

   signal(SIGQUIT, signal_handler);
   signal(SIGTERM, signal_handler);
   signal(SIGHUP, signal_handler);
   signal(SIGINT, signal_handler);

   setup_audio(0);
   setup_midi("Uno");

   audio_loop();
}

