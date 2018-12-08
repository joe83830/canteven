#include "main.hh"
#include <iostream>

ChatDialog::ChatDialog()
{
    setWindowTitle("P2Papp");
    term = 0;
    // Read-only text box where we display messages from everyone.
    // This widget expands both horizontally and vertically.
    textview = new QTextEdit(this);
    textview->setReadOnly(true);
    // Small text-entry box the user can enter messages.
    // This widget normally expands only horizontally, leaving extra vertical space for the textview widget.
    // You might change this into a read/write QTextEdit, so that the user can easily enter multi-line messages.
    textline = new QLineEdit(this);

    // Lay out the widgets to appear in the main window.
    // For Qt widget and layout concepts see:  http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(textview);
    layout->addWidget(textline);
    setLayout(layout);

    follower = new QState();
    candidate = new QState();
    leader = new QState();
    stopped = new QState();

    electTimer = new QTimer(this);
    voteReqTimer = new QTimer(this);
    heartbeattimer = new QTimer(this);

    follower->addTransition(electTimer, SIGNAL(timeout()), candidate);
    follower->addTransition(this, SIGNAL(gotheartbeat()), follower);
    candidate->addTransition(voteReqTimer, SIGNAL(timeout()), follower);
    candidate->addTransition(this, SIGNAL(gotheartbeat()), follower);
    candidate->addTransition(this, SIGNAL(gothigherterm()), follower);
    candidate->addTransition(this, SIGNAL(gotthreevotes()), leader);
    leader->addTransition(this, SIGNAL(gotstopsignal()), stopped);
    stopped->addTransition(this, SIGNAL(gotrestartsignal()), follower);

    rolemachine.addState(follower);
    rolemachine.addState(candidate);
    rolemachine.addState(leader);
    rolemachine.addState(stopped);

    rolemachine.setInitialState(follower);
    rolemachine.start();

    qDebug() << "Printable";

    //bind mySocket
    mySocket = new NetSocket();
    if (!mySocket->bind())
        exit(1);

    for (int i = mySocket->getminport(); i <= mySocket->getmaxport(); i++) {
        if (i == mySocket->getmyport()) {
            continue;
        }
        participants.push_back(i);
    }

//    timtoutTimer = new QTimer(this);
//    connect(timtoutTimer, SIGNAL(timeout()), this, SLOT(timeoutHandler()));

//    int r = rand() % (301 - 150) + 150;

////    connect(electTimer, SIGNAL(timeout()), this, SLOT(electTimeoutHandler()));
//    electTimer->start(r);

    connect(follower, SIGNAL(entered()), this, SLOT(follwerHandler()));
    connect(candidate, SIGNAL(entered()), this, SLOT(candidateHandler()));
    connect(leader, SIGNAL(entered()), this, SLOT(leaderHandler()));
    connect(stopped, SIGNAL(entered()), this, SLOT(stoppedHandler()));


    m_messageStatus = new QMap<QString, quint32>;

    SeqNo = 0;
    connect(textline, SIGNAL(returnPressed()), this, SLOT(gotReturnPressed()));
    connect(mySocket, SIGNAL(readyRead()), this, SLOT(readPendDgrams()));

}

void ChatDialog::follwerHandler() {

    qDebug() << "In followerHandler now";
    if (rolemachine.configuration().contains(follower)) {
        qDebug() << "I'm a Follower";
    }

    int r = rand() % (3001 - 1500) + 1500;
    electTimer->start(r);

// Received signal and asked to vote
    connect(this, SIGNAL(gotvoterequest(int, int)), this, SLOT(govote(int, int)));

}

void ChatDialog::govote(int recterm, int cand) {

    electTimer->stop();
    qDebug() << "Asked to goVOTE";
    int r = rand() % (3001 - 1500) + 1500;
    electTimer->start(r);

    QVariantMap ballot;
    if (recterm < term) {
        ballot.insert("votefor", false);
    } else {
        qDebug() << "I VOTED!!!";
        ballot.insert("votefor", true);
        ballot.insert("id", mySocket->getmyport());
        term = recterm;
    }

    QByteArray data;
    QDataStream stream(&data, QIODevice::ReadWrite);
    stream << ballot;
    mySocket->writeDatagram(data, data.size(), QHostAddress("127.0.0.1"), cand);

}

void ChatDialog::candidateHandler() {

    // Received votes, store stuff in

    connect(this, SIGNAL(gotvotes(QVariantMap)), this, SLOT(processVotes(QVariantMap)));
    numofvotes = 1;
    term ++;

    if (rolemachine.configuration().contains(candidate)) {
        qDebug() << "I'm a candidate now!";
        qDebug() << term;
    }
    if (rolemachine.configuration().contains(follower)) {
        qDebug() << "I'm a Follower";
    }
    votedNodes.clear();
    voteReqTimer->start(500);
    sendVoteReq();

}

