#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextBrowser>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QGridLayout>
#include <QUiLoader>
#include <QFile>
#include "server.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStringList>
#include "client.h"

class MainWindow : public QWidget {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    stackedWidget = new QStackedWidget(this);

    QWidget *ServerPage = loadUi(":/new/prefix1/ServerPage.ui");
    stackedWidget->addWidget(ServerPage);

    QWidget *ClientPage = loadUi(":/new/prefix1/ClientPage.ui");
    stackedWidget->addWidget(ClientPage);

    layout->addWidget(stackedWidget);

    QPushButton *button1 = ServerPage->findChild<QPushButton *>("pushButton_2");
    QPushButton *button2 = ServerPage->findChild<QPushButton *>("pushButton_3");
    QPushButton *button3 = ServerPage->findChild<QPushButton *>("pushButton_4");
    QLineEdit *lineedit = ServerPage->findChild<QLineEdit *>("lineEdit");
    ListUser = stackedWidget->widget(0)->findChild<QListWidget *>("listWidget");
    QObject::connect(ListUser, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        if(s!=nullptr){
            int index = ListUser->row(item);
            QString itemText = ListUser ->item(index)->text();

            if (windowInfo != nullptr) {
                windowInfo->close();
                delete windowInfo;
            }

            windowInfo = new QWidget();
            QGridLayout *layoutInfo = new QGridLayout(windowInfo);

            QLabel *warning = new QLabel("Вы уверены, что хотите отключить от сервера пользователя ", windowInfo);
            QLabel *info = new QLabel("Напишите 'ДА', если уверены: ", windowInfo);
            QLineEdit *answer = new QLineEdit(windowInfo);

            layoutInfo->addWidget(warning);
            layoutInfo->addWidget(info,0,1);
            layoutInfo->addWidget(answer,0,2);

            windowInfo->show();

            connect(answer, &QLineEdit::returnPressed, this, [this, answer, itemText]() {
                if(answer->text()=="ДА"){
                    s->DisconnectClient(itemText);
                }
                delete windowInfo;
                windowInfo = nullptr;
            });
        }
    });

    QPushButton *button = ClientPage->findChild<QPushButton *>("pushButton");
    QPushButton *connectToServer = ClientPage->findChild<QPushButton *>("pushButton_2");
    QPushButton *disconnectFromServer = ClientPage->findChild<QPushButton *>("pushButton_3");
    QLineEdit *messageFromClient = ClientPage->findChild<QLineEdit *>("lineEdit");
    QLineEdit *nameLineEdit = ClientPage->findChild<QLineEdit *>("lineEdit_2");


    connect(button1, &QPushButton::clicked, this, &MainWindow::showClientPage);
    connect(button2, &QPushButton::clicked, this, &MainWindow::StartServer);
    connect(button3, &QPushButton::clicked, this, &MainWindow::StopServer);
    connect(lineedit, &QLineEdit::returnPressed, this, [this, lineedit]() {
        if(s!=nullptr){
            s->AdminChat(lineedit->text());
        }
        lineedit->clear();
    });


    connect(button, &QPushButton::clicked, this, &MainWindow::showServerPage);
    connect(connectToServer, &QPushButton::clicked, this, &MainWindow::ConnectToServer);
    connect(disconnectFromServer, &QPushButton::clicked, this, &MainWindow::DisconnectFromServer);
    connect(messageFromClient, &QLineEdit::returnPressed, this, [this, messageFromClient]() {
        if(c!=nullptr){
            c->Chat(messageFromClient->text());
        }
        messageFromClient->clear();
    });
    connect(nameLineEdit, &QLineEdit::returnPressed, this, [this, nameLineEdit]() {
        if(c!=nullptr){
            c->Name = nameLineEdit->text();
        }
    });


    }
