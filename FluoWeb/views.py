from flask import render_template, g, request, url_for, redirect, session, jsonify
from FluoWeb import app
from wtforms import Form, validators, SelectField, FloatField, IntegerField, FieldList, StringField, TextAreaField, BooleanField
import glob



from .speccontrol import move_spec
from .fluocontrol import LED_KEYS, TUR_KEYS, LED_OFF, FluoDevice
TMP_KEYS=list(range(8))

fluorimeter = FluoDevice()

class Navigation:
    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)
        
class Status:
    pass

navigation = []
navigation.append(Navigation(href='/status', caption='Состояние', active=True))
navigation.append(Navigation(href='/deviceassoc', caption='Сопоставление устройств', active=False))
navigation.append(Navigation(href='/configfluo', caption='Настройка флуориметра', active=False))
navigation.append(Navigation(href='/configspec', caption='Настройка спектрометра', active=False))
navigation.append(Navigation(href='/setupltr11', caption='Настройка LTR-11', active=False))
navigation.append(Navigation(href='/setupltr210', caption='Настройка LTR-210', active=False))
navigation.append(Navigation(href='/start', caption='Работа с излучением в непрерывном режиме', active=False))
navigation.append(Navigation(href='/reset', caption='Отключение', active=False))        

Global = Status()

class DeviceAssocForm(Form):

    fluoDev = SelectField('Путь к устройству "флуориметр":', choices=[("/dev/ttyUSB0", "/dev/ttyUSB0"),("/dev/ttyUSB1", "/dev/ttyUSB1")])
    specDev = SelectField('Путь к устройству "спектрометр":', choices=[("/dev/ttyUSB0", "/dev/ttyUSB0"),("/dev/ttyUSB1", "/dev/ttyUSB1")])
    
    def __init__(self, *args, **kwargs):
        super(DeviceAssocForm, self).__init__(*args, **kwargs)
        # поиск терминалов
        devs = glob.glob('/dev/tty*')
        self.fluoDev.choices.clear()
        self.specDev.choices.clear()
        for d in devs:
            self.fluoDev.choices.append((d,d))
            self.specDev.choices.append((d,d))
            
class ConfigSpecForm(Form):
    currentPosition = FloatField(label='Центральная длина волны (нм.):')
    
class ConfigLtr11(Form):
    startADCMode = SelectField(label='Режим запуска АЦП', choices=[(0,'INTERNAL'),(1, 'EXTERNAL RISE'),(2,'EXTERNAL FALL')], default=0, id="startADCMode")
    inputADCMode = SelectField(label='Режим ввода данных в АЦП', choices=[(0,'EXTERNAL RISE'),(1,'EXTERNAL FALL'),(2,'INTERNAL')], default=1, id="inputADCMode")
    frequency = FloatField(label='Частота сбора данных (Гц)', default=400000, id="frequency")
    meas_time = FloatField(label='Время измерения в мкс.', default=0.0, id="meas_time")
    num_blocks = IntegerField(label='Количество блоков данных', default=1, id="num_blocks")
    channels = SelectField(label="Каналы", choices=[], id="channels")
    
class ConfigLtr210(Form):
    syncMode = SelectField(label='Режим синхронизации АЦП:', choices=[(0,'INTERNAL'),(1, 'CH1 RISE'),(2,'CH1 FALL'),\
                            (3, 'CH2_RISE'),(4, 'CH2_FALL'),(5,'SYNC_IN_RISE'),(6,'SYNC_IN_FALL'),
                            (7, 'PERIODIC'),(8, 'CONTINUOUS')], default=7, id="syncMode")
    histTime = FloatField(label="Размер предыстории (мкс.):", default=0.0, id='histTime')
    measTime = FloatField(label="Время измерений (мкс.):", default=0.0, id='measTime')
    frequency= IntegerField(label="Частота сбора данных (Гц):", default=10000000, id='frequency')
    numBlocks = IntegerField(label="Число блоков данных:", default=1, id="numBlocks")
    frameFreq = IntegerField(label="Частота сбора кадров (Гц):", default=1, id='frameFreq')
    measZero = BooleanField(label="Измерение нуля?", default=False, id="measZero")
    ch1_en = BooleanField(label="Канал 1", default=False, id="ch1_en")
    ch1_range = SelectField(label="Макс. напряжкние на входе:", default=0, choices=[(0, '10 В'),(1,"5 В"),\
                            (2,"2 В"), (3,"1 В"), (4, "0.5 В")], id="ch1_range")
    ch1_mode = SelectField(label="Режим работы канала:", default=0, choices=[(0, 'AC/DC'),(1,"AC"),\
                            (2,"Zero")], id="ch1_mode")
    ch2_en = BooleanField(label="Канал 2", default=False, id="ch2_en")
    ch2_range = SelectField(label="Макс. напряжкние на входе:", default=0, choices=[(0, '10 В'),(1,"5 В"),\
                            (2,"2 В"), (3,"1 В"), (4, "0.5 В")], id="ch2_range")
    ch2_mode = SelectField(label="Режим работы канала:", default=0, choices=[(0, 'AC/DC'),(1,"AC"),\
                            (2,"Zero")], id="ch2_mode")



