#include "PID.h"
#include <iostream>
using namespace std;

PID::PID() {}

PID::~PID() {}

void PID::Init(double Kp, double Ki, double Kd) {
	PID::Kp = Kp;
	PID::Ki = Ki;
	PID::Kd = Kd;
}

void PID::UpdateError(double cte) {
	d_error = cte - p_error; // p_error contains the previous value of cte
	p_error = cte;
	i_error += cte;
	
	// To avoid integral windup, cap the error integral at +/- 200
	if (i_error >= 200 || i_error <= -200) {
		i_error -= cte; // subtract cte from i_error if it pushes it out of bounds
	}
	// DEBUG
	//std::cout << "P term: " << p_error*Kp << "\t I term: " << i_error*Ki << "\t D term: " << d_error*Kd << endl;
}

double PID::TotalError() {
	return -Kp*p_error - Ki*i_error - Kd*d_error;
}

