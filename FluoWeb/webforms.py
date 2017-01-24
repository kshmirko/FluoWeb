from wtforms import Form, validators, SelectField, FloatField, IntegerField, FieldList, StringField, TextAreaField, BooleanField
import glob

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
    currentPosition = FloatField(label='Центральная длина волны (нм.):', id='currentPosition')
    currentGrating=SelectField(label="Решётка:", choices=[(1,'№1'), (2, '№2')], id='currentGrating')
    
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


class StartApp(Form):
    pass