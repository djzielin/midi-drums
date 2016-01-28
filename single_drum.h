#include "mono_sample.h"
#include "simple_envelope.h"
#include <string>
#include <OnePole.h>
//#include <tinyxml.h>
//#include "effect_controller.h"
#include "wave_generator.h"
#include "single_parameter.h"

using namespace std;

#include <libconfig.h++> //was h++
using namespace libconfig;

class single_drum 
{
public:
   // single_drum(TiXmlHandle base, int sample_rate); 
    single_drum(Setting &pad, int sample_rate);
    single_drum(string name, string filename, int sample_rate);
  
    ~single_drum();

    //void store_xml(TiXmlElement * root);

    void note_off();
    void note_on(float velocity);
    float tick(float gspeed);
    bool is_playing() { return _is_playing; }

    void set_speed(float s);
    void set_envelope(float d);
    void set_pitch_envelope(float d);
    void set_filter(float val) { _filter->setPole(val); }
    void set_release_envelope(float d);
    void receive_cc_message(int cc, float val); 
    void send_all_cc(void  *midi_port_buf);
    void setup_wg(int midi_note, float wg_env_time);
    void init_gui();
    void set_gate(int val);
    void set_boost(int val);

    bool _is_in_active_queue;

    void write_config(Setting &pads);

private:
   void init(string name, string filename, int sample_rate, int interp_type);
   void init_parameters();

   string _name;
   int _index;
   int _sample_rate;
   mono_sample *_msample;   
  
   string _sample_filename;
   int _interpolation_type; 
   
   wave_position _position;
   bool _is_playing;

   simple_envelope *_v_env;
   simple_envelope *_p_env;
   stk::OnePole *_filter;   

   ParamVec _pv;

   wave_generator *_wg;
   bool _wg_active;
   simple_envelope *_wg_env;
   //float _wg_env_time;
   float _wg_vol;

   float _initial_velocity;
   float _initial_volume;
   float _initial_pitch;
   float _initial_filter;
   float _initial_boost;
   float _initial_delay_send;
   float _initial_rev_send;
   int current_sample;
   int gate_value;

};

typedef vector<single_drum *> DrumVec;
