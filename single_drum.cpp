#include<math.h>
#include "single_drum.h"
#include "mono_sample.h"
#include <stdio.h>
#include <Delay.h>
#include <stdlib.h>
#include <sstream>
#include "common.h"

#include <FL/Fl.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Box.H>

#define SD_INITIAL_VOLUME 0
#define SD_INITIAL_PITCH  1
#define SD_VOLUME_ENVELOPE 2
#define SD_PITCH_ENVELOPE 3
#define SD_LP_FILTER 4
#define SD_BOOST 5
#define SD_MIDI_NOTE 6
#define SD_MIDI_VOL 7
#define SD_MIDI_ENV 8

single_drum::~single_drum()
{   
   cout << "destructor called on single_drum" << endl;

   delete _msample;
}

void single_drum::write_config(Setting &pads)
{
   Setting &pad = pads.add(Setting::TypeGroup);
   pad.add("name", Setting::TypeString) = _name;
   pad.add("sample_filename",Setting::TypeString) = _sample_filename;
   pad.add("interpolation_type",Setting::TypeInt) = _interpolation_type;
  
   Setting &parameters = pad.add("parameters",Setting::TypeList);
   for(int i=0;i<_pv.size();i++)
   {
      cout << "in " << _name << " writing: " << i << endl;
      _pv[i]->write_config(parameters);
   }
}

float calc_hz_from_midinote(int midi_note)
{
   return 440.0*pow(2.0,(midi_note-69.0)/12.0);
}

void single_drum::set_gate(int val)
{
   if(val==127)
      gate_value=-1;
   else
   {
      gate_value=100*val; 
      printf("setting gate time to: %d samples\n",gate_value);
   }
}

single_drum::single_drum(Setting &pad, int sample_rate)
{
   string sval;
   int  ival;

   if(pad.lookupValue("name", sval))
   {
      _name=sval;
      cout << "found pad name: " << _name << endl;
   }
   
   if(pad.lookupValue("sample_filename", sval))
   {
      _sample_filename=sval;
      cout << "found sample file name: " << _sample_filename << endl;
   }
   if(pad.lookupValue("interpolation_type", ival))
   {
      _interpolation_type=ival;
      cout << "found interpolation_type: " << _interpolation_type << endl;
   }  

   init(_name,_sample_filename,sample_rate, _interpolation_type);

   const Setting &parameters = pad["parameters"];
   int count = parameters.getLength();

   for(int i=0;i<count;i++)
   {
      single_parameter *sp=new single_parameter(parameters[i]);
      _pv.push_back(sp);
   }
   gate_value=-1;
   boost_offset=0.0f;
   bonus_vol=1.0f;
}


single_drum::single_drum(string name, string filename, int sample_rate)
{
   init(name,filename,sample_rate,4); //4 is best quality
   init_parameters();
}

void single_drum::init_gui()
{
   Fl_Pack* p = new Fl_Pack(0, 0, 300, 300);
   p->spacing(20);
   p->type(uchar(Fl_Pack::VERTICAL));

   char *n=new char[100];
   sprintf(n,"%s",_name.c_str());
   Fl_Box *box = new Fl_Box(0,0,50,10,n);

   for(int i=0;i<_pv.size();i++)
      _pv[i]->init_gui();
  
   p->end();
}

void single_drum::init(string name, string filename, int sample_rate, int interp_type)
{
   printf("initing single drum: %s\n",name.c_str());

   _name=name;
   _sample_filename=filename;
   _sample_rate=sample_rate;
   _msample=new mono_sample(filename.c_str(),_sample_rate);
  
   _filter=new stk::OnePole(0.0);

   _is_playing=false;
   _interpolation_type=interp_type;

   _wg=new wave_generator(sample_rate);
   //_wg_active=false;
   _wg_env=new simple_envelope();
   _wg_vol=1.0;

   _p_env=new simple_envelope();
   _v_env=new simple_envelope();
   _is_in_active_queue=false;
   retrig_count=0;
}

