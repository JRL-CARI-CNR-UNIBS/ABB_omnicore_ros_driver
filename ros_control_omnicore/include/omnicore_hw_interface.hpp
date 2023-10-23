#ifndef GENERIC_ROS_CONTROL_GENERIC_HW_INTERFACE_H
#define GENERIC_ROS_CONTROL_GENERIC_HW_INTERFACE_H

// ROS
#include <ros/ros.h>
#include <urdf/model.h>
#include <std_srvs/Trigger.h>
#include <std_msgs/Byte.h>

// ROS Controls
#include <hardware_interface/robot_hw.h>
#include <hardware_interface/joint_state_interface.h>
#include <hardware_interface/joint_command_interface.h>
#include <controller_manager/controller_manager.h>
#include <joint_limits_interface/joint_limits.h>
#include <joint_limits_interface/joint_limits_interface.h>
#include <joint_limits_interface/joint_limits_rosparam.h>
#include <joint_limits_interface/joint_limits_urdf.h>
#include <limits>
#include <trajectory_msgs/JointTrajectory.h>

// ABB libraries
#include <abb_librws/rws_state_machine_interface.h>
#include <abb_librws/rws_interface.h>
#include <abb_libegm/egm_controller_interface.h>

// Custom ROS messages
#include <omnicore_interface/OmnicoreState.h>

// Custom ROS services
#include <omnicore_interface/moveJ_rapid.h>
#include <omnicore_interface/moveL_rapid.h>

// Boost
#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>

#define __EXTERNAL_AXIS__ 2 // External robot axis. Used only if Yumi Single Arm is loaded

namespace ros_control_omnicore
{

	class OmnicoreHWInterface : public hardware_interface::RobotHW
	{
	public:
		OmnicoreHWInterface(const ros::NodeHandle &nh);
		virtual ~OmnicoreHWInterface() {}

		/** \brief Initialize the hardware interface */
		void init();

		/** \brief Read the state from the robot hardware. */
		void read(ros::Duration &elapsed_time);

		/** \brief Write the command to the robot hardware. */
		void write(ros::Duration &elapsed_time);

		/** \brief Shutdown gently the robot hardware. */
		void shutdown();

		/** \brief Set all members to default values */
		virtual void reset();

		// static void ShutDownHandler(int signal);

		/**
		 * \brief Check (in non-realtime) if given controllers could be started and stopped from the
		 * current state of the RobotHW
		 * with regard to necessary hardware interface switches. Start and stop list are disjoint.
		 * This is just a check, the actual switch is done in doSwitch()
		 */
		bool canSwitch(const std::list<hardware_interface::ControllerInfo> &start_list,
							const std::list<hardware_interface::ControllerInfo> &stop_list);

		/**
		 * \brief Perform (in non-realtime) all necessary hardware interface switches in order to start
		 * and stop the given controllers.
		 * Start and stop list are disjoint. The feasability was checked in canSwitch() beforehand.
		 */
		void doSwitch(const std::list<hardware_interface::ControllerInfo> &start_list,
						  const std::list<hardware_interface::ControllerInfo> &stop_list);

		/**
		 * \brief Register the limits of the joint specified by joint_id and joint_handle. The limits
		 * are retrieved from the urdf_model.
		 *
		 * \return the joint's type, lower position limit, upper position limit, and effort limit.
		 */
		void registerJointLimits(const hardware_interface::JointHandle &joint_handle_position,
										 const hardware_interface::JointHandle &joint_handle_velocity,
										 std::size_t joint_id);

		/** \breif Enforce limits for all values before writing */
		void enforceLimits(ros::Duration &period);

		/** \brief Helper for debugging a joint's state */
		void printState();
		std::string printStateHelper();

		/** \brief Helper for debugging a joint's command */
		std::string printCommandHelper();

		/** \brief This functions set through RWS the EGM params specified in the configuration file .yaml */

		// EGM functions
		bool SetEGMParameters();
		bool EGMStartJointSignal();
		bool EGMStopJointSignal();
		bool EGMStartStreamingSignal();
		bool EGMStopStreamingSignal();
		void WaitForEgmConnection();

