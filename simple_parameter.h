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

#include <libconfig.h++>
using namespace libconfig;

class simple_parameter
{
public:
   simple_parameter(std::string name, float initial_value, float g_min, float g_max);
   simple_parameter(Setting &effect); //init from config file

   float get_value();
   bool get_bool();

   void init_gui();
   void set_value(float value);
   void write_config(Setting &effects);

private:
   std::string _name;
   float _value;

   float _g_max;  
   float _g_min;

   Fl_Value_Slider* _vs;
};

typedef std::vector<simple_parameter *> SimpleParamVec;
