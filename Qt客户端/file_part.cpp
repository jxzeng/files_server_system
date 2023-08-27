//fileclient.cpp
#include "file_part.h"

#include <QDataStream>
#include <QHostAddress>
#include <QFile>
#include <QCryptographicHash>
#include <QVariant>
#include <QFile>
#include <QFileInfo>

FileClient::FileClient(QString ip,quint16 port,QObject *parent) :
    QObject(parent),
    m_ip(ip),
    m_port(port),
    m_socket(this)
{

}

FileClient::~FileClient()
{
    if(this->m_socket.isOpen())
    {
        this->m_socket.close();
    }
}

qint64 FileClient::filesUpload(QStringList filepath_list)
{
    qint64 status=OK;
    foreach (QString filepath, filepath_list)
    {
        status=this->fileUpload(filepath);
        if(status!=OK)
        {
            return status;
        }
    }

    return status;
}

qint64 FileClient::fileUpload(QString filepath)
{
    qint64 status=OK;
    this->paramInit();

    status=this->fileUploadHeader(filepath);
    if(status==FILE_ALREADY_EXISTS_ERROR) //already exist
    {
        return OK;
    }
    else if(status!=OK)
    {
        this->m_socket.disconnectFromHost();
        return status;
    }

    status=this->fileUploadPos();
    if(status!=OK)
    {
        this->m_socket.disconnectFromHost();
        return status;
    }

    status=this->fileUploadContent();
    if(status!=OK)
    {
        this->m_socket.disconnectFromHost();
        return status;
    }

    status=this->fileUploadTail();
    if(status!=OK)
    {
        this->m_socket.disconnectFromHost();
        return status;
    }

    return status;
}

qint64 FileClient::filesDownload(QStringList filepath_list)
{
    qint64 status=OK;
    foreach (QString filepath, filepath_list)
    {
        status=this->fileDownload(filepath);
        if(status!=OK)
        {
            return status;
        }
    }

    return status;
}

qint64 FileClient::fileDownload(QString filepath)
{
    qint64 status=OK;
    this->paramInit();
    status=this->fileDownloadHeader(filepath);
    if(status==FILE_ALREADY_EXISTS_ERROR) //already exists
    {
        return OK;
    }
    else if(status!=OK)
    {
        this->m_socket.disconnectFromHost();
        return status;
    }

    status=this->fileDownloadContent();
    if(status!=OK)
    {
        this->m_socket.disconnectFromHost();
        return status;
    }

    status=this->fileDownloadTail();
    if(status!=OK)
    {
        this->m_socket.disconnectFromHost();
        return status;
    }

    return status;
}

qint64 FileClient::fileUploadRecvData(qint64 req_id, QMap<QString, QVariant>& recv_data)
{
    if(this->m_socket.waitForReadyRead()==false)
    {
        return RECV_DATA_TIMEOUT_ERROR;
    }

    qint64 recv_rsp_id=OK;
    QByteArray inblock=this->m_socket.readAll();
    QDataStream rsp_in(&inblock,QIODevice::ReadOnly);
    rsp_in>>recv_rsp_id>>recv_data;

    if(req_id!=recv_rsp_id)
    {
        return RECV_UNKNOW_DATA_ERROR;
    }

    return qint64(recv_data[QString("reply_status")].toInt());
}

