#ifndef CONSUMER_H
#define CONSUMER_H
#include <QThread>
#include <QSemaphore>
#include "uploadtask.h"
#include<QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include "common.h"
#include <QHttpMultiPart>
#include <QHttpPart>
#include<QMessageBox>

class consumer:public QThread
{
    Q_OBJECT
public:
    consumer();

private:
    QNetworkAccessManager* m_man;
    Common m_cm;
signals:
    void datapro(DataProgress* dp,int value, int max);
    void warm_(QString str);
    void layout(UploadFileInfo* info);
protected:
    void run() override;
    void uploadFile(UploadFileInfo *info);
};

#endif // CONSUMER_H
