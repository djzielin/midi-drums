#ifndef COMMON_DJZ
#define COMMON_DJZ

#include<string>
#include<sstream>

//from http://notfaq.wordpress.com/2006/08/30/c-convert-int-to-string/
template <class T> inline std::string to_string (const T& t)
{
   std::stringstream ss;
   ss << t;
   return ss.str();
}

void queue_up_single_cc(int controller, int value);
void send_out_any_waiting_cc_messages(void  *midi_port_buf);

#endif