qint64 FileClient::fileUploadHeader(QString filepath)
{
    if(this->m_socket.isOpen()==false)
    {
        if(this->socketConnect()==false)
        {
            qDebug()<<"socket connect failed:"<<this->m_socket.errorString()<<endl;
            return CONNECT_SERVER_ERROR;
        }
    }

    this->m_local_file.setFileName(filepath);
    if(this->m_local_file.open(QIODevice::ReadOnly)==false)
    {
        qDebug()<<"read file failed:"<<this->m_local_file.errorString()<<endl;
        return FILE_OPEN_ERROR;
    }
    this->m_file_total_size=this->m_local_file.size();
    this->m_local_file.close();

    QByteArray outblock;
    QDataStream file_out(&outblock,QIODevice::WriteOnly);
    QMap<QString,QVariant> file_header;
    file_header[QString("file_prjpath")]=QVariant(filepath.right(filepath.size()-filepath.lastIndexOf('/')-1));
    file_out<<qint64(FILE_UPLOAD_HEADER)<<file_header;

    this->m_socket.write(outblock);
    this->m_socket.waitForBytesWritten();

    QMap<QString, QVariant> rsp_msg;
    qint64 recv_status=this->fileUploadRecvData(FILE_UPLOAD_HEADER,rsp_msg);
    if(recv_status!=OK)
    {
        return recv_status;
    }
    this->m_file_pos=rsp_msg[QString("file_pos")].toInt();
    QString recv_md5=rsp_msg[QString("file_md5")].toString();

    if(this->m_file_pos==0)
    {
        return OK;
    }
    else
    {
        if(this->m_local_file.open(QIODevice::ReadOnly)==false)
        {
            return FILE_OPEN_ERROR;
        }

        qint64 readtotalBytes=0;
        qint64 payloadSize=10*1024*1024;
        QCryptographicHash clc_md5(QCryptographicHash::Md5);
        while(readtotalBytes<this->m_file_pos)
        {
            qint64 readBytes=qMin(payloadSize,this->m_file_pos-readtotalBytes);
            clc_md5.addData(this->m_local_file.read(readBytes));
            readtotalBytes+=readBytes;
        }
        this->m_local_file.close();

        if(QString(clc_md5.result().toHex())!=recv_md5)
        {
            this->m_file_pos=0;
        }

        this->m_file_total_size-=this->m_file_pos;
        if(this->m_file_total_size==0)
        {
            return FILE_ALREADY_EXISTS_ERROR;
        }
        else
        {
            return OK;
        }

    }
}

qint64 FileClient::fileUploadPos()
{
    QByteArray outblock;
    QDataStream file_out(&outblock,QIODevice::WriteOnly);
    QMap<QString,QVariant> file_header;
    file_header[QString("file_pos")]=QVariant(this->m_file_pos);
    file_header[QString("file_size")]=QVariant(this->m_file_total_size);
    file_out<<qint64(FILE_UPLOAD_POS)<<file_header;

    this->m_socket.write(outblock);
    this->m_socket.waitForBytesWritten();

    QMap<QString, QVariant> rsp_msg;
    qint64 recv_status=this->fileUploadRecvData(FILE_UPLOAD_POS,rsp_msg);

    return recv_status;
}

qint64 FileClient::fileUploadContent()
{
    if(this->m_socket.isOpen()==false)
    {
        return CONNECT_SERVER_ERROR;
    }

    if(this->m_local_file.open(QIODevice::ReadOnly)==false)
    {
        return FILE_OPEN_ERROR;
    }
    this->m_local_file.seek(this->m_file_pos);

    while(this->m_file_total_size!=0)
    {
        this->m_file_total_size-=this->m_socket.write(this->m_local_file.read(qMin(this->m_file_total_size,this->m_payloadSize)));
        this->m_socket.waitForBytesWritten();
    }

    return OK;
}

qint64 FileClient::fileUploadTail()
{
    if(this->m_socket.isOpen()==false)
    {
        return CONNECT_SERVER_ERROR;
    }

    QMap<QString, QVariant> rsp_msg;
    qint64 recv_status=this->fileUploadRecvData(FILE_UPLOAD_TAIL,rsp_msg);
    return recv_status;
}