void ChatDialog::sendVoteReq() {
    qDebug() << "Sending Vote Request......";

    QVariantMap votereq;
    votereq.insert("candidate", mySocket->getmyport());
    votereq.insert("term", term);

    QByteArray data;
    QDataStream stream(&data, QIODevice::ReadWrite);
    stream << votereq;
    qDebug() << votereq;

//    QVariantMap test;
//    QDataStream inStream(&data, QIODevice::ReadOnly);
//    inStream >> test;
//    qDebug() << test;
//     send vote requests to other nodes
    for (int i = 0; i < 4; i++) {
        if (votedNodes.count(participants[i]) == 0) {
            qDebug() << "Loop send";
            qDebug() << participants[i];
            mySocket->writeDatagram(data, QHostAddress::LocalHost, participants[i]);
        }
    }
}

void ChatDialog::processVotes(QVariantMap receivedVotes) {
    qDebug() << "Processing votes";
    qDebug() << receivedVotes;
    qDebug() << numofvotes;
    if (!receivedVotes["votefor"].toBool()) {
        qDebug() << "Got false";
        emit gothigherterm();
    } else {
        qDebug() << "Got true";
//        qDebug() <<
        if (votedNodes.count(receivedVotes["id"].toInt()) == 0) {
            votedNodes.insert(receivedVotes["id"].toInt());
            qDebug() << "Added to voted nodes";
            numofvotes ++;
            qDebug() << "Numofvotes += 1";
            if (numofvotes >= 3) {
                emit gotthreevotes();
            }
            qDebug() << "Not enough votes yet";
        }
    }
    qDebug() << "Exited if-else";
}


void ChatDialog::leaderHandler() {
    qDebug() << "In leader handler";
    if (rolemachine.configuration().contains(leader)) {
        qDebug() << "I'm a FUCKING LEADER!!!!";
    }
    broadcast();
    qDebug() << "out of broadcast";
    heartbeattimer->start(300);
    qDebug() << "hbtimer started";
    connect(heartbeattimer, SIGNAL(timeout()), this, SLOT(broadcast()));
    qDebug() << "made it here";
}

void ChatDialog::broadcast() {
    qDebug() << "In Broadcast";
    QVariantMap content;
    content.insert("leader", mySocket->getmyport());
    content.insert("term", term);
    qDebug() << "Past first segment";

    QByteArray data;
    QDataStream stream(&data, QIODevice::ReadWrite);
    stream << content;
    qDebug() << "Past second segment";

    for (int i = 0; i < 4; i++) {
        mySocket->writeDatagram(data, data.size(), QHostAddress("127.0.0.1"), participants[i]);
    }
    qDebug() << "Past third segment";
}

void ChatDialog::stoppedHandler() {
    qDebug() << "In Stopped handler";
    heartbeattimer->stop();
    electTimer->stop();
    disconnect(this, SIGNAL(gotvoterequest(int, int)), this, SLOT(govote));
    // Disconnect gotvoterequest signal slot

}

//void ChatDialog::createMessageMap(QVariantMap * map, QString text) {
//    map->insert("ChatText", text);
//    map->insert("Origin", QString::number(mySocket->getmyport()));
//    map->insert("SeqNo",  SeqNo);
//}

//QByteArray ChatDialog::serialize(QString message_text) {

//    QVariantMap msgMap;

//    createMessageMap(&msgMap, message_text);
//    SeqNo += 1;

//    if(messages_list.contains(QString::number(mySocket->getmyport()))) {
//        messages_list[QString::number(mySocket->getmyport())].insert(msgMap.value("SeqNo").toUInt(), msgMap);
//    }
//    else {
//        QMap<quint32, QVariantMap> qvariantmap;
//        messages_list.insert(QString::number(mySocket->getmyport()), qvariantmap);
//        messages_list[QString::number(mySocket->getmyport())].insert(msgMap.value("SeqNo").toUInt(), msgMap);
//    }

//    QByteArray msgBarr;
//    QDataStream stream(&msgBarr,QIODevice::ReadWrite);
//    stream << msgMap;

//    return msgBarr;
//}

//QByteArray ChatDialog::serializeStatus() {
//    QMap<QString, QMap<QString, quint32> > statusMap;
//    statusMap.insert("Want", localWants);

//    QByteArray datagram;
//    QDataStream stream(&datagram,QIODevice::ReadWrite);
//    stream << statusMap;

//    return datagram;
//}

