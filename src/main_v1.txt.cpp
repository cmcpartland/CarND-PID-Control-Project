#include <uWS/uWS.h>
#include <iostream>
#include "json.hpp"
#include "PID.h"
#include <math.h>
#include <ctime>
#include <cstdio>

// for convenience
using json = nlohmann::json;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
std::string hasData(std::string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.find_last_of("]");
  if (found_null != std::string::npos) {
    return "";
  }
  else if (b1 != std::string::npos && b2 != std::string::npos) {
    return s.substr(b1, b2 - b1 + 1);
  }
  return "";
}

int main()
{
  uWS::Hub h;
  const double speed_limit = 50.0;
  PID pid_steering;
  // TODO: Initialize the pid variable.
  pid_steering.Init(0.18, 0.00000005, 3.5+(speed_limit/100.0));
  
  PID pid_throttle;
  pid_throttle.Init(0.09, 0.000001, 1.1);
  
  std::clock_t start;
  start = std::clock();
  
  double error = 0;
  double best_error = 0;
  
  int num_runs = 0;
  int param_index = 0;
  std::vector<double> dp;
  dp.push_back(0.1);
  dp.push_back(0.1);
  dp.push_back(0.1);
  std::vector<double> params;
  params.push_back(0.18);
  params.push_back(0.00000005);
  params.push_back(3.5+(speed_limit/100.0));
  
  bool testing_param = false;

  h.onMessage([&pid_steering, &pid_throttle, &speed_limit, &start, &error, &best_error, &num_runs, &param_index, &dp, &params, &testing_param](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    if (length && length > 2 && data[0] == '4' && data[1] == '2') {
      auto s = hasData(std::string(data).substr(0, length));
      if (s != "") {
        auto j = json::parse(s);
        std::string event = j[0].get<std::string>();
        if (event == "telemetry") {
          // j[1] is the data JSON object
          double cte = std::stod(j[1]["cte"].get<std::string>());
          double speed = std::stod(j[1]["speed"].get<std::string>());
          double angle = std::stod(j[1]["steering_angle"].get<std::string>());
          double steer_value;
		  double throttle_value;
		  
          /*
          * TODO: Calculate steering value here, remember the steering value is
          * [-1, 1].
          * NOTE: Feel free to play around with the throttle and speed. Maybe use
          * another PID controller to control the speed!
          */
		  
		  pid_steering.UpdateError(cte);
		  steer_value = pid_steering.TotalError();
		  if (steer_value <= -1) {
			steer_value = -1;
		  }
		  else if (steer_value >= 1) {
			steer_value = 1;
		  }
		  if (speed >= 25.0 && fabs(cte) > 1.0) {
			  //throttle_value = -.01;
			  ;
		  }
		  else {
			  pid_throttle.UpdateError(speed-speed_limit);
			  throttle_value = pid_throttle.TotalError();
			  if (throttle_value >= speed_limit/10.0) {
				  throttle_value = speed_limit/100.0;
			  }
		  }
		  
		  error += pow(cte,2);
			   
          // DEBUG
          std::cout << "CTE: " << cte << " Steering Value: " << steer_value << std::endl;

          json msgJson;
          msgJson["steering_angle"] = steer_value;
          msgJson["throttle"] = throttle_value;
          auto msg = "42[\"steer\"," + msgJson.dump() + "]";
          std::cout << msg << std::endl;
          ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
		  
		  const double run_time = (std::clock() - start) /(double)CLOCKS_PER_SEC;
		  std::cout << "Run time: " << run_time << std::endl;
		  
		  
		  

		  if (run_time > 10.6) {
			  num_runs += 1;
			  if (testing_param) {
				  if (error < best_error) {
					  best_error = error;
					  dp[param_index] *= 1.1;
				  }
				  else {
					  params[param_index] += dp[param_index];
					  dp[param_index] *= 0.9;
				  }
				  testing_param = false;
				  param_index += 1;
				  if (param_index == 3) {
					param_index = 0;
				  }
			  }
			  else {
				  params[param_index] += dp[param_index];
				  if (num_runs == 1) {
					best_error = error;
					error = 0;
				  }
				  else {	
					if (error < best_error) {
						best_error = error;
						dp[param_index] *= 1.1;
						error = 0;
						param_index += 1;
						if (param_index == 3) {
							param_index = 0;
						}
					}
					else {
						params[param_index] -= 2*dp[param_index];
						testing_param = true;
					}
				  }
			  }
		  start = std::clock(); // restart the clock
		  std::cout << "Iteration: " << num_runs << std::endl;
		  }		 
        }
        else {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
        }
	  }
    }
  });

  // We don't need this since we're not using HTTP but if it's removed the program
  // doesn't compile :-(
  h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t, size_t) {
    const std::string s = "<h1>Hello world!</h1>";
    if (req.getUrl().valueLength == 1)
    {
      res->end(s.data(), s.length());
    }
    else
    {
      // i guess this should be done more gracefully?
      res->end(nullptr, 0);
    }
  });

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port))
  {
    std::cout << "Listening to port " << port << std::endl;
  }
  else
  {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  h.run();
}