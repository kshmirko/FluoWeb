import serial

CR = b'\r'
HEADER_STR_LEN = 156
TEST_HEADER_LEN=226

LED_KEYS = ['a','s','d','f','g','h','j','k']
TUR_KEYS = ['z','x','c','v','b','n','m','q']
LED_OFF =  ['e']
SYNC_KEY = ['w']

class FluoDevice(serial.Serial):

    def __init__(self, *args, **kwargs):
        super(FluoDevice, self).__init__(*args, **kwargs)
        self.test_mode = False

    def initialize(self):
        self.write(CR)
        v = self.read(HEADER_STR_LEN)
        self.test_mode = False
        return v
        
    def enter_test(self):
        self.write(b't')
        v=self.read(TEST_HEADER_LEN)
        self.test_mode = True
        return v
    
    def leave_test(self):
        v=None
        if self.test_mode:
            self.write(b'e')
            v=self.read(HEADER_STR_LEN)
            self.test_mode = False
        return v
    
    def send_char(self, ch):
        if not self.test_mode:
            self.enter_test()
        
        n=self.write(ch.encode('ascii'))
        v=self.read(TEST_HEADER_LEN)
        return v
        
    def send_char_led(self, ch):
        self.leave_test() # Switch off LED
        self.enter_test()
        
        n=self.write(ch.encode('ascii'))
        v=self.read(TEST_HEADER_LEN)
        return v  