qint64 FileClient::fileDownloadHeader(QString filepath)
{
    if(this->m_socket.isOpen()==false)
    {
        if(this->socketConnect()==false)
        {
            qDebug()<<"connect server error"<<endl;
            return CONNECT_SERVER_ERROR;
        }
    }

    qint64 file_pos=0;
    QString file_md5_value;
    this->m_local_file.setFileName(filepath);
    if(this->m_local_file.open(QIODevice::ReadOnly)==false)
    {
        file_pos=0;
    }
    else
    {
        file_pos=this->m_local_file.size();
        qint64 readtotalBytes=0;
        qint64 payloadSize=10*1024*1024;
        QCryptographicHash clc_md5(QCryptographicHash::Md5);
        while(readtotalBytes<file_pos)
        {
            qint64 readBytes=qMin(payloadSize,file_pos-readtotalBytes);
            clc_md5.addData(this->m_local_file.read(readBytes));
            readtotalBytes+=readBytes;
        }
        file_md5_value=QString(clc_md5.result().toHex());
        this->m_local_file.close();
    }

    QByteArray outblock;
    QDataStream file_out(&outblock,QIODevice::WriteOnly);
    QMap<QString,QVariant> file_header;
    file_header[QString("file_prjpath")]=QVariant(filepath);
    file_header[QString("file_pos")]=QVariant(file_pos);
    file_header[QString("file_md5")]=QVariant(file_md5_value);
    file_out<<qint64(FILE_DOWNLOAD_HEADER)<<file_header;

    this->m_socket.write(outblock);
    this->m_socket.waitForBytesWritten();

    QMap<QString, QVariant> rsp_msg;
    qint64 recv_status=this->fileUploadRecvData(FILE_DOWNLOAD_HEADER,rsp_msg);
    if(recv_status!=OK)
    {
        return recv_status;
    }

    this->m_file_pos=rsp_msg[QString("file_pos")].toInt();
    this->m_file_total_size=rsp_msg[QString("file_size")].toInt();

    if(0==this->m_file_total_size)
    {
        qDebug()<<"file already exist error"<<endl;
        return FILE_ALREADY_EXISTS_ERROR;
    }

    QByteArray outblock2;
    QDataStream file_out2(&outblock2,QIODevice::WriteOnly);
    QMap<QString,QVariant> file_header2;
    file_header2[QString("reply_status")]=QVariant(qint64(OK));
    file_out2<<qint64(FILE_DOWNLOAD_CONTENT)<<file_header2;

    this->m_socket.write(outblock2);
    this->m_socket.waitForBytesWritten();

    return OK;
}

qint64 FileClient::fileDownloadContent()
{
    if(this->m_local_file.open(QIODevice::WriteOnly)==false)
    {
        qDebug()<<"file open error:"<<this->m_local_file.fileName()<<endl;
        return FILE_OPEN_ERROR;
    }
    this->m_local_file.seek(this->m_file_pos);

    qint64 status=OK;
    while(this->m_file_total_size>0)
    {
        if(this->m_socket.waitForReadyRead()==false)
        {
            status=RECV_DATA_TIMEOUT_ERROR;
            break;
        }
        QByteArray inblock=this->m_socket.readAll();
        this->m_file_total_size-=inblock.size();
        if(status==OK)
        {
            if(this->m_local_file.write(inblock)==-1)
            {
                status=FILE_WRITE_ERROR;
            }
        }
    }
    this->m_local_file.close();

    if(status==OK)
    {
        QByteArray outblock2;
        QDataStream file_out2(&outblock2,QIODevice::WriteOnly);
        QMap<QString,QVariant> file_header2;
        file_header2[QString("reply_status")]=QVariant(qint64(OK));
        file_out2<<qint64(FILE_DOWNLOAD_TAIL)<<file_header2;

        this->m_socket.write(outblock2);
        this->m_socket.waitForBytesWritten();
    }

    return status;
}

qint64 FileClient::fileDownloadTail()
{
    QMap<QString, QVariant> rsp_msg;
    qint64 recv_status=this->fileUploadRecvData(FILE_DOWNLOAD_TAIL,rsp_msg);

    return recv_status;
}

bool FileClient::socketConnect()
{
    this->m_socket.close();

    this->m_socket.connectToHost(QHostAddress(this->m_ip),this->m_port);
    if(this->m_socket.waitForConnected()==false)
    {
        qDebug()<<"connect timeout:"<<this->m_ip<<this->m_port<<endl;
        return false;
    }

    return true;
}

void FileClient::paramInit()
{
    this->m_file_pos=0;
    this->m_payloadSize=1*1024*1024;//1MB
    this->m_local_file.close();
    this->m_file_total_size=0;
}