def make_navigation(page):
    for item in navigation:
        item.active=False
        if page in item.href:
            item.active=True
    

@app.route('/')
@app.route('/status', methods=['GET','POST'])
def status():
    make_navigation('status')
    return render_template('status.html', navigation=navigation, status=session)
    
@app.route('/deviceassoc', methods=['GET','POST'])
def device_assoc():
    make_navigation('deviceassoc')
    
    form=DeviceAssocForm(request.form)
    
    if request.method=='POST' and form.validate():
        session['fluoDev'] = str(form.fluoDev.data)
        session['specDev'] = str(form.specDev.data)
        fluorimeter.port = session['fluoDev'] 
        fluorimeter.baudrate=9600
        return render_template('%s.html'%('status'), navigation=navigation, form=form, status=session)
    
    return render_template('deviceassoc.html', navigation=navigation, form=form, status=session)
    
@app.route('/reset', methods=['GET','POST'])
def reset():
    make_navigation('status')
    
    session.clear()
    
    return render_template('status.html', navigation=navigation, status=session)
    

@app.route('/configspec', methods=['GET','POST'])
def configspec():
    make_navigation('configspec')
    
    form=ConfigSpecForm(request.form)
    
    if request.method=='GET':
        form["currentPosition"].data = float(getattr(session,'currentPosition',0.0))
    elif request.method=='POST' and form.validate():
        session['currentPosition'] = float(form.data['currentPosition'])
        if 'specDev' in session:
            move_spec(session['specDev'], session['currentPosition'])
        
        return render_template('status.html', navigation=navigation, form=form, status=session)
    
    return render_template('configspec.html', navigation=navigation, form=form, status=session)
    

@app.route('/start', methods=['GET','POST'])
def start():
    make_navigation('start')
    
    if request.method=='POST':
        tmpform = request.form
    
        key = None
        if 'led' in tmpform:
            key = LED_KEYS[int(tmpform['led'])]
            if not fluorimeter.isOpen():
                fluorimeter.open()
                fluorimeter.initialize()
            print(fluorimeter.send_char_led(key))
            
        elif 'tur' in tmpform:
            key = TUR_KEYS[int(tmpform['tur'])]
            if not fluorimeter.isOpen():
                fluorimeter.open()
                fluorimeter.initialize()
            print(fluorimeter.send_char(key))
        else:
            if fluorimeter.isOpen():
                fluorimeter.leave_test()
                fluorimeter.close()
            return redirect(url_for('status'))
            
    return render_template('start.html', navigation=navigation, status=session, leds=TMP_KEYS, turs=TMP_KEYS)
    
    
@app.route('/setupltr11', methods=['GET'])
def setupltr11():
    make_navigation('setupltr11')
    form=ConfigLtr11(request.form)
    return render_template('setupltr11.html', navigation=navigation, form=form, status=session)
    
@app.route('/setupltr210', methods=['GET'])
def setupltr210():
    make_navigation('setupltr210')
    form=ConfigLtr210(request.form)
    return render_template('setupltr210.html', navigation=navigation, form=form, status=session)
    
@app.route('/setupltr11ajax', methods=['POST'])
def setupltr11ajax():
    # Save data to file
    print(request.form["startADCMode"])
    fname='ltr11.config'
    config = []
    config.append('START ADC MODE: %s\n'%request.form["startADCMode"])
    config.append('INPUT MODE: %s\n'%request.form["inputADCMode"])
    config.append('ADC MODE: 0\n')
    config.append('FREQUENCY: %d Hz\n'%int(request.form["frequency"]))
    config.append('LOGICAL CHANNELS COUNT: %s\n'%request.form["num_channels"])
    config.append(request.form["channels"])
    config.append('MEASUREMENTS TIME (mks): %.2f\n'%float(request.form["meas_time"]))
    config.append('REQUIRED BLOCKS : %d\n'%int(request.form["num_blocks"]))
    with open(fname,'wt') as txt:
        txt.writelines(config)
        
    
    session['ltr11']=fname
    return jsonify(config_file=fname)
    
@app.route('/setupltr210ajax', methods=['POST'])
def setupltr210ajax():
    fname='ltr210.config'
    print(request.form["frequency"])
    return jsonify(**{})
