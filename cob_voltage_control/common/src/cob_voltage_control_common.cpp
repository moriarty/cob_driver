//ROS typedefs
#include "ros/ros.h"
#include <pr2_msgs/PowerBoardState.h>
#include <pr2_msgs/PowerState.h>
#include <cob_relayboard/EmergencyStopState.h>
#include <std_msgs/Float64.h>

#include <libphidgets/phidget21.h>


class cob_voltage_control_config
{
public:
	double min_voltage;
	double max_voltage;
	double max_voltage_res;
	int num_voltage_port;
	int num_em_stop_port;
	int num_scanner_em_port;
	
};

class cob_voltage_control_data
{
// autogenerated: don't touch this class
public:
//configuration data

//input data

//output data
	pr2_msgs::PowerBoardState out_pub_em_stop_state_;
	pr2_msgs::PowerState out_pub_powerstate_;
  cob_relayboard::EmergencyStopState out_pub_relayboard_state;
 std_msgs::Float64 out_pub_voltage;
};

//document how this class has to look
//never change after first generation


class cob_voltage_control_impl
{

public:

	CPhidgetInterfaceKitHandle IFK;
	



	cob_voltage_control_impl()
    {
        //user specific code
    }
    void configure() 
    {
        //user specific code
    	//init and open phidget
    	int numInputs, numOutputs, numSensors, numAnalog;
    	int err;

    	IFK = 0;
		CPhidget_enableLogging(PHIDGET_LOG_VERBOSE, NULL);
		CPhidgetInterfaceKit_create(&IFK);

		//CPhidgetInterfaceKit_set_OnSensorChange_Handler(IFK, IFK_SensorChangeHandler, NULL);
		
		/*CPhidgetInterfaceKit_set_OnInputChange_Handler(IFK, IFK_InputChangeHandler, NULL);
		CPhidgetInterfaceKit_set_OnOutputChange_Handler(IFK, IFK_OutputChangeHandler, NULL);
		CPhidgetInterfaceKit_set_OnSensorChange_Handler(IFK, IFK_SensorChangeHandler, NULL);
		CPhidget_set_OnAttach_Handler((CPhidgetHandle)IFK, IFK_AttachHandler, NULL);
		CPhidget_set_OnDetach_Handler((CPhidgetHandle)IFK, IFK_DetachHandler, NULL);
		CPhidget_set_OnError_Handler((CPhidgetHandle)IFK, IFK_ErrorHandler, NULL);*/

		//opening phidget
		CPhidget_open((CPhidgetHandle)IFK, -1);

		//wait 5 seconds for attachment
		ROS_INFO("waiting for phidgets attachement...");
		if((err = CPhidget_waitForAttachment((CPhidgetHandle)IFK, 10000)) != EPHIDGET_OK )
		{
			const char *errStr;
			CPhidget_getErrorDescription(err, &errStr);
			ROS_ERROR("Error waiting for attachment: (%d): %s",err,errStr);
			return;
		}
		ROS_INFO("... attached");

		int sernum, version;
		const char *deviceptr, *label;
		CPhidget_getDeviceType((CPhidgetHandle)IFK, &deviceptr);
		CPhidget_getSerialNumber((CPhidgetHandle)IFK, &sernum);
		CPhidget_getDeviceVersion((CPhidgetHandle)IFK, &version);
		CPhidget_getDeviceLabel((CPhidgetHandle)IFK, &label);

		ROS_INFO("%s", deviceptr);
		ROS_INFO("Version: %8d SerialNumber: %10d", version, sernum);
		ROS_INFO("Label: %s", label);
		CPhidgetInterfaceKit_getOutputCount((CPhidgetInterfaceKitHandle)IFK, &numOutputs);
		CPhidgetInterfaceKit_getInputCount((CPhidgetInterfaceKitHandle)IFK, &numInputs);
		CPhidgetInterfaceKit_getSensorCount((CPhidgetInterfaceKitHandle)IFK, &numSensors);

		ROS_INFO("Sensors:%d Inputs:%d Outputs:%d", numSensors, numInputs, numOutputs);


    }
    void update(cob_voltage_control_data &data, cob_voltage_control_config config)
    {
        //user specific code

    	//Check for EM Stop
    	int em_stop_State = -1;
    	int scanner_stop_State = -1;
      //CPhidgetInterfaceKit_getInputState ((CPhidgetInterfaceKitHandle)IFK, config.num_em_stop_port, &em_stop_State);
    	CPhidgetInterfaceKit_getInputState ((CPhidgetInterfaceKitHandle)IFK, 1, &em_stop_State);
    	//CPhidgetInterfaceKit_getInputState ((CPhidgetInterfaceKitHandle)IFK, config.num_scanner_em_port, &scanner_stop_State);
    	CPhidgetInterfaceKit_getInputState ((CPhidgetInterfaceKitHandle)IFK, 1, &scanner_stop_State);
    	ROS_DEBUG("DIO: %d %d", em_stop_State, scanner_stop_State);
    	
	data.out_pub_em_stop_state_.header.stamp = ros::Time::now();
	// pr2 power_board_state
	if(em_stop_State == 1)
	{
		data.out_pub_em_stop_state_.run_stop = true;
		//for cob the wireless stop field is misused as laser stop field
		data.out_pub_relayboard_state.emergency_state = 2;
		if(scanner_stop_State == 1)
    {
                	data.out_pub_em_stop_state_.wireless_stop = true;
		    data.out_pub_relayboard_state.emergency_state = 0;
    }
        else
		  {
                	data.out_pub_em_stop_state_.wireless_stop = false;
			data.out_pub_relayboard_state.emergency_state = 1;
		  }

	}	
	else
	{
		data.out_pub_em_stop_state_.run_stop = false;
		data.out_pub_em_stop_state_.wireless_stop = true;
		data.out_pub_relayboard_state.emergency_state = 1;
	}
	
	data.out_pub_relayboard_state.emergency_button_stop = data.out_pub_em_stop_state_.run_stop;
	data.out_pub_relayboard_state.scanner_stop = data.out_pub_em_stop_state_.wireless_stop;
	



    	//Get Battery Voltage
		int voltageState = -1;
		CPhidgetInterfaceKit_getSensorValue((CPhidgetInterfaceKitHandle)IFK, config.num_voltage_port, &voltageState);
		ROS_DEBUG("Sensor: %d", voltageState);

		//Calculation of real voltage 
		//max_voltage = 70V ; max_counts = 999
		double max_counts = 999.0; // see phidgit analog io docu
		double voltage = voltageState * config.max_voltage_res/max_counts;
		data.out_pub_voltage.data = voltage;
		ROS_DEBUG("Current voltage %f", voltage);

		//Linear calculation for percentage
		double percentage =  (voltage - config.min_voltage) * 100/(config.max_voltage - config.min_voltage);
	

		data.out_pub_powerstate_.header.stamp = ros::Time::now();
		data.out_pub_powerstate_.power_consumption = 0.0;
		data.out_pub_powerstate_.time_remaining = ros::Duration(1000);
		data.out_pub_powerstate_.relative_capacity = percentage;

    }
    
    void exit()
    {
    	CPhidget_close((CPhidgetHandle)IFK);
    	CPhidget_delete((CPhidgetHandle)IFK);
    }

};
