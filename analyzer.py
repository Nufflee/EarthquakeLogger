import math
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.dates as md
import datetime
import dateutil
import time
from scipy import signal
import numpy as np

data = {}

"""
1. Automated way of downloading data & uploading scripts to the Raspberry Pi (scp etc.)
2. Log times of interest
3. Plot data
  - Live data plotting
4. Try to use this for detecting other kinds of vibrations, like walking
5. Look into ways of increasing sensitivity
  - Like putting it on a stick that'll act as an oscillator
"""

def calc_mag(data):
  return math.sqrt(data['x'] ** 2 + data['y'] ** 2 + data['z'] ** 2)

start = time.time()

with open('data-20200328-021420.txt') as f:
  for i, line in enumerate(f.readlines()):
    parts = line.split(' ')
    date = ' '.join(parts[:2])[:-1]
    x = float(parts[2][:-1])
    y = float(parts[3][:-1])
    z = float(parts[4])

    xyz = {'x': x, 'y': y, 'z': z}
    mag = calc_mag(xyz)

    if (mag > 1.1):
      pass
      #print(date, mag)

    #outfile.write('%s - %f, %f, %f\n' % (date, x, y, z))
    #outfile.flush()

    data[date] = xyz

print('Processed in %f seconds' % (time.time() - start))

#plot_data = list(data.items())[:-50000]
plot_data = list(data.items())#[321500:421500]
#plot_data = data

fs = 100
fc = 10  # Cut-off frequency of the filter
w = fc / (fs / 2) # Normalize the frequency
b, a = signal.butter(10, w, 'low')

x_ys = [i[1]['x'] for i in plot_data]
y_ys = [i[1]['y'] for i in plot_data]
z_ys = [i[1]['z'] for i in plot_data]

#x_ys = x_ys[:150000] + x_ys[170000:]
#y_ys = y_ys[:150000] + y_ys[170000:]
#z_ys = z_ys[:150000] + z_ys[170000:]
xs1 = [dateutil.parser.parse(v[0]) for v in plot_data]#[dateutil.parser.parse(v[0]) for v in plot_data] #[i for i in range(len(x_ys))]
xs2 = [i for i in range(len(x_ys))]#[dateutil.parser.parse(v[0]) for v in plot_data] #[i for i in range(len(x_ys))]

print(len(data))

#x_ys = signal.filtfilt(b, a, x_ys)
#y_ys = signal.filtfilt(b, a, y_ys)
#z_ys = signal.filtfilt(b, a, z_ys)

print('x avg: %f g' % np.mean(x_ys))
print('y avg: %f g' % np.mean(y_ys))
print('z avg: %f g' % np.mean(z_ys))

print('x pp: %f g' % (np.max(x_ys) - np.min(x_ys)))
print('y pp: %f g' % (np.max(y_ys) - np.min(y_ys)))
print('z pp: %f g' % (np.max(z_ys) - np.min(z_ys)))

fig, ax = plt.subplots()

plt.subplots_adjust(bottom=0.2)
plt.xticks(rotation=25)
#xfmt = md.DateFormatter('%H:%M:%S.%f')
#ax.xaxis.set_major_formatter(xfmt)

ax.set(xlabel='Time', ylabel='Values (g)')
ax.grid()

ax.plot(xs1, x_ys, label='X axis (g)')
#plt.plot(xs1, y_ys, label='Y axis (g)', color='green')
ax.plot(xs1, y_ys, label='Y axis (g)')
ax.plot(xs1, z_ys, label='Z axis (g)')

plt.legend(loc=1)

print('Plotted in %f seconds' % (time.time() - start))

plt.show()