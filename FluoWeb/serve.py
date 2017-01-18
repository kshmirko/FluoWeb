from flask import Flask, render_template, g, request, session

app = Flask(__name__)

class Navigation:
    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)

navigation = []
navigation.append(Navigation(href='/status', caption='Состояние', active=True))
navigation.append(Navigation(href='/device_assoc', caption='Сопоставление устройств', active=False))
navigation.append(Navigation(href='/config_fluo', caption='Настройка флуориметра', active=False))
navigation.append(Navigation(href='/setup_ltr11', caption='Настройка LTR-11', active=False))
navigation.append(Navigation(href='/setup_ltr210', caption='Настройка LTR-210', active=False))
navigation.append(Navigation(href='/start', caption='Запуск', active=False))
navigation.append(Navigation(href='/reset', caption='Отключение', active=False))        


@app.route('/')
@app.route('/<page>')
def index(page='status', methods=['GET','POST']):
    # prepare menu
    for item in navigation:
        item.active=False
        if page in item.href:
            item.active=True
    if request.method=='POST':
        return {'request'}
        pass
    else:
        return render_template('%s.html'%(page), navigation=navigation)


if __name__=='__main__':
    app.run() 