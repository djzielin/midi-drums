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

jack_port_t *input_port;
jack_port_t *output_port;
jack_port_t *bass_port;
jack_port_t *output_sub_port[6];
jack_port_t *output_port2;
jack_port_t *midi_output_port;

double sample_rate=0;

float *recording_buffer;

jack_client_t *client;
jack_nframes_t total_frames=0;

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
   jack_client_close(client);
   fprintf(stderr, "signal received, exiting ...\n");
 
   write_config();
	
   exit(0);
}

bool all_ports_connected=false;
bool pedal_pressed=false;

static int process(jack_nframes_t nframes, void *arg)
{
  // printf("generating samples: %d\n",nframes);

   //printf("CPU is: %d\n",sched_getcpu());

   if(all_ports_connected==false) return 0;

   int i,note,vol;
   void* port_buf = jack_port_get_buffer(input_port, nframes);
   jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port, nframes);


   jack_default_audio_sample_t *out_sub[6];
 
   #ifdef RECORDING
    for(int i=0;i<6;i++)
      out_sub[i] = (jack_default_audio_sample_t *) jack_port_get_buffer (output_sub_port[i], nframes);
   #endif

   #ifdef BASSPORT
      jack_default_audio_sample_t *bout = (jack_default_audio_sample_t *) jack_port_get_buffer (bass_port, nframes);
      float bass_sample;
   #endif
   //jack_default_audio_sample_t *out2 = (jack_default_audio_sample_t*)jack_port_get_buffer (output_port2, nframes);

   midi_port_buf = jack_port_get_buffer(midi_output_port, nframes);
   jack_midi_clear_buffer(midi_port_buf);

   jack_midi_event_t in_event;
   jack_nframes_t event_index = 0;

   jack_nframes_t event_count = jack_midi_get_event_count(port_buf);
   if(event_count>0) jack_midi_event_get(&in_event, port_buf, 0); 

   if(first_time==true)
   {
      //do we need to do anything the first time?
      first_time=false;
   }

   for(i = 0; i < nframes; i++)
   {
     // printf("in frame: %d\n",i);
      double t=((double)(total_frames))/sample_rate;

      while((in_event.time == i) && (event_index < event_count))
      {
         unsigned char command=in_event.buffer[0] & 0xf0;
         unsigned char channel=in_event.buffer[0] & 0x0f; 

         if (command == 0x90) //note on
         {   
            note = in_event.buffer[1];
            vol = in_event.buffer[2]; 
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
  
            /*if(dv[index]->_is_in_active_queue==false)
            {
               active_samples[highest_sample]=dv[index];
               highest_sample++;
               dv[index]->_is_in_active_queue=true;
            }*/
            dv[index]->note_on(vol_sq);
            }
         }
         if(command == 0x80) //note off
         {
            //printf("got note off!\n");
            note = in_event.buffer[1]; //used for HH-pedal
            if(note==46) 
            {
               pedal_pressed=false;
               //printf("user released delay pedal\n");
            }
         }
         if(command ==0xB0) //cc message
         {
            unsigned char cc= in_event.buffer[1];
            unsigned char val= in_event.buffer[2];
            float val_f=val/127.0;

            if(channel==0x00)//cc message
            {     
               //printf("cc: %d val: %d\n",cc,val);
            }
         }
         event_index++;
         if(event_index < event_count)
            jack_midi_event_get(&in_event, port_buf, event_index);           
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

      out[i]=atan(sum*spv[SP_BOOST_POST]->get_value());
      
      total_frames++;
   }

   /*int real_index=0;
   for (int q=0;q<highest_sample;q++) //remove any samples that aren't playing
   {      
      single_drum *sd=active_samples[q];
      if(sd->is_playing()==true)
      { 
         active_samples[real_index]=sd;
         real_index++;
      }
      else
        sd->_is_in_active_queue=false;

   }

   int diff=highest_sample-real_index;
   highest_sample=real_index;

   //if(diff!=0) printf("removed samples: %d\n",diff);
    */
   
   return 0;      
}

static int srate(jack_nframes_t nframes, void *arg)
{
   printf("the sample rate is now %d samples per second\n", nframes);
   sample_rate=nframes;

   return 0;
}

