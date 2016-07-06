/*
This file is part of pipesocks. Pipesocks is a pipe-like SOCKS5 tunnel system.
Copyright (C) 2016  yvbbrjdr

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pump.h"

Pump::Pump(qintptr handle,const QString &Password,QObject *parent):QObject(parent),Password(Password) {
    csock=new SecureSocket(this);
    connect(csock,SIGNAL(RecvData(QByteArray)),this,SLOT(ClientRecv(QByteArray)));
    connect(csock,SIGNAL(disconnected()),this,SLOT(EndSession()));
    csock->setSocketDescriptor(handle);
    ssock=NULL;
    status=Initiated;
    printf("[%s] New connection from %s:%d\n",QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLocal8Bit().data(),csock->peerAddress().toString().toLocal8Bit().data(),csock->peerPort());
}

void Pump::ClientRecv(const QByteArray &Data) {
    QVariantMap qvm(QJsonDocument::fromJson(Data).toVariant().toMap());
    switch (status) {
        case Initiated:
            if (qvm["password"]!=Password) {
                csock->disconnectFromHost();
            }
            ssock=new TcpSocket(this);
            connect(ssock,SIGNAL(RecvData(QByteArray)),this,SLOT(ServerRecv(QByteArray)));
            connect(ssock,SIGNAL(disconnected()),this,SLOT(EndSession()));
            ssock->connectToHost(qvm["host"].toString(),qvm["port"].toUInt());
            status=Connected;
            break;
        case Connected:
            emit ssock->SendData(Data);
            break;
    }
}

void Pump::ServerRecv(const QByteArray &Data) {
    emit csock->SendData(Data);
}

void Pump::EndSession() {
    if (ssock) {
        disconnect(ssock,SIGNAL(RecvData(QByteArray)),this,SLOT(ServerRecv(QByteArray)));
        disconnect(ssock,SIGNAL(disconnected()),this,SLOT(EndSession()));
        ssock->disconnectFromHost();
    }
    disconnect(csock,SIGNAL(RecvData(QByteArray)),this,SLOT(ClientRecv(QByteArray)));
    disconnect(csock,SIGNAL(disconnected()),this,SLOT(EndSession()));
    csock->disconnectFromHost();
    deleteLater();
}