		// FreeDrive functions
		bool FreeDriveStartSignal();
		bool FreeDriveStopSignal();

		// Funcitons to move from one command to the other
		bool SetControlToEgm(std_srvs::TriggerRequest& req, std_srvs::TriggerResponse& res);
		bool SetControlToFreeDrive(std_srvs::TriggerRequest& req, std_srvs::TriggerResponse& res);
		bool MoveJRapid(omnicore_interface::moveJ_rapidRequest& req, omnicore_interface::moveJ_rapidResponse& res);
		bool MoveLRapid(omnicore_interface::moveL_rapidRequest& req, omnicore_interface::moveL_rapidResponse& res);

		// Generic funtions
		void 		   LoadToolData();
		std_msgs::Byte ReadDigitalInputs();
		void 		   PublishOmnicoreState();

	private:
		std::string robot_name;
		std::string task_name;
		ros::NodeHandle nh;
		// static OmnicoreHWInterface* hardware_interface_instance;

		// --------------------------------------------------------------- //
		// ------------------ Variables for ros_control ------------------ //
		// --------------------------------------------------------------- //

		// Hardware interfaces - Resources
		hardware_interface::JointStateInterface joint_state_interface;
		hardware_interface::PositionJointInterface position_joint_interface;
		hardware_interface::VelocityJointInterface velocity_joint_interface;

		// Joint limits interfaces - Saturation
		joint_limits_interface::PositionJointSaturationInterface pos_jnt_sat_interface;
		joint_limits_interface::VelocityJointSaturationInterface vel_jnt_sat_interface;

		// Robot configuration
		std::vector<std::string> joint_names;
		std::size_t num_joints;
		urdf::Model *urdf_model;

		// States
		std::vector<double> joint_position;
		std::vector<double> joint_velocity;
		std::vector<double> joint_effort;

		// Commands
		std::vector<double> joint_position_command;
		std::vector<double> joint_velocity_command;
		// std::vector<double> joint_effort_command; -> Not supported by Omnicore :(

		// Copy of limits, in case we need them later in our control stack
		std::vector<double> joint_position_lower_limits;
		std::vector<double> joint_position_upper_limits;
		std::vector<double> joint_velocity_limits;
		std::vector<double> joint_effort_limits;


		/** \brief Get the URDF XML from the parameter server */
		virtual void loadURDF(const ros::NodeHandle &nh, std::string param_name);

		// --------------------------------------------------------------- //
		// ---------------------- Variables for ROS ---------------------- //
		// --------------------------------------------------------------- //
		
		// Publishers
		ros::Timer timerOmnicoreState;
		ros::Publisher omnicore_state_publisher;

		// Ros services servers
		ros::ServiceServer server_set_egm_state;
		ros::ServiceServer server_set_free_drive_state;
		ros::ServiceServer server_moveJ_rapid;
		ros::ServiceServer server_moveL_rapid;

		// --------------------------------------------------------------- //
		// -------------- Variables for connecting to Robot -------------- //
		// --------------------------------------------------------------- //

		// Generic data
		std::string ip_robot;
		int port_robot_rws;
		int port_robot_egm;
		std::string task_robot;

		// StateMachine state
		abb::rws::RWSStateMachineInterface::States state_machine_state = abb::rws::RWSStateMachineInterface::STATE_IDLE;
		abb::rws::RWSStateMachineInterface::EGMActions egm_action = abb::rws::RWSStateMachineInterface::EGM_ACTION_UNKNOWN;

		// Boost components for managing asynchronous UDP socket(s).
		boost::thread_group thread_group_;
		boost::asio::io_service io_service_;

		// EGM parameters
		int pos_corr_gain;
		int max_speed_deviation;

		// Interfaces
		abb::rws::RWSStateMachineInterface *p_rws_interface;
		abb::egm::EGMControllerInterface *p_egm_interface;

		// Data from and to the robot
		abb::egm::wrapper::Input data_from_egm;
		abb::egm::wrapper::Output data_to_egm;
	};

}

#endif // GENERIC_ROS_CONTROL_GENERIC_HW_INTERFACE_H
