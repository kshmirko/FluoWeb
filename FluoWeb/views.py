from flask import render_template, g, request, url_for, redirect, session, jsonify, Response
from FluoWeb import app
from functools import wraps
import time

from .speccontrol import move_spec
from .fluocontrol import LED_KEYS, TUR_KEYS, LED_OFF, FluoDevice
from .webforms import DeviceAssocForm, ConfigSpecForm, ConfigLtr11, ConfigLtr210
TMP_KEYS=list(range(8))

fluorimeter = FluoDevice()

class Navigation:
    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)
        

navigation = []
navigation.append(Navigation(href='/status', caption='Состояние', active=True))
navigation.append(Navigation(href='/deviceassoc', caption='Сопоставление устройств', active=False))
navigation.append(Navigation(href='/configfluo', caption='Настройка флуориметра', active=False))
navigation.append(Navigation(href='/configspec', caption='Настройка спектрометра', active=False))
navigation.append(Navigation(href='/setupltr11', caption='Настройка LTR-11', active=False))
navigation.append(Navigation(href='/setupltr210', caption='Настройка LTR-210', active=False))
navigation.append(Navigation(href='/start', caption='Работа с излучением в непрерывном режиме', active=False))
navigation.append(Navigation(href='/reset', caption='Отключение', active=False))        

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
    

@app.route('/configspec', methods=['GET'])
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

@app.route("/configspecajax", methods=['POST'])
def configspecajax():
    cmd = request.form['cmd']
    param = request.form['param']
    message='Spectrometer device is not assigned'
    print(cmd, param)
    if 'specDev' in session:
        
        if cmd== 'wave':
            session['currentPosition'] = float(param)
            time.sleep(5)
            move_spec(session['specDev'], session['currentPosition'])
            message = 'Wavelength successfully changed to {}'.format(session['currentPosition'])
        elif cmd=='grat':
            session['currentGrating'] = int(param)
            time.sleep(20)
            move_grating(session['specDev'], session['currentGrating'])
            message = 'Grating successfully changed to {}'.format(session['currentGrating'])
        
    return jsonify(message=message)

@app.route('/start', methods=['GET','POST'])
def start():
    make_navigation('start')
    
    
            
    return render_template('start.html', navigation=navigation, status=session, leds=TMP_KEYS, turs=TMP_KEYS)

@app.route('/startajax', methods=['POST'])
def startajax():
    data = request.form["key"]
    pref=data[:3]
    cmd=data[3:]
    if pref == 'led':
        if cmd=='off' and fluorimeter.isOpen():
            fluorimeter.leave_test()
            fluorimeter.close()
        elif cmd in '01234567':
            key = LED_KEYS[int(cmd)]
            if not fluorimeter.isOpen():
                fluorimeter.open()
                fluorimeter.initialize()
            print(fluorimeter.send_char_led(key))
    elif pref == 'tur':
        if cmd=='off' and fluorimeter.isOpen():
            fluorimeter.leave_test()
            fluorimeter.close()
        elif cmd in '01234567':
            key = TUR_KEYS[int(cmd)]
            if not fluorimeter.isOpen():
                fluorimeter.open()
                fluorimeter.initialize()
            print(fluorimeter.send_char_led(key))
            
    elif pref == 'syn':
        time.sleep(4)
    elif pref == 'l11':
        time.sleep(4)
    elif pref == '210':
        time.sleep(4)
            
    return jsonify(res=cmd)
    
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
    
    config = []
    config.append("SYNC MODE: %s\n"%(request.form["syncMode"]))
    config.append("FLAGS: 3\n")
    config.append("HISTORY TIME (mks): %.2f\n"%(float(request.form["histTime"])))
    config.append("MEASUREMENTS TIME (mks): %.2f\n"%(float(request.form["measTime"])))
    config.append("FREQUENCY: %d Hz\n"%(int(request.form["frequency"])))
    config.append("MEAS ZERO OFFSET: %d\n"%int(request.form["measZero"]=='true'))
    config.append("REQUIRED BLOCKS: %d\n"%(int(request.form["numBlocks"])))
    config.append("CH[0].Enabled: %d\n"%int(request.form["ch1_en"]=='true'))
    config.append("CH[0].Range: %d\n"%int(request.form["ch1_range"]))
    config.append("CH[0].Mode: %d\n"%int(request.form["ch1_mode"]))
    config.append("CH[1].Enabled: %d\n"%int(request.form["ch2_en"]=='true'))
    config.append("CH[1].Range: %d\n"%int(request.form["ch2_range"]))
    config.append("CH[1].Mode: %d\n"%int(request.form["ch2_mode"]))
    config.append("FRAME FREQ: %d Hz\n"%(int(request.form["frameFreq"])))
    with open(fname,'wt') as txt:
        txt.writelines(config)
    
    
    session['ltr210']=fname
    return jsonify(config_file=fname)
