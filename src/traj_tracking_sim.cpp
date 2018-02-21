#include "ros/ros.h"
#include <tf/transform_broadcaster.h>
#include <tf/transform_datatypes.h>
#include <sensor_msgs/JointState.h>
#include <visualization_msgs/Marker.h>
#include <nav_msgs/Odometry.h>

#include <fstream>

#include "acado_common.h"
#include "acado_auxiliary_functions.h"

/* Some convenient definitions. */
#define NX          ACADO_NX  /* Number of differential state variables.  */
#define NXA         ACADO_NXA /* Number of algebraic variables. */
#define NU          ACADO_NU  /* Number of control inputs. */
#define NOD         ACADO_NOD  /* Number of online data values. */

#define NY          ACADO_NY  /* Number of measurements/references on nodes 0..N - 1. */
#define NYN         ACADO_NYN /* Number of measurements/references on node N. */

#define N           ACADO_N   /* Number of intervals in the horizon. */
#define NUM_OBS   2
#define NUM_STEPS 10

/* Global variables used by the solver. */
ACADOvariables acadoVariables;
ACADOworkspace acadoWorkspace;

/* A template for testing of the solver. */
int main(int argc, char** argv) {
  ros::init(argc, argv, "quad_sim");
  ros::NodeHandle nh;
  int    i, iter;
  acado_timer t;

  std::string states_file; nh.getParam("states_file", states_file);
  std::string controls_file; nh.getParam("controls_file", controls_file);

  std::string reference_states; nh.getParam("reference_states", reference_states);
  std::string reference_controls; nh.getParam("reference_controls", reference_controls);

  std::ofstream states(states_file);
  std::ofstream controls(controls_file);

  std::ifstream xref(reference_states);
  std::ifstream uref(reference_controls);

  /* Initialize the solver. */
  acado_initializeSolver();

  for (i = 0; i < N ; ++i) {
    std::string line;
    std::getline(xref, line);
    std::istringstream iss(line);
    for(int x=0; x < NX; x++) 
      iss >> acadoVariables.y[i*NY + x];
    std::getline(uref, line);
    std::istringstream iss2(line);
    for(int u=0; u < NU; u++)
      iss2 >> acadoVariables.y[i*NY + NX + u];
  }

  for (i = 0; i < N ; ++i) {
    std::string line;
    std::istringstream iss(line);
    std::getline(uref, line);
    for(int u=0; u < NU; u++)
      iss >> acadoVariables.u[i*NU + u];
  }

  for (i = 0; i < N+1 ; ++i) {
    std::string line;
    std::istringstream iss(line);
    std::getline(xref, line);
    for(int x=0; x < NX; x++)
      iss >> acadoVariables.x[i*NX + x];
  }

  /* MPC: initialize the current state feedback. */
#if ACADO_INITIAL_STATE_FIXED
  for (i = 0; i < NX; ++i) acadoVariables.x0[i] = 0.0;
  acadoVariables.x0[14] = -1.5;
#endif
  ROS_INFO("added init");

  /* Prepare first step */
  acado_preparationStep();

  // /* Get the time before start of the loop. */
  acado_tic(&t);

   /* The "real-time iterations" loop. */
  for(iter = 0; iter < NUM_STEPS; ++iter) {
    acado_feedbackStep();
    acado_preparationStep();
  }
  /* Read the elapsed time. */
  real_t te = acado_toc(&t);

  printf("\n\n time:   %.3g seconds\n\n", te);

  // ros::Duration(0.5).sleep();
  // tf::TransformBroadcaster br;
  // ros::Publisher jointPub = nh.advertise<sensor_msgs::JointState>("/joint_states", 1);
  // ros::Publisher goalPub = nh.advertise<visualization_msgs::Marker>("/goal_marker", 1);
  // ros::Publisher obsPub = nh.advertise<visualization_msgs::Marker>("/obs_marker", 1);
  // ros::Publisher odomPub = nh.advertise<nav_msgs::Odometry>("/odometry", 1);

  // visualization_msgs::Marker marker;
  // marker.header.frame_id = "world";
  // marker.id = 0;
  // marker.type = visualization_msgs::Marker::CUBE;
  // marker.action = visualization_msgs::Marker::ADD;
  // marker.pose.position.x = gx + 0.42 + 0.175;
  // marker.pose.position.y = gy;
  // marker.pose.position.z = gz - 0.0875;
  // marker.pose.orientation.x = 0.0;
  // marker.pose.orientation.y = 0.0;
  // marker.pose.orientation.z = 0.0;
  // marker.pose.orientation.w = 1.0;
  // marker.scale.x = 0.1;
  // marker.scale.y = 0.1;
  // marker.scale.z = 0.1;
  // marker.color.a = 1.0; 
  // marker.color.r = 0.0;
  // marker.color.g = 1.0;
  // marker.color.b = 0.0;

  // // double obs[] = {1.5, 2.5};
  // // visualization_msgs::Marker obs_marker[NUM_OBS];

  // // for(tnt o = 0; o < NUM_OBS; o++) {
  // //   obs_marker[o].header.frame_id = "world";
  // //   obs_marker[o].id = o;
  // //   obs_marker[o].type = visualization_msgs::Marker::SPHERE;
  // //   obs_marker[o].action = visualization_msgs::Marker::ADD;
  // //   obs_marker[o].pose.position.x = obs[o];
  // //   obs_marker[o].pose.position.y = obs[o] + 0.2;
  // //   obs_marker[o].pose.position.z = obs[o] + 0.2;
  // //   obs_marker[o].pose.orientation.x = 0.0;
  // //   obs_marker[o].pose.orientation.y = 0.0;
  // //   obs_marker[o].pose.orientation.z = 0.0;
  // //   obs_marker[o].pose.orientation.w = 1.0;
  // //   obs_marker[o].scale.x = 0.3;
  // //   obs_marker[o].scale.y = 0.3;
  // //   obs_marker[o].scale.z = 0.3;
  // //   obs_marker[o].color.a = 1.0; 
  // //   obs_marker[o].color.r = 1.0;
  // //   obs_marker[o].color.g = 0.0;
  // //   obs_marker[o].color.b = 0.0;
  // // }

  for(int tt=0; tt < N+1; tt++) {
  //   tf::Transform transform = tf::Transform(
  //    tf::createQuaternionFromRPY(0,0,acadoVariables.x[NX*tt + 12]), 
  //    tf::Vector3(acadoVariables.x[NX*tt],
  //                acadoVariables.x[NX*tt + 1],
  //                acadoVariables.x[NX*tt + 2]));

  //   double x,y,z,ga,q1,q2;
  //   x = acadoVariables.x[NX*tt];
  //   y = acadoVariables.x[NX*tt + 1];
  //   z = acadoVariables.x[NX*tt + 2];
  //   ga = acadoVariables.x[NX*tt + 12];
  //   q1 = acadoVariables.x[NX*tt + 14];
  //   q2 = acadoVariables.x[NX*tt + 15];
  //   double l1 = 0.175; double l2 = 0.42;

      for(int s=0; s < NX; s++) states << acadoVariables.x[NX*tt + s] <<" ";
      states <<"\n";

      for(int u=0; u < NU; u++) controls << acadoVariables.u[NU*tt + u] <<" ";
      controls <<"\n";

  //   ROS_INFO("ee : %f %f %f", x + (l1*cos(q1) + l2*cos(q1+q2))*cos(ga),
  //     y + (l1*cos(q1) + l2*cos(q1+q2))*sin(ga),
  //     z + l1*sin(q1) + l2*sin(q1+q2));

  //   ROS_INFO("joints: %f %f", q1, q2);

  //   sensor_msgs::JointState joint_state;
  //   joint_state.name.push_back("airbasetolink1");
  //   joint_state.name.push_back("link1tolink2");

  //   joint_state.position.push_back(-acadoVariables.x[NX*tt + 14] + 1.57);
  //   joint_state.position.push_back(-acadoVariables.x[NX*tt + 15]);
  //   joint_state.header.stamp = ros::Time::now();

  //   jointPub.publish(joint_state);

  //   nav_msgs::Odometry odom;
  //   odom.header.stamp = ros::Time::now();
  //   odom.header.frame_id = "world";
  //   odom.pose.pose.position.x = acadoVariables.x[NX*tt];
  //   odom.pose.pose.position.y = acadoVariables.x[NX*tt+1];
  //   odom.pose.pose.position.z = acadoVariables.x[NX*tt+2];

  //   odom.pose.pose.orientation = tf::createQuaternionMsgFromRollPitchYaw(0,0,acadoVariables.x[NX*tt + 12]);

  //   odom.twist.twist.linear.x = acadoVariables.x[NX*tt + 3];
  //   odom.twist.twist.linear.y = acadoVariables.x[NX*tt + 4];
  //   odom.twist.twist.linear.z = acadoVariables.x[NX*tt + 5];

  //   odom.twist.twist.angular.z = acadoVariables.x[NX*tt + 13];

  //   odomPub.publish(odom);

  //   br.sendTransform(tf::StampedTransform(transform, ros::Time::now(), "world", "baselink"));
  //   marker.header.stamp = ros::Time::now();
  //   goalPub.publish(marker);
  //   // for(tnt o = 0; o < NUM_OBS; o++) {
  //   //   obs_marker[o].header.stamp = ros::Time::now();
  //   //   obsPub.publish(obs_marker[o]);
  //   // }
  //   ros::Duration(1.0/N).sleep();
  }
  return 0;
}