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
  
  // Set the safe speed limit for the vehicle
  const double speed_limit = 60.0;
  
  PID pid_steering;
  
  // D parameter includes an extra term based on the speed limit. For higher speeds, greater damping is necessary.
  // I parameter is chosen so that it can handle gentle consistent curves in the road. Since the integral error is capped,
  // the I term can only 0.3 to the steering value. 
  pid_steering.Init(0.19, 0.0016, 2.8+(speed_limit/100.0));

  PID pid_throttle;
  pid_throttle.Init(0.09, 0.0005, 2.1);  
  int increment = 1;
  h.onMessage([&pid_steering, &pid_throttle, &speed_limit, &increment](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
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
          //double angle = std::stod(j[1]["steering_angle"].get<std::string>());
          double steer_value;
		  double throttle_value;
		  
		  // Update the steering PID with the new error term
		  pid_steering.UpdateError(cte);
		  
		  // Calculate the new steering value
		  steer_value = pid_steering.TotalError();
		  
		  // Cap the steering values at -1 and 1
		  if (steer_value <= -1) {
			steer_value = -1;
		  }
		  else if (steer_value >= 1) {
			steer_value = 1;
		  }
		  
		  // Update the throttle PID with the new error term
		  // -> Add an offset to the error term proportional to the cte, this way the vehicle slows down when the cte increases
		  pid_throttle.UpdateError(speed-speed_limit+15*cte);
		  
		  // Calculate the new throttle value
		  throttle_value = pid_throttle.TotalError();
		  
		  // Cap the throttle at 1
		  if (throttle_value >= 1) { 
			  throttle_value = 1;
		  }
		  			   
          // DEBUG
          std::cout << "CTE: " << cte << " Steering Value: " << steer_value << std::endl;
		  increment++;

          json msgJson;
          msgJson["steering_angle"] = steer_value;
          msgJson["throttle"] = throttle_value;
          auto msg = "42[\"steer\"," + msgJson.dump() + "]";
          std::cout << msg << std::endl;
          ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
		}
	  }
      else {
      // Manual driving
      std::string msg = "42[\"manual\",{}]";
      ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
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