void single_drum::init_parameters()
{
   _pv.push_back(new single_parameter("initial_volume",0,1,0,2));
   _pv.push_back(new single_parameter("initial_pitch",0,0,0,2));
   _pv.push_back(new single_parameter("volume_envelope",0,1,0,2));
   _pv.push_back(new single_parameter("pitch_envelope",0,0,0,2));
   _pv.push_back(new single_parameter("lp_filter",0.5,1,0,1));
   _pv.push_back(new single_parameter("boost",0,0,0,1000));
   _pv.push_back(new single_parameter("midi_note",0,0,0,100));
   _pv.push_back(new single_parameter("midi_vol",0,0,0,1));
   _pv.push_back(new single_parameter("midi_env",0,1,0,2));
   gate_value=-1;

}


void single_drum::setup_wg(int midi_note, float wg_env_time)
{
   _wg->set_speed(calc_hz_from_midinote(midi_note));
   _wg_active=true;
   //_wg_env_time=wg_env_time;
}

void single_drum::send_all_cc(void  *midi_port_buf)
{
   //printf("_ev size: %d\n",_ev.size());

   //for(int i=0;i<_ev.size();i++)
   //   _ev[i]->send_all_cc(midi_port_buf,i);
}

void single_drum::receive_cc_message(int cc, float val)
{
   //printf("  in single drum, received cc message\n");

  /* int effect=0;
   int corrected_cc=0;

   if(cc>0 && cc<9)
   {
      effect=ceil((float)cc/2.0)-1;
      corrected_cc=cc-effect*2;
   }
   else if(cc>80 && cc<89)
   {
      effect=ceil((float)(cc-80)/2.0)-1;
      corrected_cc=cc-effect*2;
   }
   else if(cc>88 && cc<97)
   {
      effect=ceil((float)(cc-88)/2.0)-1;
      corrected_cc=cc-effect*2;
   }
   else if(cc>96 && cc<105)
   {
      effect=ceil((float)(cc-96)/2.0)-1;
      corrected_cc=cc-effect*2;
   }

   //printf("effect: %d cc: %d\n",effect,corrected_cc);

   if(effect>=3) return;

   _ev[effect]->receive_cc(corrected_cc,val);
*/
}

void single_drum::set_boost(int val)
{
   _pv[SD_BOOST]->set_max((float)val/127.0f*_pv[SD_BOOST]->get_gmax());
}

void single_drum::set_boost_offset(int val)
{
   boost_offset=(float)val/127.0f;
}

void single_drum::note_on()
{
   note_on(_initial_velocity); //just use most recent velocity
}

