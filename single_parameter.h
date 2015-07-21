#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Pack.H>

#include<string>
#include<vector>
using namespace std;

#include <libconfig.h++> //was h++
using namespace libconfig;

class single_parameter
{
public:
   single_parameter(Setting &effect); //init from config file
   single_parameter(std::string name, float initial_min, float initial_max, float g_min, float g_max);
   float calc_value(float velocity);
   void init_gui();
   void set_max(float value);
   void set_min(float value);
   bool is_active() { return _active; }

   void write_config(Setting &effects);

private:
   void update_helper_vals();   

   std::string _name;
   bool _active;
   float _v_min;
   float _v_max;
   float _v_delta;

   float _g_min;
   float _g_max;  

   Fl_Value_Slider* _vs;
   Fl_Value_Slider* _vs2;
};

typedef std::vector<single_parameter *> ParamVec;
