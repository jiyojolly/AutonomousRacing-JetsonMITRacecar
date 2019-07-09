#include <ros/ros.h>
#include <rc_msgs/RCControlMsg.h>
#include <std_msgs/Float64.h>


/* ROS Package to publish ackerrman msgs according to vesc controllers ROS topic structure.
   Vesc receives data from to seperate topics published as std_msgs::Float64
 */

// 0.5 is the center for servo
//

inline float sgn(float a){
    if(a > 0.0f){
        return 1.0f;
    }

    return -1.0f;
}

inline bool near(float a, float b,float distance){
    return (abs(a - b) < distance);
}



class RC_Driver_vesc{
public:

    RC_Driver_vesc(){
        n = ros::NodeHandle("~");

        if(n.getParam("max_throttle_output",max_throttle_output) && n.getParam("max_steering_output",max_steering_output) 
        && n.getParam("min_throttle_output",min_throttle_output) && n.getParam("min_steering_output",min_steering_output) 
        && n.getParam("middle_throttle_output",middle_throttle_output) && n.getParam("middle_steering_output",middle_steering_output) 
        && n.getParam("max_car_linear_speed",max_car_linear_speed) && n.getParam("max_car_angle",max_car_angle)
        && n.getParam("min_car_linear_speed",min_car_linear_speed) && n.getParam("min_car_angle",min_car_angle)){
            ROS_ERROR("THERE ARE MISSING PARAMETERS PLEASE SUPPLY REQUIRED PARAMETERS. EXITING...");
            return;
        }else{
            ROS_WARN("Parameters Are missing. Default Values may not be suitable. Please provide a yaml file");
        }

        motor_topic = std::string("/drive");
        servo_topic = std::string("/servo_command");


        n.getParam("motor_topic",motor_topic);
        n.getParam("servo_topic",servo_topic);
        n.getParam("rc_topic",rc_command_topic);
        rc_command_sub = n.subscribe(rc_command_topic,10,&RC_Driver_vesc::rc_command_callback_unique_command,this);

        vesc_motor_pub = n.advertise<std_msgs::Float64>(motor_topic,10);
        vesc_servo_pub = n.advertise<std_msgs::Float64>(servo_topic,10);

        //TODO Implement non unique_command
    }


    void rc_command_callback_unique_command(const rc_msgs::RCControlMsg::ConstPtr &msg){
        //Messages are only zero if RC is closed. Discard Values;
        if(msg->steering_cmd == 0 || msg->throttle_cmd == 0){
            ROS_WARN("RC IS CLOSED. OPEN RC TO CONTROL");
            return;
        }

        //MIT Racecar Lightweight 2-D Simulator Updates poses on regular intervals.
        //If same command is being published from RC, then there is no need to publish this since
        //Pose still get updated.

        

        float throttle_percentage = 0.0f;
        float steer_percentage = 0.0f;

        if(msg->steering_cmd > middle_steering_output){
            steer_percentage = (msg->steering_cmd - middle_steering_output)/(max_steering_output - middle_steering_output);
        }else{
            steer_percentage = -(middle_steering_output - msg->steering_cmd)/(middle_steering_output - min_steering_output);
        }

        if(msg->throttle_cmd > middle_throttle_output){
            throttle_percentage = (msg->throttle_cmd - middle_throttle_output)/(max_throttle_output - middle_throttle_output);
        }else{
            throttle_percentage = -(middle_throttle_output - msg->throttle_cmd)/(middle_throttle_output - min_throttle_output);
        }

            

            //ROS_INFO("Calculating Output Throttle Percentage:%f Steering Percentage:%f",throttle_percentage,steer_percentage);
            

        float throttle_interval = max_car_linear_speed - min_car_linear_speed;
        float steering_interval = max_car_angle - min_car_angle;

        float speed_output = sgn(throttle_percentage) * (min_car_linear_speed + (sgn(throttle_percentage) * throttle_interval * throttle_percentage));
            //float steer_output = sgn(steer_percentage) * (min_car_angle + (sgn(steer_percentage) * steering_interval) * steer_percentage);

        float steer_output = (steer_percentage < 0) ? (0.5f * -steer_percentage) : (0.5f * (1 + steer_percentage));

        if(sgn(steer_percentage) * steer_percentage < 0.05){
            steer_output = 0;
        }

        if(sgn(throttle_percentage) * throttle_percentage< 0.05){
            speed_output = 0;
        }

        vesc_motor_pub.publish(speed_output);
        vesc_servo_pub.publish(steer_output);
    }

private:
    ros::Subscriber rc_command_sub;
    ros::Publisher vesc_motor_pub;
    ros::Publisher vesc_servo_pub;
    ros::NodeHandle n;
    //seq_count for drive message stamp
    

    //Store old Command Value
    rc_msgs::RCControlMsg command_value;
    //ackermann_msgs::AckermannDriveStamped drive_output;
    //ackermann_msgs::AckermannDrive drive_output_unstamped;

    //Store Max and Min RC Values and the middle point
    float max_throttle_output;
    float max_steering_output;

    float min_throttle_output;
    float min_steering_output;

    float middle_throttle_output;
    float middle_steering_output;

    //Store Max and Min Values of the Car.
    //Output will be based on percentage value and these values is
    //necessary to output correct values;

    float max_car_linear_speed;
    float max_car_angle;

    float min_car_linear_speed;
    float min_car_angle;

    //std::string drive_topic;
    std::string rc_command_topic;

    std::string motor_topic;
    std::string servo_topic;

    //If the simulation or other systems update their values on message callback
    //And not with regular intervals, then publish a message every time a new command arrives
    //If update has regular intervals, then only output message when there is a unique message
    


};




int main(int argc,char **argv){
    ros::init(argc,argv,"ackermann_to_vesc_cmd");
    RC_Driver_vesc vesc();
    ros::spin();
    
    return 0;
}

