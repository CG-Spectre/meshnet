from PyQt5.QtWebEngineWidgets import QWebEngineView
from PyQt5.QtWebEngineCore import QWebEngineUrlRequestInterceptor, QWebEngineUrlSchemeHandler, QWebEngineUrlRequestJob
from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5.QtCore import QUrl, QByteArray, QBuffer, QTimer

class Interceptor(QWebEngineUrlRequestInterceptor):
    def interceptRequest(self, info):
        url = info.requestUrl().toString()
        if info.requestMethod() == "GET" and not url.startswith("mm://"):
            info.redirect(QUrl(url.replace("https://", "mm://")))

class MMSchemeHandler(QWebEngineUrlSchemeHandler):
    def __init__(self, mm):
        super().__init__()
        self.mm = mm
    def requestStarted(self, job: QWebEngineUrlRequestJob):
        url = job.requestUrl().toString()
        print("Handling request for:", url)
        mimetype = QByteArray(b"text/html")
        resp = self.mm.getRequest(url.replace("mm://", "https://"))
        headers, body = resp.split("\r\n\r\n", 1)
        self._data = QByteArray(body.encode('utf-8'))
        buffer = QBuffer(job)
        buffer.setData(self._data)
        buffer.open(QBuffer.ReadOnly)
        job.reply(mimetype, buffer)

class Browser:
    def start(self, mm):
        app = QApplication([])
        self.view = QWebEngineView()
        interceptor = Interceptor()
        self.view.load(QUrl("https://example.com"))
        #window = QMainWindow()
        #window.setWindowTitle("Example")
        profile = self.view.page().profile()
        profile.setRequestInterceptor(interceptor)
        scheme_handler = MMSchemeHandler(mm)
        profile.installUrlSchemeHandler(b"mm", scheme_handler)
        self.view.show()
        #QTimer.singleShot(5000, lambda: self.view.load(QUrl("https://google.com")))
        app.exec_()
    def load(self, url):
        self.view.load(QUrl(url))

