import serial

def move_spec(device, wl):
    cmd = ("%.1f goto\r"%wl).encode('utf-8')
    print('SPECTRO: %s'%cmd)
    dev = serial.Serial(device, timeout=600)
    dev.write(cmd)
    resp = dev.readline()
    print(resp.decode('utf-8'))
    dev.close()

def move_grating(device, pos):
    """"""
    cmd = ("%d grating\r"%pos).encode('utf-8')
    print('SPECTRO: %s'%cmd)
    dev = serial.Serial(device, timeout=600)
    dev.write(cmd)
    resp = dev.readline()
    print(resp.decode('utf-8'))
    dev.close()