void ChatDialog::sendDgram(QByteArray datagram) {

    if (mySocket->getmyport() == mySocket->getmaxport()) {
        neighbor = mySocket->getmyport() - 1;

    } else if (mySocket->getmyport() == mySocket->getminport()) {
        neighbor = mySocket->getmyport() + 1;

    } else {
        srand(time(NULL));
        (rand() % 2 == 0) ?  neighbor = mySocket->getmyport() + 1: neighbor = mySocket->getmyport() - 1;
    }

    mySocket->writeDatagram(datagram, datagram.size(), QHostAddress("127.0.0.1"), neighbor);
    int r = rand() % (3001 - 1500) + 1500;
    timtoutTimer->start(r);
}

//void ChatDialog::sendStatus(QByteArray datagram)
//{
//    mySocket->writeDatagram(datagram.data(), datagram.size(), QHostAddress("127.0.0.1"), remotePort);
//    int r = rand() % (301 - 150) + 150;
//    timtoutTimer->start(r);
//}


//void ChatDialog::rumorMongering(QVariantMap messageMap){

//    QByteArray rumorBytes;
//    QDataStream stream(&rumorBytes,QIODevice::ReadWrite);

//    stream << messageMap;
//    sendDgram(rumorBytes);

//}

void ChatDialog::readPendDgrams()
{
    while (mySocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(mySocket->pendingDatagramSize());
        QHostAddress senderAddress;
        quint16 senderPort;

        mySocket->readDatagram(datagram.data(), datagram.size(), &senderAddress,  &senderPort);
        remotePort = senderPort;
        processIncomingDatagram(datagram);
    }
}

void ChatDialog::processIncomingDatagram(QByteArray incomingBytes)
{
    qDebug() << "Received <Something>";
    QVariantMap messageMap;
    QDataStream serializer(&incomingBytes, QIODevice::ReadOnly);
    serializer >> messageMap;
    qDebug() << serializer.status();
//    if (serializer.status() != QDataStream::Ok) {
//        return;
//    }
    qDebug() << "Still alive";
    qDebug() << messageMap;

    if (messageMap.contains("leader")) {
        curleader = messageMap["leader"].toInt();
        term = messageMap["term"].toInt();
        qDebug() << "Got heartbeat, I respect my leader";
        if (rolemachine.configuration().contains(follower)) {
            qDebug() << "I'm a sheep that follows guidance";
        }
        emit gotheartbeat();
    } else if (messageMap.contains("candidate")) {
//        recterm = messageMap["term"].toInt();
//        to_be_voted = messageMap["candidate"].toInt();
        qDebug() << "<Something> is vote request";
        emit gotvoterequest(messageMap["term"].toInt(), messageMap["candidate"].toInt());
    } else if (messageMap.contains("votefor")) {
        qDebug() << "<Something is a vote>";
//        receivedVotes = messageMap;
        emit gotvotes(messageMap);
    }

//    QMap<QString, quint32> statusMap;
//    QDataStream stream(&incomingBytes, QIODevice::ReadOnly);
//    stream >> statusMap;
//    if (messageMap.contains("Want")) {
//        if (statusMap.isEmpty()) {
//            return;
//        }

//        processStatus(statusMap);
//    } else if(messageMap.contains("ChatText")){
//         processMessage(messageMap);
//    } else {
//        return;
//    }
}

//void ChatDialog::processMessage(QVariantMap messageMap){

//    quint32 origin = messageMap.value("Origin").toUInt();
//    quint32 seqNo = messageMap.value("SeqNo").toUInt();

//    if(mySocket->getmyport() != origin) {
//        if(localWants.contains(QString::number(origin))) {\
//            if (seqNo == localWants.value(QString::number(origin))) {
//                 addMlist(messageMap, origin, seqNo);
//                 localWants[QString::number(origin)] = seqNo + 1;
//            }
//        } else {
//            localWants.insert(QString::number(origin), seqNo+1);
//            addMlist(messageMap, origin, seqNo);
//        }
//    } else {

//        if(localWants.contains(QString::number(origin))) {
//            localWants[QString::number(origin)] = seqNo + 1;
//        } else {
//            localWants.insert(QString::number(origin), seqNo+1);
//        }
//    }

//    timtoutTimer->stop();
//    sendStatus(serializeStatus());
//}


//void ChatDialog::addMlist(QVariantMap messageMap, quint32 origin, quint32 seqNo){

//    if(messages_list.contains(QString::number(origin))) {
//        messages_list[QString::number(origin)].insert(seqNo, messageMap);
//    }
//    else {
//        QMap<quint32, QMap<QString, QVariant> > qMap;
//        messages_list.insert(QString::number(origin), qMap);
//        messages_list[QString::number(origin)].insert(seqNo, messageMap);
//    }

//    textview->append(QString::number(origin) + ": " + messageMap.value("ChatText").toString());
//    rumorMongering(messageMap);
//    last_message = messageMap;

