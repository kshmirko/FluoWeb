import serial

def move_spec(device, wl):
    cmd = ("%f goto\r"%wl).encode('utf-8')
    print('SPECTRO: %s'%cmd)
    dev = serial.Serial(device, timeout=600)
    dev.write(cmd)
    resp = dev.readline()
    print(resp.decode('utf-8'))
    dev.close()
    