static void jack_shutdown(void *arg)
{
   exit(1);
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
         single_drum *sd=new single_drum(pads[i], sample_rate); 
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

   if(kit_name.compare("808")==0)
   {
      printf("choosing 808 kit!\n");
      d1=new single_drum("kick",        "../dsamples/Roland_TR808/BD/BD7575.WAV", sample_rate);
      d2=new single_drum("snare",       "../dsamples/Roland_TR808/SD/SD5075.WAV", sample_rate); 
      d3=new single_drum("snare_rim",   "../dsamples/Roland_TR808/LC/LC00.WAV",   sample_rate);
      d4=new single_drum("cymbal",      "../dsamples/Roland_TR808/CH/CH.WAV",     sample_rate); 
      d5=new single_drum("cymbal_bell ","../dsamples/Roland_TR808/CP/CP.WAV",     sample_rate);
      d6=new single_drum("cymbal_crash","../dsamples/Roland_TR808/CY/CY7575.WAV", sample_rate);  
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


   printf("CPU is: %d\n",sched_getcpu());

   srand ( time(NULL) );

   if ((client = jack_client_open("dave_drums", JackNullOption, NULL)) == 0)
   {
      fprintf(stderr, "jack server not running?\n");
      return 1;
   }
   
   sample_rate=jack_get_sample_rate (client);
 
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
   cf=new comb_filter(sample_rate,5.0);
   printf("done\n");

   ee=new echo_effect(sample_rate,10.0); 
   ee->force_length(sample_rate*0.1);

#ifdef BASSPORT
    cf2=new comb_filter(sample_rate,5.0);
#endif

   ewg=new wave_generator(sample_rate);
   ewg->set_speed(2);
   //ewg->set_speed(50);

   init_gui(); //TODO check command line flag to see if we should do gui or not

   jack_set_process_callback (client, process, 0);
   jack_set_sample_rate_callback (client, srate, 0);
   jack_on_shutdown (client, jack_shutdown, 0);

   input_port = jack_port_register (client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
   output_port = jack_port_register (client, "audio_out_master", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

#ifdef BASSPORT
   bass_port = jack_port_register (client, "audio_out_bass", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
#endif

   #ifdef RECORDING
   for(int i=0;i<6;i++)
   {
      char name[80];
      sprintf(name,"audio_out_%d",i);
      output_sub_port[i]=jack_port_register (client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
   }
   #endif

   midi_output_port = jack_port_register (client, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

   printf("connecting to jack...\n");
   if (jack_activate (client))
   {
      fprintf(stderr, "cannot activate client");
      return 1;
   }
   printf("   success!\n");

   printf("connecting to physical playback ports\n"); fflush(stdout);

   const char **ports;
   ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);
   if (ports == NULL)
   {
      fprintf(stderr, "no physical playback ports\n");
      exit (1);
   }

   
   for(int i=1;i<2;i++) 
   {

      if(ports==NULL) break;
      if(ports[i]==NULL) break;
      printf("plaback ports: %s\n",ports[i]); fflush(stdout);
      printf("   trying audio: %s to %s\n",jack_port_name (output_port), ports[i]); fflush(stdout);
      int ret_val=jack_connect (client, jack_port_name (output_port), ports[i]); 

      if(ret_val==0)
         { printf("      SUCCESS!\n"); fflush(stdout); }
      else
         { printf("      FAIL!\n"); fflush(stdout); }

      /*if(ret_val!=0) 
      { 
         printf("   trying midi: %s to %s\n", jack_port_name (midi_output_port), ports[i]);

         string s1=ports[i];
         string s2="BCR2000";
         if (std::string::npos != s1.find(s2))
         {    
            ret_val=jack_connect (client, jack_port_name (midi_output_port), ports[i]);
 
            if(ret_val==0)
            { printf("      SUCCESS!\n"); fflush(stdout); }
            else
            { printf("      FAIL!\n"); fflush(stdout); }
         }
      }*/
   }

#ifdef BASSPORT
     for(int i=3;i<4;i++) 
     {
      

      if(ports==NULL) break;
      if(ports[i]==NULL) break;
      printf("plaback ports: %s\n",ports[i]); fflush(stdout);
      printf("   trying audio: %s to %s\n",jack_port_name (bass_port), ports[i]); fflush(stdout);
      int ret_val=jack_connect (client, jack_port_name (bass_port), ports[i]); 

      if(ret_val==0)
         { printf("      SUCCESS!\n"); fflush(stdout); }
      else
         { printf("      FAIL!\n"); fflush(stdout); }
    }
#endif

   ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput);
   
   printf("connecting to available MIDI input ports: \n"); fflush(stdout);

   int i;
   for(i=0;;i++) 
   {
      if(ports==NULL) break;
      if(ports[i]==NULL) break;
      printf("midi input ports: %s\n",ports[i]); fflush(stdout);

      string s1=ports[i];
      string s2="Uno";
      if (std::string::npos != s1.find(s2))
      {    
        std::cout << "found the Uno!" << std::endl;
        int ret_val=jack_connect (client, ports[i], jack_port_name (input_port));
      
        if(ret_val==0)
         { printf("      SUCCESS!\n"); fflush(stdout); }
        else
         { printf("      FAIL!\n"); fflush(stdout); }
      }
   }       

   signal(SIGQUIT, signal_handler);
   signal(SIGTERM, signal_handler);
   signal(SIGHUP, signal_handler);
   signal(SIGINT, signal_handler);

   printf("now running!\n");
   all_ports_connected=true;
   //while(1) sleep(1);

   //TODO: only do if if gui active
   //fltk_window->show(argc, argv);
   //fltk_window->iconize();

   //return Fl::run();
   while(1)
      sleep(1);

   jack_client_close(client);
   exit (0);
}