//}


//void ChatDialog::processStatus(QMap<QString, QMap<QString, quint32> > receivedStatusMap)
//{

//    QMap<QString, QVariant> rumorMapToSend;
//    QMap<QString, quint32> remwant = receivedStatusMap["Want"];

//    enum Status { SYNCED = 1, AHEAD = 2 , BEHIND = 3 };
//    Status status =  SYNCED;

//    QMap<QString, quint32>::const_iterator localIter = localWants.constBegin();

//    while (localIter != localWants.constEnd()){
//        if(!remwant.contains(localIter.key())){
//            status = AHEAD;
//            rumorMapToSend = messages_list[localIter.key()][quint32(0)];
//        } else if(remwant[localIter.key()] < localWants[localIter.key()]) {
//            status = AHEAD;
//            rumorMapToSend = messages_list[localIter.key()][remwant[localIter.key()]];
//        }
//        else if(remwant[localIter.key()] > localWants[localIter.key()]){
//            status = BEHIND;
//        }
//        ++localIter;
//    }

//    QMap<QString, quint32>::const_iterator remoteIter = remwant.constBegin();
//    while (remoteIter != remwant.constEnd()){
//        if(!localWants.contains(remoteIter.key())) {
//            status = BEHIND;
//        }
//        ++remoteIter;
//    }
//    timtoutTimer->stop();

//    QByteArray rumorByteArray;
//    QDataStream * stream = new QDataStream(&rumorByteArray, QIODevice::ReadWrite);
//    (*stream) << rumorMapToSend;
//    delete stream;

//    switch(status) {
//        case AHEAD:
//            mySocket->writeDatagram(rumorByteArray, QHostAddress::LocalHost, remotePort);
//            break;
//        case BEHIND:
//            sendStatus(serializeStatus());
//            break;
//        case SYNCED:
//            if(qrand() > .5*RAND_MAX) {
//                rumorMongering(last_message);
//            }
//            break;
//    }
//}


void ChatDialog::gotReturnPressed()
{
    // Just echo the string locally.
    qDebug() << "INFO: Entered gotReturnPressed(). Message Sending: " << textline->text();
    textview->append(QString::number(mySocket->getmyport()) + ": " + textline->text());

    QString input = textline->text();
    qDebug() << input;
    if (input == "STOP") {
        emit gotstopsignal();
    }

    if (input == "RESTART") {
        emit gotrestartsignal();
    }
//    QByteArray message = serialize(input);

//    if(localWants.contains(QString::number(mySocket->getmyport()))) {
//        localWants[QString::number(mySocket->getmyport())] += 1;
//    }
//    else {
//        localWants.insert(QString::number(mySocket->getmyport()), 1);
//    }

//    sendDgram(message);
//    textline->clear();
}

//void ChatDialog::timeoutHandler() {
//    qDebug() << "Timeout.";
//    QByteArray data;
//    QDataStream stream(&data, QIODevice::ReadWrite);
//    stream << last_message;
//    mySocket->writeDatagram(data, data.size(), QHostAddress("127.0.0.1"), neighbor);

//    int r = rand() % (301 - 150) + 150;
//    timtoutTimer->start(r);
//}

//constructing NetSocket Class
NetSocket::NetSocket()
{
    // Pick a range of four UDP ports to try to allocate by default, computed based on my Unix user ID.
    // This makes it trivial for up to four P2Papp instances per user to find each other on the same host, barring UDP port conflicts with other applications (which are quite possible).
    // We use the range from 32768 to 49151 for this purpose.
    myPortMin = 32768 + (getuid() % 4096)*4;
    myPortMax = myPortMin + 4;
}

bool NetSocket::bind()
{
    // Try to bind to each of the range myPortMin..myPortMax in turn.
    for (int p = myPortMin; p <= myPortMax; p++) {
        if (QUdpSocket::bind(p)) {
            qDebug() << "INFO: bound to UDP port " << p;
            //store myPort number
            myPort = p;

           return true;
        }
    }
    qDebug() << "ERROR: No ports avaialble in the default range " << myPortMin << "-" << myPortMax << " available";
    return false;
}

int NetSocket::getmaxport() {
    return myPortMax;
}

int NetSocket::getminport() {
    return myPortMin;
}

int NetSocket::getmyport() {
    return myPort;
}

//main function
int main(int argc, char **argv)
{
    printf("AAAAAAAaa");
    // Initialize Qt toolkit
    QApplication app(argc,argv);

    // Create an initial chat dialog window
    ChatDialog dialog;
    dialog.show();
//    printf("AAAAAAAaa");

    // Enter the Qt main loop; everything else is event driven
    return app.exec();

}
