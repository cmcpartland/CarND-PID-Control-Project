from matplotlib import pyplot as plt
import csv
import numpy as np

filename = r"C:\Users\cmcpa\OneDrive\Desktop\cte_data2_short.txt"
file = open(filename, 'r')

data = csv.reader(file)
data_points = []
for line in data:
	data_points.append(float(line[0].split(' ')[-1]))

cte_values = np.array(data_points)
time_values = np.arange(len(cte_values))

Kp = 0.19
Ki = 0.0016
Kd = 4.3
prev_cte = 0;
p_contribution = []
i_contribution = []
i_sum = 0
d_contribution = []
steer_values = []
for cte in cte_values:
	p_contribution.append(-cte*Kp)
	i_sum += cte
	if i_sum < -150 or i_sum > 150:
		i_sum -= cte
	i_contribution.append(-i_sum*Ki)
	d_contribution.append(-Kd*(cte-prev_cte))
	steer_values.append(-cte*Kp -i_sum*Ki -Kd*(cte-prev_cte))
	prev_cte = cte
	

plt.ylim(-3.0, 3.0)
cte_plot, = plt.plot(time_values, cte_values, linewidth=2.0, label='CTE',color='r')
p_plot, = plt.plot(time_values, p_contribution, color='blue', linestyle='--', label='P')
i_plot, = plt.plot(time_values, i_contribution, color='gray', linestyle='--', label='I')
d_plot, = plt.plot(time_values, d_contribution, color='green', linestyle='--', label='D')
steer_plot, = plt.plot(time_values, steer_values, color='purple', label='Steer value')
plt.legend(handles=[cte_plot, p_plot, i_plot, d_plot, steer_plot])
plt.grid(True)
plt.show()