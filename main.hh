#ifndef P2PAPP_MAIN_HH
#define P2PAPP_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>
#include <unistd.h>
#include <QDataStream>
#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QHostInfo>
#include <QtGlobal>
#include <QTimer>
#include <ctime>
#include <cstdlib>


class NetSocket : public QUdpSocket
{
    Q_OBJECT

    public:
        NetSocket();

        // Bind this socket to a P2Papp-specific default port.
        bool bind();

        int getminport();
        int getmaxport();
        int getmyport();

    private:
        int myPortMin, myPortMax, myPort;

};

class ChatDialog : public QDialog
{
    Q_OBJECT

    public:
        ChatDialog();
        void sendDgram(QByteArray);
        NetSocket *mySocket;
        int SeqNo;
        int remotePort; //port i receive from
        int neighbor; //port i send to
        QMap<QString, quint32> localWants;
        QVariantMap last_message;
        QMap<QString, QMap<quint32, QVariantMap> > messages_list;
        QTimer * timtoutTimer;
        QTimer * antientropyTimer;


    public slots:
        void gotReturnPressed();
        void readPendingDatagrams();
        void timeoutHandler();

    private:
        QTextEdit *textview;
        QLineEdit *textline;
        QMap<QString, quint32>* m_messageStatus;
        void processIncomingDatagram(QByteArray incomingBytes);
        void processStatus(QMap<QString, QMap<QString, quint32> > wants);
        void processMessage(QVariantMap wants);
        void sendStatus(QByteArray);
        void rumorMongering(QVariantMap messageMap);
        void addToMessageList(QVariantMap messageMap, quint32 origin, quint32 seqNo);
        QByteArray serialize(QString);
        QByteArray serializeStatus();
        void createMessageMap(QVariantMap * map, QString text);

};

#endif // P2PAPP_MAIN_HH
