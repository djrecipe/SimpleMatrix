from subprocess import call
import os
os.chdir('/home/pi/SimpleMatrix')
call('/home/pi/SimpleMatrix/display-test /home/pi/SimpleMatrix/matrix.cfg', shell=True)