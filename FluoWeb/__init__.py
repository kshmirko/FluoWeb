#from gevent import monkey
#monkey.patch_all()
from flask import Flask
#from gevent import wsgi

app = Flask(__name__)
app.config['SECRET_KEY'] = '123123123'


import FluoWeb.views


#server = wsgi.WSGIServer(('127.0.0.1', 5000), app)
#server.serve_forever()