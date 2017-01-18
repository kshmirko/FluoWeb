from flask import render_template, g, request, url_for, redirect, session
from FluoWeb import app
from wtforms import Form, validators, SelectField, FloatField
import glob

from .speccontrol import move_spec

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
navigation.append(Navigation(href='/start', caption='Запуск', active=False))
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
    
    
            
        

@app.route('/')
@app.route('/<page>', methods=['GET','POST'])
def index(page='status'):
    # prepare session variables
    #if page=='status':        
    #    session['fluoDev'] = None
    #    session['specDev'] = None
    #    session['currentPosition'] = 0.0
    
    # prepare menu
    for item in navigation:
        item.active=False
        if page in item.href:
            item.active=True
    
    
    form=None
    
    if page == 'deviceassoc':
        form=DeviceAssocForm(request.form)
        
        if request.method=='POST' and form.validate():
            session['fluoDev'] = str(form.fluoDev.data)
            session['specDev'] = str(form.specDev.data)
                    
            return render_template('%s.html'%('status'), navigation=navigation, form=form, status=session)
            
    elif page=='reset':

        session.clear()
        # сбрасываем настройки и отключаем все подключенные устройства
    
    elif page=='configspec': 
        form=ConfigSpecForm(request.form)
        
        if request.method=='GET':
            form.currentPosition.data = float(getattr(session,'currentPosition',0.0))
            
        elif request.method=='POST' and form.validate():
            session['currentPosition'] = float(form.data['currentPosition'])
            if 'specDev' in session:
                move_spec(session['specDev'], session['currentPosition'])
            
            return render_template('%s.html'%('status'), navigation=navigation, form=form, status=session)
    
    elif page=='start':
        return render_template('%s.html'%page, navigation=navigation, form=form, status=session, leds=list(range(8)), turs=list(range(8)))
    
        
    
    return render_template('%s.html'%(page), navigation=navigation, form=form, status=session)





    