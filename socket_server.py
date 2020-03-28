import socket
import struct
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib import style
import threading
import time
import datetime
import dateutil

accel_data = []

def socket_thread():
  s = socket.socket()
  addr = ('0.0.0.0', 6969)

  s.bind(addr)

  s.listen(0)

  print('Socket is listening on', addr)

  filename = 'data-%s.txt' % time.strftime("%Y%m%d-%H%M%S")

  with open(filename, 'a') as f:
    print('Openning file', filename)

    while True:
      conn, addr = s.accept()
      with conn:
        conn.settimeout(15.0)

        print('Connection from ', addr)

        bytes_recieved = 0
        buffer = b''
        counter = 0

        start_time = datetime.datetime.utcnow()

        while True:
          try:
            received = conn.recv(15000)
            if not received: continue
          except socket.timeout:
            print('timeout')
            break
          except ConnectionResetError:
            print('reset')
            break

          bytes_recieved += len(received)
          buffer += received

          print(bytes_recieved)

          if bytes_recieved >= 18000:
            for i in range(0, len(buffer), 18):
              tt = struct.unpack("12s", buffer[i:i+12])[0]

              t = dateutil.parser.parse(tt)
              x,y,z = struct.unpack("3h", buffer[i+12:i+18])

              x *= 0.00006103515
              y *= 0.00006103515
              z *= 0.00006103515

              if (i / 3) % 5 == 0:
                f.write('%sZ: %f, %f, %f\n' % (t.isoformat(sep=' ', timespec='milliseconds'), x, y, z))
                f.flush()

            print('Written to file.')

            buffer = b''
            bytes_recieved = 0
    f.close()

def main():
  t1 = threading.Thread(target=socket_thread)
  t1.daemon = True
  t1.start()

  while True:
    time.sleep(1)

if __name__ == '__main__':
  main()