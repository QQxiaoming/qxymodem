#ifndef QXMODEM_H
#define QXMODEM_H

#include <QString>
#include <QFile>
#include <QMutex>
#include <QThread>
#include <QDebug>

class QXmodem: public QThread {
    Q_OBJECT

public:
    explicit QXmodem(unsigned short sendPktSize = 128, int timeout = 1000, int retry_limit = 16, bool no_timeout = true,QObject *parent = nullptr):
        QThread(parent),m_sendPktSize(sendPktSize),m_timeout(timeout),m_retry_limit(retry_limit),m_no_timeout(no_timeout) {};
    ~QXmodem(){};

    enum {
        SEND,
        RECV
    };
    
    /* xmodem control characters */
    enum {
        SOH	  = 0x01,
        STX	  = 0x02,
        EOT	  = 0x04,
        ACK	  = 0x06,
        NAK	  = 0x15,
        CAN	  = 0x18,
        CTRLZ = 0x1A,
    };

    /* error return codes */
    enum {
        XMODEM_ERROR_REMOTECANCEL = -1,
        XMODEM_ERROR_OUTOFSYNC	  = -2,
        XMODEM_ERROR_RETRYEXCEED  = -3,
    };

    void startSend(void) {
        dir=SEND;
        start();
    }

    void startRecv(void) {
        dir=RECV;
        start();
    }

protected:
    void run() {
        _start();
        switch(dir) {
            case SEND:
                xmodemTransmit(m_sendPktSize);
                break;
            case RECV:
                xmodemReceive();
                break;
        }
        _end();
    }

private:
    virtual void _start(void) = 0;
    virtual void _end(void) = 0;

    virtual int writefile(const char* buffer, int size) = 0;
    virtual int readfile(char* buffer, int size) = 0;
    virtual int flushfile(void) = 0;

    virtual int sendStream(const char* buffer, int size) = 0;
    virtual int receiveStream(const char* buffer, int size) = 0;

    virtual void timerPause(int t) {
        Q_UNUSED(t);
    }

    void xmodemOut(unsigned char c) {
        sendStream((const char*)&c,1);
    }

    int xmodemIn(unsigned char *c) {
        return receiveStream((const char*)c,1);
    }    
    
    uint16_t crc_xmodem_update(uint16_t crc, uint8_t data);
    long xmodemReceive(void);
    long xmodemTransmit(unsigned short pktsize = 128);
    int xmodemCrcCheck(int crcflag, const unsigned char *buffer, int size);
    int xmodemInTime(unsigned char *c, unsigned short timeout);
    void xmodemInFlush(void);

private:
    int dir=SEND;

    /* xmodem parameters */
    unsigned short m_sendPktSize = 128;
    int m_timeout = 1000;
    int m_retry_limit = 16;
    bool m_no_timeout = true;
};

class QXmodemFile: public QXmodem {
    Q_OBJECT

public:
    QXmodemFile(QString filename,unsigned short sendPktSize = 128, int timeout = 1000, int retry_limit = 16, bool no_timeout = true, QObject *parent = nullptr) :
        QXmodem(sendPktSize,timeout,retry_limit,no_timeout,parent)
    {
        m_file = new QFile(filename);
    }
    QXmodemFile(const char *filename,unsigned short sendPktSize = 128, int timeout = 1000, int retry_limit = 16, bool no_timeout = true,QObject *parent = nullptr) :
        QXmodem(sendPktSize,timeout,retry_limit,no_timeout,parent)
    {
        m_file = new QFile(QString(filename));
    }
    QXmodemFile(QString filename, QObject *parent = nullptr) :
        QXmodem(128,1000,16,true,parent)
    {
        m_file = new QFile(filename);
    }
    QXmodemFile(const char *filename, QObject *parent = nullptr) :
        QXmodem(128,1000,16,true,parent)
    {
        m_file = new QFile(QString(filename));
    }
    ~QXmodemFile(){
        delete m_file;
    }
    
signals:
    void send(QByteArray ba);

public slots:
    void receive(QByteArray ba) {
        m_mutex.lock();
        cache.append(ba);
        m_mutex.unlock();
    }

private:
    void _start(void) {
        m_file->open(QIODevice::ReadWrite);
    }

    void _end(void) {
        m_file->close();
    }

    int writefile(const char* buffer, int size) {
        return m_file->write(buffer,size);
    }
    int readfile(char* buffer, int size) {
        int r = m_file->read(buffer,size);
        return r;
    }
    int flushfile(void)
    {
        return m_file->flush();
    }
    int sendStream(const char* buffer, int size) {
        emit send(QByteArray(buffer,size));
        return size;
    }
    int receiveStream(const char* buffer, int size) {
        m_mutex.lock();
        int ret = qMin(size,cache.size());
        memcpy((void*)buffer,cache.data(),ret);
        cache.remove(0,ret);
        m_mutex.unlock();
        return ret;
    }
    void timerPause(int t) {
        QThread::msleep(t);
    }

private:
    QFile *m_file = nullptr;
    QMutex m_mutex;
    QByteArray cache;
};

#endif /* QXMODEM_H */ 
