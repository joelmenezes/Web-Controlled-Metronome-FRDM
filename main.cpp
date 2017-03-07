#include "mbed.h"
#include "utils.hpp"
 
#include "EthernetInterface.h"
#include "frdm_client.hpp"
 
#include "metronome.hpp"
 
#define IOT_ENABLED
 
//Serial pc(USBTX, USBRX);
 
namespace active_low
{
    const bool on = false;
    const bool off = true;
}
 
M2MResource* setval_resource;
M2MResource* getmin_resource;
M2MResource* getmax_resource;
M2MResource* resetmm_resource;
 
bool set_on_BPM = false;
bool get_API_BPM = false;
bool mode = 0;             //0 for play, 1 for learn
int bpm = 0;
 
metronome* m_metro = new metronome();
 
 
DigitalOut g_led_red(LED1);
DigitalOut g_led_green(LED2);
DigitalOut g_led_blue(LED3);
 
InterruptIn g_button_mode(SW3);
InterruptIn g_button_tap(SW2);
 
 
void reset(void *argument)
{
    g_led_red = active_low::on;
    wait(1);
    g_led_red = active_low::off;
    (*m_metro).reset_min_max();
    getmin_resource->set_value(0);
    getmax_resource->set_value(0);
}
   
void set_bpm_put(const char *argument)
{
    pc.printf("PUT BPM");
    get_API_BPM = true;
}
 
 
void on_mode()
{
    // Change modes
    mode = !mode;
    if(mode == 0)
    {
		m_metro->stop_timing();
        int incoming_bpm = m_metro->get_bpm();
        if(incoming_bpm != 0)
        {
            bpm = incoming_bpm;
            set_on_BPM = true;
        }
    }
    else
    {
         m_metro->start_timing();
    }
}
 
void on_tap_finish()
{           
    if(mode == 0)
        return;
        g_led_red = active_low::off;
}
 
void on_tap()
{
    if(mode == 0)
        return;
             
    g_led_red = active_low::on;
    m_metro->tap();
}
 
int main()
{
    // Seed the RNG for networking purposes
    unsigned seed = utils::entropy_seed();
    srand(seed);
 
    // LEDs are active LOW - true/1 means off, false/0 means on
    // Use the constants for easier reading
    g_led_red = active_low::off;
    g_led_green = active_low::off;
    g_led_blue = active_low::off;
 
    // Button falling edge is on push (rising is on release)
    g_button_mode.fall(&on_mode);
    g_button_tap.fall(&on_tap);
    g_button_tap.rise(&on_tap_finish);
   
    
 
#ifdef IOT_ENABLED
    g_led_blue = active_low::on;
	EthernetInterface ethernet;
    if (ethernet.connect() != 0)
        return 1;
 
	// Pair with the device connector
    frdm_client client("coap://api.connector.mbed.com:5684", &ethernet);
    if (client.get_state() == frdm_client::state::error)
        return 1;
 
 
 
    M2MObject* m_object = M2MInterfaceFactory::create_object("3318");
    //Create an instance of that object
    M2MObjectInstance* m_inst = m_object->create_object_instance();
             
    //5900
	setval_resource = m_inst->create_dynamic_resource("5900", "BPM Read",
		M2MResourceInstance::STRING, false);
    setval_resource->set_operation(M2MBase::GET_PUT_ALLOWED);
    setval_resource->set_value((const uint8_t*)"0", 1);
    setval_resource->set_value_updated_function(value_updated_callback(&set_bpm_put));
    
	//5601
	getmin_resource = m_inst->create_dynamic_resource("5601", "Min Read",
        M2MResourceInstance::INTEGER, false);
    getmin_resource->set_operation(M2MBase::GET_ALLOWED);
    getmin_resource->set_value((*m_metro).get_min());
             
    //5602
    getmax_resource = m_inst->create_dynamic_resource("5602", "Max Read",
        M2MResourceInstance::INTEGER, false);
    getmax_resource->set_operation(M2MBase::GET_ALLOWED);
    getmax_resource->set_value((*m_metro).get_max());
             
    //5605
    resetmm_resource = m_inst->create_dynamic_resource("5605", "Reset",
        M2MResourceInstance::STRING, false);
    resetmm_resource->set_operation(M2MBase::POST_ALLOWED);
    resetmm_resource->set_execute_function(execute_callback(&reset));
       
    
    
 
    // The REST endpoints for this device
    // Add your own M2MObjects to this list with push_back before client.connect()
    M2MObjectList objects;
 
    M2MDevice* device = frdm_client::make_device();
    objects.push_back(device);
    objects.push_back(m_object);
 
    // Publish the RESTful endpoints
    client.connect(objects);
 
    // Connect complete; turn off blue LED forever
    g_led_blue = active_low::off;
#endif
 
    while (true)  //Runs continuously.
    {
#ifdef IOT_ENABLED
		if (client.get_state() == frdm_client::state::error)
            break;
#endif
 
    if(get_API_BPM == true)
    {
        uint8_t* val = NULL;
		uint32_t valSize;
        setval_resource->get_value(val, valSize);
        bpm = atoi((char*)val);
                               
        get_API_BPM = false;
                                        
        if(bpm < (*m_metro).min_bpm)
		{
			(*m_metro).set_min(bpm);
			getmin_resource->set_value(bpm);
        }
        
		if(bpm > (*m_metro).max_bpm)
        {
            (*m_metro).set_max(bpm);
            getmax_resource->set_value(bpm);
        }
    }
                         
    if(set_on_BPM == true)
    {
        set_on_BPM = false;
        setval_resource->set_value(bpm);
                         
        if(bpm < (*m_metro).min_bpm)
        {
			(*m_metro).set_min(bpm);
            getmin_resource->set_value(bpm);
        }
        if(bpm > (*m_metro).max_bpm)
        {
            (*m_metro).set_max(bpm);
            getmax_resource->set_value(bpm);
        }
    }
                          
    if(mode == 1)   //Learn mode
    {
    g_led_green = active_low::off;
	continue;         
	}

	else
	{
		if(bpm == 0)
        {
            g_led_green = active_low::off;
        }
        else if (bpm > 0)
			{
            g_led_green = active_low::off;
            wait(((float)60/bpm) - 0.2f);
            g_led_green = active_low::on;
            wait(0.2f);
			}
        }
    }
 
#ifdef IOT_ENABLED
    client.disconnect();
#endif
 
    return 1;
}
 
