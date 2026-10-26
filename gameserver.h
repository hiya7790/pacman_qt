#ifndef GAMESERVER_H
#define GAMESERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>

struct ConnectedClient {
    int id;
    QTcpSocket *socket;
    bool isWebSocket;
    QByteArray buffer;
    QString role;       // "p2", "p3", "p4", "blinky", "pinky", "inky", "clyde"
    QString playerName;
    int lastDir;
};

class GameServer : public QObject
{
    Q_OBJECT

public:
    explicit GameServer(QObject *parent = nullptr);
    ~GameServer();

    bool startServer(quint16 port = 8080);
    void stopServer();
    bool isRunning() const;
    QString getLocalIpAddress() const;
    quint16 getPort() const { return currentPort; }
    QString getServerUrl() const;
    int getConnectedClientCount() const;
    QList<ConnectedClient*> getActiveClients() const;
    bool isRoleTaken(const QString &role) const;
    QString getRolePlayerName(const QString &role) const;

    void broadcastGameState(const QJsonObject &state);
    void sendClientMessage(int clientId, const QJsonObject &obj);

signals:
    void playerJoined(int clientId, const QString &role, const QString &name);
    void playerLeft(int clientId, const QString &role, const QString &name);
    void playerMoved(int clientId, const QString &role, int dir);
    void playerTriggeredPeek(int clientId, const QString &role);
    void serverStatusChanged(bool running, const QString &url);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    void handleHttpRequest(ConnectedClient *client, const QByteArray &data);
    void handleWebSocketHandshake(ConnectedClient *client, const QString &request);
    void processWebSocketFrame(ConnectedClient *client);
    void handleWebSocketMessage(ConnectedClient *client, const QString &message);
    void sendWebSocketFrame(QTcpSocket *socket, const QString &text);
    QByteArray getControllerHtml() const;
    QJsonObject getRolesStatusJson() const;

    QTcpServer *tcpServer;
    quint16 currentPort;
    int nextClientId;
    QMap<int, ConnectedClient*> clients;
};

#endif // GAMESERVER_H
