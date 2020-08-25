import serial
import time
from sqlite import Sqlite

def main():
    ser = serial.Serial(port='COM6',baudrate=115200,parity=serial.PARITY_NONE,stopbits=serial.STOPBITS_ONE,bytesize=serial.EIGHTBITS,timeout=0)
    db = Sqlite('database')
    print("connected to: " + ser.portstr)
    # sleep a clock cycle to wait data ready in serial
    time.sleep(1)

    while True:
        timestamp = int(time.time())
        system_time, led0, led1 = ser.readline().decode().split(',')
        led = (timestamp, system_time, led0, led1)
        print(led)
        db.insert(led)
        time.sleep(1)

    ser.close()

def check():
    """
    check data saved in database
    """
    db = Sqlite('database')
    db.select()

if __name__ == '__main__':
    main()
    #check()