public slots:
    void StartServer(){
        if(s==nullptr){
            if (windowInfo != nullptr) {
                windowInfo->close();
                delete windowInfo;
            }

            windowInfo = new QWidget();
            QGridLayout *layoutInfo = new QGridLayout(windowInfo);

            QLabel *info = new QLabel("Порт:", windowInfo);
            QLineEdit *port = new QLineEdit(windowInfo);

            layoutInfo->addWidget(info,0,1);
            layoutInfo->addWidget(port,0,2);

            windowInfo->show();

            connect(port, &QLineEdit::returnPressed, this, [this, port]() {
                bool ok;
                int portNumber = port->text().toInt(&ok);

                if (ok) {
                    s = new Server();
                    connect(s, &Server::messageReceived, this, &MainWindow::appendMessageToLogs);
                    connect(s, &Server::messageToChat, this, &MainWindow::appendMessageToChat);
                    connect(s, &Server::messageUserList, this, &MainWindow::appendMessageToUserList);
                    connect(s, &Server::setUserList, this, &MainWindow::updateUserList);

                    s->Start(portNumber);
                    delete windowInfo;
                    windowInfo = nullptr;
                }
            });
        }
    }

    void StopServer(){\
        if(s!=nullptr){
            s->Close();
            delete s;
            s = nullptr;
        }
    }

    void ConnectToServer(){
        if(c==nullptr){
            if (windowInfo != nullptr) {
                windowInfo->close();
                delete windowInfo;
            }

            windowInfo = new QWidget();
            QGridLayout *layoutInfo = new QGridLayout(windowInfo);

            QLabel *info1 = new QLabel("IP адресс:", windowInfo);
            QLineEdit *IP = new QLineEdit(windowInfo);

            QLabel *info2 = new QLabel("Порт:", windowInfo);
            QLineEdit *port = new QLineEdit(windowInfo);

            layoutInfo->addWidget(info1,0,1);
            layoutInfo->addWidget(IP,0,2);
            layoutInfo->addWidget(info2,1,1);
            layoutInfo->addWidget(port,1,2);

            windowInfo->show();

            connect(port, &QLineEdit::returnPressed, this, [this, port, IP]() {
                bool ok;
                QString IPadress = IP->text();
                QRegularExpression ipRegex("^(\\d{1,3}\\.){3}\\d{1,3}$");
                QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);
                IP->setValidator(ipValidator);
                int portNumber = port->text().toInt(&ok);

                int pos = 0;
                if (ipValidator->validate(IPadress, pos) == QValidator::Acceptable) {
                    if (IPadress != "" && portNumber != 0) {
                        c = new Client(IPadress, portNumber);
                        connect(c, &Client::messageToClientChat, this, &MainWindow::appendMessageToClientChat);
                        delete windowInfo;
                        windowInfo = nullptr;
                    }
                }
            });
        }
    }

    void DisconnectFromServer(){
        if(c!=nullptr){
            c->disconnectFromServer();
            delete c;
            c = nullptr;
        }
    }


private slots:
    void showServerPage() {
        stackedWidget->setCurrentIndex(0);
    }

    void showClientPage() {
        stackedWidget->setCurrentIndex(1);
    }

    void appendMessageToLogs(const QString &message) {
        QTextBrowser *logs = stackedWidget->widget(0)->findChild<QTextBrowser *>("textBrowser");
        if (logs) {
            logs->append(message);
        }
    }

    void appendMessageToChat(const QString &message) {
        QTextBrowser *chat = stackedWidget->widget(0)->findChild<QTextBrowser *>("textBrowser_2");
        if (chat) {
            chat->append(message);
        }
    }

    void appendMessageToClientChat(const QString &message) {
        QTextBrowser *clientChat = stackedWidget->widget(1)->findChild<QTextBrowser *>("textBrowser_3");
        if (clientChat) {
            clientChat->append(message);
        }
    }

    void appendMessageToUserList(const QString &message){
        if (ListUser) {
            ListUser->addItem(message);
        }
    }

    void updateUserList(const QStringList &userList) {
        ListUser->clear();
        for (const QString &userId : userList) {
            ListUser->addItem(userId);
        }
    }

private:
    QStackedWidget *stackedWidget;

    QWidget *windowInfo = nullptr;

    Server *s = nullptr;
    Client *c = nullptr;
    QListWidget *ListUser;

    QWidget* loadUi(const QString &fileName) {
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly)) {
            return nullptr;
        }
        QUiLoader loader;
        QWidget *widget = loader.load(&file, this);
        file.close();
        return widget;
    }

};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.setFixedSize(625, 310);
    window.setWindowTitle("QtChat");
    window.show();
    return app.exec();
}

#include "main.moc"