void single_drum::note_on(float velocity)
{ 
   _position.reset();
   _is_playing=true;

   _initial_velocity=velocity;
   _initial_volume=1.0;
   _initial_pitch=1.0;
   _initial_filter=0.0;
   _initial_boost=1.0;
   _initial_delay_send=0.0;
   _initial_rev_send=0.0;
   current_sample=0;

   if(_pv[SD_INITIAL_VOLUME]->is_active())
      _initial_volume=_pv[SD_INITIAL_VOLUME]->calc_value(_initial_velocity);
   if(_pv[SD_INITIAL_PITCH]->is_active())
      _initial_pitch=_pv[SD_INITIAL_PITCH]->calc_value(_initial_velocity);
   if(_pv[SD_INITIAL_PITCH]->is_active())
      _initial_filter=_pv[SD_LP_FILTER]->calc_value(_initial_velocity);   
   if(_pv[SD_BOOST]->is_active())
      _initial_boost=_pv[SD_BOOST]->calc_value(_initial_velocity);   
   //if(_pv[SD_DELAY_SEND]->is_active())
   //   _initial_delay_send=_pv[SD_DELAY_SEND]->calc_value(_initial_velocity);   
   //if(_pv[SD_REV_SEND]->is_active())
   //   _initial_rev_send=_pv[SD_REV_SEND]->calc_value(_initial_velocity);   
   
   if(_pv[SD_LP_FILTER]->is_active())
   {
      _initial_filter=1.0f-_pv[SD_LP_FILTER]->calc_value(_initial_velocity);   
      _filter->setPole(_initial_filter);
   }

   if(_pv[SD_VOLUME_ENVELOPE]->is_active())
   {
      _v_env->reset();
      _v_env->set_envelope(_pv[SD_VOLUME_ENVELOPE]->calc_value(_initial_velocity)*_sample_rate);

   }
   if(_pv[SD_PITCH_ENVELOPE]->is_active())
   {
      _p_env->set_envelope(_pv[SD_PITCH_ENVELOPE]->calc_value(_initial_velocity)*_sample_rate);
      _p_env->reset();
   }
   if(_pv[SD_MIDI_NOTE]->is_active())
   {
      _wg->set_speed(calc_hz_from_midinote(_pv[SD_MIDI_NOTE]->calc_value(_initial_velocity)));
      
   }

   if(_pv[SD_MIDI_ENV]->is_active())
   {
      _wg_env->set_envelope(_pv[SD_MIDI_ENV]->calc_value(_initial_velocity)*_sample_rate);
      _wg_env->reset();
      _wg->reset();
      //printf("wg env time: %f\n",_wg_env->get_decay_time());
   }
   if(_pv[SD_MIDI_VOL]->is_active())
   {
      _wg_vol=_pv[SD_MIDI_VOL]->calc_value(_initial_velocity);
      //printf("midi vol: %f\n",_wg_vol);
   }
   

   /*printf("note on with velocity: %f\n",_initial_velocity);
   printf("   volume: %f\n",_initial_volume);
   if(_pv[SD_INITIAL_PITCH]->is_active())
      printf("   pitch: %f\n",_initial_pitch);
   if(_pv[SD_LP_FILTER]->is_active()) 
      printf("   filter: %f\n",_initial_filter);
   //printf("   boost: %f\n",_initial_boost);
   printf("   delay send: %f\n",_initial_delay_send);
   if(_pv[SD_VOLUME_ENVELOPE]->is_active())
      printf("   volume envelope active\n");
   if(_pv[SD_PITCH_ENVELOPE]->is_active())
      printf("   pitch envelope active\n");
*/
   //printf("   rev send: %f\n",_initial_rev_send);
   fflush(stdout);
}

void single_drum::note_off()
{
   _is_playing=false;
    //queue_up_single_cc(73+_index,0);
}

float single_drum::tick(float gspeed)
{
   if(_is_playing==false) return 0.0;
  
   /*if(retrig_count>0 && current_sample>3000)
   {
      
       if(retrig_count>5) //number of -retrigs
       {
          retrig_count=0;
          _is_playing=false; //hmmm, do we want tail?
          return 0.0;
       }
       else
       {
          retrig_count++;
          note_on();
         //_initial_pitch=_initial_pitch-(retrig_count-1.0f)*0.2f;
         // if(_initial_pitch<0.0) _initial_pitch=0.0;
       } 
   }*/

   if(gate_value!=-1 && current_sample>gate_value) { printf("activatating gate\n"); _is_playing=false; return 0.0;}

   float sample=_msample->get_sample(_position,_interpolation_type);
   if(_pv[SD_BOOST]->is_active())
      sample=atan(sample*_initial_boost+boost_offset);
   float volume=_initial_volume;

   if(_pv[SD_VOLUME_ENVELOPE]->is_active())
     volume=volume*_v_env->tick();
   if(volume<0.01) 
      printf("less than 0\n");
   sample=sample*volume*bonus_vol;
  
   //if(_pv[SD_LP_FILTER]->is_active())
   //   sample=_filter->tick(sample);
 
   float speed=_initial_pitch;
   if(_pv[SD_PITCH_ENVELOPE]->is_active())
      speed=speed*_p_env->tick();

   _position.increment(speed*gspeed);
  
   if(_msample->is_past_end(_position) || volume<0.01) // || _position.whole_part>2000)
   {
      if(_pv[SD_MIDI_NOTE]->is_active())
      {
         if(_wg_env->get_state()==1)
            _is_playing=false;
      }
      else
      {
          
         _is_playing=false;
         printf("past end position. stopping\n");
      }
   }
   if(_pv[SD_MIDI_NOTE]->is_active())
   {
      float wg_e=_wg_env->tick();
      sample+=(_wg->tick()*2.0-1.0)*_wg_vol*wg_e;
   }

  //printf("generated sample: %.02f\n",sample); fflush(stdout);
  //rev_send+=sample*_initial_rev_send;
  //delay_send+=sample*_initial_delay_send;
  current_sample++;

  return sample;
}
