from flask import render_template, g, request, url_for, redirect, session
from FluoWeb import app
from wtforms import Form, validators, SelectField, FloatField, IntegerField, FieldList, StringField
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
    startADCMode = SelectField(label='Режим запуска АЦП', choices=[('INTERNAL',0),('EXTERNAL RISE',1),('EXTERNAL FALL',2)])
    inputADCMode = SelectField(label='Режим ввода данных в АЦП', choices=[('EXTERNAL RISE',1),('EXTERNAL FALL',2),('INTERNAL',0)])
    frequency = FloatField(label='Частота сбора данных (Гц)')
    meas_time = FloatField(label='Время измерения в мкс.')
    num_blocks = IntegerField(label='Количество блоков данных')
    channels = FieldList(StringField('channel', [validators.DataRequired()]))


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
    
    
@app.route('/setupltr11', methods=['GET', 'POST'])
def setupltr11():
    make_navigation('setupltr11')
    form=ConfigLtr11(request.form)
    
    return render_template('setupltr11.html', navigation=navigation, status=session, form=form)
    
            





    