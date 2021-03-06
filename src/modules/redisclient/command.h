#pragma once

#include <QtCore>
#include <functional>

namespace RedisClient {

class Response;

class Command 
{    
public:
    Command();
    Command(const QString& cmdString, QObject * owner = nullptr, int db = -1);
    Command(const QString& cmdString, QObject * owner, const QString& invokeMethod, int db = -1);
    Command(const QStringList& cmd, QObject * owner, const QString& invokeMethod, int db = -1);
    Command(const QStringList& cmd, QObject * owner = nullptr, int db = -1);           
    Command(const QStringList& cmd, QObject * owner, std::function<void(Response)> callback, int db = -1);
    Command(const QStringList& cmd, int db);
    Command(int db);

    Command &operator <<(const QString&);

    /** @see http://redis.io/topics/protocol for more info **/    
    QByteArray  getByteRepresentation() const;
    QString     getRawString() const;
    QStringList getSplitedRepresentattion() const;
    QString     getCallbackName();
    QString     getProgressCallbackName();
    int         getDbIndex() const;
    QObject *   getOwner() const;

    void setOwner(QObject *);      
    void setCallBackName(const QString &);        
    void setProgressCallBackName(const QString &);

    /** New callback API **/
    void setCallBack(QObject* context, std::function<void(Response)> callback);
    std::function<void(Response)> getCallBack();

    void cancel();

    bool isCanceled() const;
    bool isValid() const;
    bool hasCallback() const;
    bool isEmpty() const;
    bool hasDbIndex() const;
    bool isSelectCommand() const;

private:
    QObject * owner;
    QStringList commandWithArguments;
    int dbIndex;
    QString callBackMethod; // method(Response)
    QString progressMethod; // method(unsigned int)
    bool commandCanceled;
    std::function<void(Response)> m_callback;

    QStringList splitCommandString(const QString &);
};

}
