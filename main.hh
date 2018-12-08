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
#include <vector>
#include <QStateMachine>
#include <QState>
#include <unordered_set>

using namespace std;


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
//        int to_be_voted;

        QMap<QString, quint32> localWants;
        QVariantMap last_message;
        QMap<QString, QMap<quint32, QVariantMap> > messages_list;
        QTimer * timtoutTimer;
        QTimer * electTimer;
        QTimer * antientropyTimer;
        QTimer * voteReqTimer;
        QTimer * heartbeattimer;

        QStateMachine rolemachine;
        QState *follower;
        QState *candidate;
        QState *leader;
        QState *stopped;




//        enum Role {FOLLOWER, CANDIDATE, LEADER};
//        Role role;
        int curleader = 0;
        int term;
        int votedFor = 0;
        bool voted = false;
//        int recterm;
        unordered_set<int> votedNodes;


        vector<int> participants;
        int numofvotes;
        int majority = 3;

        void sendVoteReq();

//        QVariantMap receivedVotes;

    public slots:
        void gotReturnPressed();
        void readPendDgrams();
//        void voteReqTimeoutHandler();
        void follwerHandler();
        void candidateHandler();
        void leaderHandler();
        void processVotes(QVariantMap receivedVotes);
        void govote(int recterm ,int cand);
        void broadcast();
        void stoppedHandler();
//        void timeoutHandler();

    signals:
        void gothigherterm();
        void gotthreevotes();
        void gotheartbeat();
        void gotvoterequest(int recterm, int cand);
        void gotvotes(QVariantMap receivedVotes);
        void gotstopsignal();
        void gotrestartsignal();


    private:
        QTextEdit *textview;
        QLineEdit *textline;
        QMap<QString, quint32>* m_messageStatus;
        void processIncomingDatagram(QByteArray incomingBytes);
        void processStatus(QMap<QString, QMap<QString, quint32> > wants);
        void processMessage(QVariantMap wants);
        void sendStatus(QByteArray);
        void rumorMongering(QVariantMap messageMap);
        void addMlist(QVariantMap messageMap, quint32 origin, quint32 seqNo);
        QByteArray serialize(QString);
        QByteArray serializeStatus();
        void createMessageMap(QVariantMap * map, QString text);

};

#endif // P2PAPP_MAIN_HH
