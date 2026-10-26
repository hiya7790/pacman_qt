#include "gameserver.h"
#include <QNetworkInterface>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <cstdint>

GameServer::GameServer(QObject *parent)
    : QObject(parent)
    , tcpServer(nullptr)
    , currentPort(8080)
    , nextClientId(1)
    , m_hostRole("p1")
{
}

GameServer::~GameServer()
{
    stopServer();
}

bool GameServer::startServer(quint16 port)
{
    stopServer();
    currentPort = port;
    tcpServer = new QTcpServer(this);

    // Attempt to listen on specified port, or try subsequent ports if already bound
    bool listening = false;
    for (int attempts = 0; attempts < 5; ++attempts) {
        if (tcpServer->listen(QHostAddress::Any, currentPort)) {
            listening = true;
            break;
        }
        currentPort++;
    }

    if (!listening) {
        qWarning() << "Failed to start GameServer on ports 8080-8084:" << tcpServer->errorString();
        delete tcpServer;
        tcpServer = nullptr;
        emit serverStatusChanged(false, "");
        return false;
    }

    connect(tcpServer, &QTcpServer::newConnection, this, &GameServer::onNewConnection);
    emit serverStatusChanged(true, getServerUrl());
    return true;
}

void GameServer::stopServer()
{
    if (tcpServer) {
        for (ConnectedClient *client : clients) {
            if (client->socket) {
                client->socket->disconnectFromHost();
                client->socket->deleteLater();
            }
            delete client;
        }
        clients.clear();
        tcpServer->close();
        tcpServer->deleteLater();
        tcpServer = nullptr;
        emit serverStatusChanged(false, "");
    }
}

bool GameServer::isRunning() const
{
    return tcpServer != nullptr && tcpServer->isListening();
}

QString GameServer::getLocalIpAddress() const
{
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : addresses) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
            !addr.isLoopback() &&
            !addr.isLinkLocal()) {
            return addr.toString();
        }
    }
    return "127.0.0.1";
}

QString GameServer::getServerUrl() const
{
    if (!isRunning()) return "";
    return QString("http://%1:%2").arg(getLocalIpAddress()).arg(currentPort);
}

int GameServer::getConnectedClientCount() const
{
    return clients.size();
}

QList<ConnectedClient*> GameServer::getActiveClients() const
{
    return clients.values();
}

bool GameServer::isRoleTaken(const QString &role) const
{
    if (role == m_hostRole) return true;
    for (ConnectedClient *c : clients) {
        if (c->role == role) return true;
    }
    return false;
}

QString GameServer::getRolePlayerName(const QString &role) const
{
    if (role == m_hostRole) return "Host (PC)";
    for (ConnectedClient *c : clients) {
        if (c->role == role) return c->playerName;
    }
    return "";
}

void GameServer::setHostRole(const QString &role)
{
    m_hostRole = role;
    QJsonObject rolesMsg;
    rolesMsg["type"] = "rolesUpdate";
    rolesMsg["roles"] = getRolesStatusJson();
    broadcastGameState(rolesMsg);
}

void GameServer::onNewConnection()
{
    if (!tcpServer) return;
    while (tcpServer->hasPendingConnections()) {
        QTcpSocket *socket = tcpServer->nextPendingConnection();
        ConnectedClient *client = new ConnectedClient();
        client->id = nextClientId++;
        client->socket = socket;
        client->isWebSocket = false;
        client->lastDir = -1;

        clients.insert(client->id, client);

        connect(socket, &QTcpSocket::readyRead, this, &GameServer::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &GameServer::onClientDisconnected);
    }
}

void GameServer::onClientReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    ConnectedClient *client = nullptr;
    for (ConnectedClient *c : clients) {
        if (c->socket == socket) {
            client = c;
            break;
        }
    }
    if (!client) return;

    client->buffer.append(socket->readAll());

    if (!client->isWebSocket) {
        // Plain HTTP handshake / GET controller
        if (client->buffer.contains("\r\n\r\n")) {
            QString req = QString::fromUtf8(client->buffer);
            if (req.contains("Upgrade: websocket", Qt::CaseInsensitive)) {
                handleWebSocketHandshake(client, req);
            } else {
                handleHttpRequest(client, client->buffer);
            }
            client->buffer.clear();
        }
    } else {
        // WebSocket frame stream
        processWebSocketFrame(client);
    }
}

void GameServer::onClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    int clientIdToRemove = -1;
    QString roleToFree;
    QString nameToFree;

    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it.value()->socket == socket) {
            clientIdToRemove = it.key();
            roleToFree = it.value()->role;
            nameToFree = it.value()->playerName;
            delete it.value();
            clients.erase(it);
            break;
        }
    }

    if (clientIdToRemove != -1 && !roleToFree.isEmpty()) {
        emit playerLeft(clientIdToRemove, roleToFree, nameToFree);
        // Broadcast updated roles to all remaining clients
        QJsonObject rolesMsg;
        rolesMsg["type"] = "rolesUpdate";
        rolesMsg["roles"] = getRolesStatusJson();
        broadcastGameState(rolesMsg);
    }
}

void GameServer::handleHttpRequest(ConnectedClient *client, const QByteArray &data)
{
    Q_UNUSED(data);
    QByteArray html = getControllerHtml();
    QByteArray resp;
    resp.append("HTTP/1.1 200 OK\r\n");
    resp.append("Content-Type: text/html; charset=utf-8\r\n");
    resp.append("Cache-Control: no-cache, no-store, must-revalidate\r\n");
    resp.append("Pragma: no-cache\r\n");
    resp.append("Expires: 0\r\n");
    resp.append("Content-Length: " + QByteArray::number(html.size()) + "\r\n");
    resp.append("Access-Control-Allow-Origin: *\r\n");
    resp.append("Connection: close\r\n\r\n");
    resp.append(html);

    client->socket->write(resp);
    client->socket->flush();
}

void GameServer::handleWebSocketHandshake(ConnectedClient *client, const QString &request)
{
    QString key;
    const QStringList lines = request.split("\r\n");
    for (const QString &line : lines) {
        if (line.startsWith("Sec-WebSocket-Key:", Qt::CaseInsensitive)) {
            key = line.mid(18).trimmed();
            break;
        }
    }

    if (key.isEmpty()) {
        client->socket->close();
        return;
    }

    QByteArray magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    QByteArray accept = QCryptographicHash::hash(key.toUtf8() + magic, QCryptographicHash::Sha1).toBase64();

    QByteArray resp;
    resp.append("HTTP/1.1 101 Switching Protocols\r\n");
    resp.append("Upgrade: websocket\r\n");
    resp.append("Connection: Upgrade\r\n");
    resp.append("Sec-WebSocket-Accept: " + accept + "\r\n\r\n");

    client->socket->write(resp);
    client->socket->flush();
    client->isWebSocket = true;

    // Send initial welcome & roles status packet
    QJsonObject welcome;
    welcome["type"] = "init";
    welcome["clientId"] = client->id;
    welcome["roles"] = getRolesStatusJson();
    sendClientMessage(client->id, welcome);
}

void GameServer::processWebSocketFrame(ConnectedClient *client)
{
    QByteArray &buf = client->buffer;
    while (buf.size() >= 2) {
        uint8_t b0 = (uint8_t)buf[0];
        uint8_t b1 = (uint8_t)buf[1];
        int opcode = b0 & 0x0F;
        bool masked = (b1 & 0x80) != 0;
        int payloadLen = b1 & 0x7F;
        int headerLen = 2;

        if (payloadLen == 126) {
            if (buf.size() < 4) return;
            payloadLen = ((uint8_t)buf[2] << 8) | (uint8_t)buf[3];
            headerLen = 4;
        } else if (payloadLen == 127) {
            if (buf.size() < 10) return;
            // Support 32-bit payload size
            payloadLen = ((uint8_t)buf[6] << 24) | ((uint8_t)buf[7] << 16) | ((uint8_t)buf[8] << 8) | (uint8_t)buf[9];
            headerLen = 10;
        }

        int maskOffset = headerLen;
        int dataOffset = maskOffset + (masked ? 4 : 0);
        if (buf.size() < dataOffset + payloadLen) {
            return; // Wait for complete frame
        }

        QByteArray unmasked;
        unmasked.resize(payloadLen);
        if (masked) {
            const uint8_t mask[4] = {
                (uint8_t)buf[maskOffset],
                (uint8_t)buf[maskOffset+1],
                (uint8_t)buf[maskOffset+2],
                (uint8_t)buf[maskOffset+3]
            };
            for (int i = 0; i < payloadLen; ++i) {
                unmasked[i] = (char)((uint8_t)buf[dataOffset + i] ^ mask[i % 4]);
            }
        } else {
            unmasked = buf.mid(dataOffset, payloadLen);
        }

        buf.remove(0, dataOffset + payloadLen);

        if (opcode == 0x8) {
            // Close frame
            client->socket->close();
            return;
        } else if (opcode == 0x9) {
            // Ping -> reply Pong
            QByteArray pong;
            pong.append((char)0x8A);
            pong.append((char)0x00);
            client->socket->write(pong);
            client->socket->flush();
        } else if (opcode == 0x1) {
            // Text frame
            QString message = QString::fromUtf8(unmasked);
            handleWebSocketMessage(client, message);
        }
    }
}

void GameServer::handleWebSocketMessage(ConnectedClient *client, const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "join") {
        QString role = obj["role"].toString().trimmed();
        QString name = obj["name"].toString().trimmed();
        if (name.isEmpty()) name = "Player " + QString::number(client->id);
        if (name.length() > 15) name = name.left(15);

        // Check if role is available or if client already has it
        if (!role.isEmpty() && (!isRoleTaken(role) || client->role == role)) {
            client->role = role;
            client->playerName = name;
            emit playerJoined(client->id, role, name);

            QJsonObject ack;
            ack["type"] = "joined";
            ack["role"] = role;
            ack["name"] = name;
            sendClientMessage(client->id, ack);

            // Broadcast updated roles to everyone
            QJsonObject rolesMsg;
            rolesMsg["type"] = "rolesUpdate";
            rolesMsg["roles"] = getRolesStatusJson();
            broadcastGameState(rolesMsg);
        } else {
            QJsonObject reject;
            reject["type"] = "reject";
            reject["reason"] = "Role is already taken by another player!";
            sendClientMessage(client->id, reject);
        }
    } else if (type == "move") {
        int dir = obj["dir"].toInt();
        if (!client->role.isEmpty() && dir >= 0 && dir <= 4) {
            client->lastDir = dir;
            emit playerMoved(client->id, client->role, dir);
        }
    } else if (type == "peek") {
        if (!client->role.isEmpty() && client->role.startsWith("p")) {
            emit playerTriggeredPeek(client->id, client->role);
        }
    } else if (type == "leave") {
        if (!client->role.isEmpty()) {
            QString prevRole = client->role;
            QString prevName = client->playerName;
            client->role.clear();
            emit playerLeft(client->id, prevRole, prevName);

            QJsonObject rolesMsg;
            rolesMsg["type"] = "rolesUpdate";
            rolesMsg["roles"] = getRolesStatusJson();
            broadcastGameState(rolesMsg);
        }
    }
}

void GameServer::sendWebSocketFrame(QTcpSocket *socket, const QString &text)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray payload = text.toUtf8();
    QByteArray frame;
    frame.append((char)0x81); // FIN + Text

    int len = payload.size();
    if (len < 126) {
        frame.append((char)len);
    } else if (len <= 65535) {
        frame.append((char)126);
        frame.append((char)((len >> 8) & 0xFF));
        frame.append((char)(len & 0xFF));
    } else {
        frame.append((char)127);
        for (int i = 7; i >= 0; --i) {
            frame.append((char)((len >> (8 * i)) & 0xFF));
        }
    }
    frame.append(payload);

    socket->write(frame);
}

void GameServer::sendClientMessage(int clientId, const QJsonObject &obj)
{
    ConnectedClient *client = clients.value(clientId, nullptr);
    if (client && client->socket && client->isWebSocket) {
        QJsonDocument doc(obj);
        sendWebSocketFrame(client->socket, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    }
}

void GameServer::broadcastGameState(const QJsonObject &state)
{
    QJsonDocument doc(state);
    QString text = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    for (ConnectedClient *c : clients) {
        if (c->socket && c->isWebSocket) {
            sendWebSocketFrame(c->socket, text);
        }
    }
}

QJsonObject GameServer::getRolesStatusJson() const
{
    QJsonObject roles;
    // Pac-Heroes
    roles["p1"] = getRolePlayerName("p1");
    roles["p2"] = getRolePlayerName("p2");
    roles["p3"] = getRolePlayerName("p3");
    roles["p4"] = getRolePlayerName("p4");
    // Ghosts
    roles["blinky"] = getRolePlayerName("blinky");
    roles["pinky"] = getRolePlayerName("pinky");
    roles["inky"] = getRolePlayerName("inky");
    roles["clyde"] = getRolePlayerName("clyde");
    return roles;
}

QByteArray GameServer::getControllerHtml() const
{
    return R"rawhtml(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>Pac-Man: Classroom Multiplayer</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; -webkit-user-select: none; }
  body {
    background: #060814;
    color: #fff;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    overflow-x: hidden;
  }
  .app-container {
    width: 100%;
    max-width: 440px;
    padding: 10px 12px 20px;
    display: flex;
    flex-direction: column;
    align-items: center;
    transition: max-width 0.2s ease;
  }
  header {
    text-align: center;
    margin-bottom: 10px;
  }
  h1 {
    font-size: 20px;
    font-weight: 900;
    letter-spacing: 1.5px;
    background: linear-gradient(90deg, #ffe600, #ff0077, #00f0ff);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
  }
  .subtitle {
    font-size: 10px;
    letter-spacing: 2px;
    color: #8b9bb4;
    text-transform: uppercase;
  }

  /* Lobby Card */
  .card {
    background: #111429;
    border: 1px solid #232b54;
    border-radius: 14px;
    padding: 14px;
    width: 100%;
    box-shadow: 0 10px 25px rgba(0,0,0,0.5);
    margin-bottom: 12px;
  }
  .input-group {
    margin-bottom: 12px;
    width: 100%;
  }
  .input-group label {
    font-size: 11px;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 1px;
    color: #00e5ff;
    display: block;
    margin-bottom: 6px;
  }
  .input-group input {
    width: 100%;
    padding: 10px 12px;
    border-radius: 8px;
    background: #0a0c1a;
    border: 1px solid #283363;
    color: #fff;
    font-size: 14px;
    font-weight: 600;
    outline: none;
  }
  .input-group input:focus {
    border-color: #00e5ff;
    box-shadow: 0 0 10px rgba(0,229,255,0.3);
  }
  .section-title {
    font-size: 11px;
    font-weight: 800;
    letter-spacing: 1.2px;
    text-transform: uppercase;
    margin: 10px 0 6px 0;
  }
  .role-grid {
    display: grid;
    grid-template-columns: 1fr;
    gap: 7px;
    width: 100%;
  }
  .role-btn {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 9px 12px;
    border-radius: 10px;
    border: 1.5px solid #242c55;
    background: #0e1124;
    color: #fff;
    cursor: pointer;
    transition: all 0.12s ease;
  }
  .role-btn:active:not(:disabled) {
    transform: scale(0.97);
  }
  .role-info {
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .role-icon { font-size: 20px; }
  .role-name { font-size: 13px; font-weight: 700; text-align: left; }
  .role-sub { font-size: 9.5px; color: #8395b0; text-align: left; }
  .role-badge {
    font-size: 10px;
    font-weight: 700;
    padding: 3px 8px;
    border-radius: 14px;
    background: #192040;
    color: #00ffcc;
  }
  .role-btn:disabled {
    opacity: 0.45;
    cursor: not-allowed;
    border-color: #1a1e36;
  }
  .role-btn:disabled .role-badge {
    background: #2b1822;
    color: #ff5577;
  }

  /* Gameplay Screen */
  #gameScreen {
    display: none;
    width: 100%;
    flex-direction: column;
    align-items: center;
  }

  /* Top HUD Bar */
  .hud-bar {
    width: 100%;
    background: #111429;
    border: 1px solid #232b54;
    border-radius: 10px;
    padding: 8px 12px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 8px;
    box-shadow: 0 4px 15px rgba(0,0,0,0.4);
  }
  .hero-chip {
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .hero-chip-icon { font-size: 22px; }
  .hero-chip-title { font-size: 13px; font-weight: 800; }
  .hero-chip-sub { font-size: 9px; color: #889bb8; }
  .hud-stat {
    text-align: center;
  }
  .hud-lbl { font-size: 8px; font-weight: 800; color: #7384a8; letter-spacing: 1px; }
  .hud-val { font-size: 13px; font-weight: 900; color: #ffe600; font-family: monospace; }
  .hud-right {
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .status-pill {
    font-size: 10px;
    font-weight: 800;
    padding: 3px 8px;
    border-radius: 10px;
    background: #00ff8822;
    color: #00ff88;
    border: 1px solid #00ff8866;
  }
  .status-pill.frightened {
    background: #00e5ff22;
    color: #00e5ff;
    border-color: #00e5ff;
    animation: pulse 0.5s infinite alternate;
  }
  @keyframes pulse { from { opacity: 0.5; } to { opacity: 1; } }

  .fullscreen-btn {
    background: #192040;
    border: 1px solid #2d3b70;
    color: #00e5ff;
    border-radius: 8px;
    padding: 4px 8px;
    font-size: 13px;
    cursor: pointer;
    line-height: 1;
    display: flex;
    align-items: center;
    justify-content: center;
  }
  .fullscreen-btn:active { background: #00e5ff; color: #070914; }

  /* Stage Layout (Responsive Mobile vs Desktop) */
  .stage-layout {
    width: 100%;
    display: flex;
    flex-direction: column;
    align-items: center;
  }

  /* Canvas Box */
  .canvas-box {
    position: relative;
    display: flex;
    justify-content: center;
    align-items: center;
    margin-bottom: 8px;
  }
  #gameCanvas {
    width: min(94vw, 390px);
    height: min(94vw, 390px);
    aspect-ratio: 1;
    background: #060814;
    border-radius: 12px;
    border: 2px solid #232c58;
    box-shadow: 0 6px 20px rgba(0,0,0,0.6), 0 0 16px rgba(0, 229, 255, 0.15);
    display: block;
    touch-action: none;
  }

  /* Side Panel & Controls */
  .side-panel {
    width: 100%;
    display: flex;
    flex-direction: column;
    align-items: center;
  }

  /* D-Pad Controls */
  .dpad-container {
    width: 180px;
    height: 180px;
    position: relative;
    margin: 4px auto 8px auto;
  }
  .d-btn {
    position: absolute;
    width: 56px;
    height: 56px;
    border-radius: 14px;
    background: #161b36;
    border: 2px solid #2d3868;
    color: #00f0ff;
    font-size: 22px;
    display: flex;
    align-items: center;
    justify-content: center;
    box-shadow: 0 4px 10px rgba(0,0,0,0.5);
    cursor: pointer;
    touch-action: manipulation;
    -webkit-tap-highlight-color: transparent;
  }
  .d-btn:active {
    background: #00e5ff;
    color: #060814;
    border-color: #fff;
    transform: scale(0.92);
    box-shadow: 0 0 16px #00e5ff;
  }
  .btn-up { top: 0; left: 62px; }
  .btn-down { bottom: 0; left: 62px; }
  .btn-left { top: 62px; left: 0; }
  .btn-right { top: 62px; right: 0; }
  .dpad-center {
    position: absolute;
    top: 62px;
    left: 62px;
    width: 56px;
    height: 56px;
    border-radius: 14px;
    background: #0e1124;
    border: 1px dashed #242c55;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 18px;
    color: #4b588c;
  }

  /* Action Buttons */
  .action-row {
    width: 100%;
    display: flex;
    gap: 8px;
    margin-top: 4px;
  }
  .action-btn {
    width: 100%;
    padding: 12px;
    border-radius: 10px;
    border: none;
    font-size: 13px;
    font-weight: 800;
    letter-spacing: 0.5px;
    cursor: pointer;
    box-shadow: 0 4px 12px rgba(0,0,0,0.4);
    transition: transform 0.1s;
    touch-action: manipulation;
    text-align: center;
  }
  .action-btn:active { transform: scale(0.96); }
  .btn-peek {
    background: linear-gradient(135deg, #00d2ff, #0066ff);
    color: #fff;
    box-shadow: 0 4px 14px rgba(0,150,255,0.4);
  }
  .btn-peek:disabled {
    background: #1b2038;
    color: #647094;
    box-shadow: none;
    cursor: not-allowed;
  }
  .btn-subhint {
    display: none;
  }
  .btn-ghost-status {
    background: #1b1328;
    border: 1.5px solid #6c2a9a;
    color: #da70d6;
    cursor: default;
  }
  .leave-btn {
    width: 100%;
    background: transparent;
    border: 1px solid #2d3866;
    color: #8292b3;
    font-size: 11px;
    font-weight: 700;
    padding: 10px 8px;
    border-radius: 10px;
    cursor: pointer;
    margin-top: 6px;
  }
  .leave-btn:active { background: #161b36; }
  .desktop-guide {
    display: none;
  }
  .keyboard-hint {
    margin-top: 8px;
    font-size: 10px;
    color: #617192;
    text-align: center;
  }

  /* ======================================================== */
  /* LAPTOP / DESKTOP PC MODE (Screen width >= 768px)         */
  /* Expands to match host desktop view and fill screen!      */
  /* ======================================================== */
  @media (min-width: 768px) {
    .app-container {
      max-width: 1100px;
      padding: 14px 20px 24px;
    }
    header h1 {
      font-size: 24px;
    }
    header .subtitle {
      font-size: 11px;
    }
    .hud-bar {
      padding: 10px 18px;
      margin-bottom: 14px;
    }
    .hero-chip-icon { font-size: 26px; }
    .hero-chip-title { font-size: 15px; }
    .hero-chip-sub { font-size: 11px; }
    .hud-lbl { font-size: 9px; }
    .hud-val { font-size: 16px; }
    .status-pill { font-size: 11px; padding: 4px 10px; }

    .stage-layout {
      flex-direction: row;
      align-items: flex-start;
      justify-content: center;
      gap: 24px;
      width: 100%;
    }
    .canvas-box {
      margin-bottom: 0;
    }
    #gameCanvas {
      width: min(74vh, 640px);
      height: min(74vh, 640px);
      border-radius: 16px;
      border: 3px solid #2b3972;
      box-shadow: 0 10px 35px rgba(0,0,0,0.8), 0 0 25px rgba(0,229,255,0.22);
    }
    .side-panel {
      width: 270px;
      align-items: stretch;
      gap: 10px;
    }
    .action-row {
      flex-direction: column;
      gap: 6px;
    }
    .action-btn {
      padding: 14px;
      font-size: 14px;
      border-radius: 12px;
    }
    .btn-subhint {
      display: block;
      font-size: 10px;
      opacity: 0.85;
      font-weight: 600;
      margin-top: 2px;
    }
    .dpad-container {
      margin: 2px auto;
      transform: scale(0.9);
    }
    .desktop-guide {
      display: flex;
      flex-direction: column;
      gap: 6px;
      background: #111429;
      border: 1px solid #232b54;
      border-radius: 12px;
      padding: 12px 14px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.3);
    }
    .guide-title {
      font-size: 11px;
      font-weight: 800;
      letter-spacing: 1.5px;
      color: #00e5ff;
      margin-bottom: 2px;
    }
    .guide-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      font-size: 12px;
      color: #b0c2de;
    }
    .key-badge {
      font-family: monospace;
      font-weight: 700;
      background: #1c234a;
      border: 1px solid #364484;
      color: #ffe600;
      padding: 2px 7px;
      border-radius: 5px;
      font-size: 11px;
    }
    .guide-note {
      font-size: 10px;
      color: #00ff88;
      font-weight: 700;
      margin-top: 3px;
      text-align: center;
    }
    .keyboard-hint {
      display: none;
    }
  }
</style>
</head>
<body>

<div class="app-container">
  <header>
    <h1>PAC-MAN MULTIPLAYER</h1>
    <div class="subtitle">Classroom Graphics Lab</div>
  </header>

  <!-- LOBBY SCREEN -->
  <div id="lobbyScreen" style="width:100%;">
    <div class="card">
      <div class="input-group">
        <label>Your Name</label>
        <input type="text" id="nameInput" placeholder="Enter your name (e.g. Alex)" maxlength="15">
      </div>

      <div class="section-title" style="color: #ff99cc;">🟡 Mode A: Pac-Family Co-op (4 Heroes)</div>
      <div class="role-grid">
        <button class="role-btn" id="btn-p1" onclick="selectRole('p1')">
          <div class="role-info"><span class="role-icon">🟡</span><div><div class="role-name" style="color:#ffe600;">Pac-Man</div><div class="role-sub">Classic Yellow Hero</div></div></div>
          <span class="role-badge" id="badge-p1">JOIN</span>
        </button>
        <button class="role-btn" id="btn-p2" onclick="selectRole('p2')">
          <div class="role-info"><span class="role-icon">🎀</span><div><div class="role-name" style="color:#ff69b4;">Ms. Pac-Man</div><div class="role-sub">Pink Hero with Red Ribbon Bow</div></div></div>
          <span class="role-badge" id="badge-p2">JOIN</span>
        </button>
        <button class="role-btn" id="btn-p3" onclick="selectRole('p3')">
          <div class="role-info"><span class="role-icon">🧢</span><div><div class="role-name" style="color:#00e5ff;">Jr. Pac-Man</div><div class="role-sub">Cyan Hero with Beanie Cap</div></div></div>
          <span class="role-badge" id="badge-p3">JOIN</span>
        </button>
        <button class="role-btn" id="btn-p4" onclick="selectRole('p4')">
          <div class="role-info"><span class="role-icon">🟢</span><div><div class="role-name" style="color:#00ff66;">Miracle Pac-Man</div><div class="role-sub">Emerald Green Superhero</div></div></div>
          <span class="role-badge" id="badge-p4">JOIN</span>
        </button>
      </div>

      <div class="section-title" style="color: #ff4466; margin-top:14px;">👻 Mode B: Playable Ghost Hunters</div>
      <div class="role-grid">
        <button class="role-btn" id="btn-blinky" onclick="selectRole('blinky')">
          <div class="role-info"><span class="role-icon">🔴</span><div><div class="role-name" style="color:#ff3333;">Blinky (Red)</div><div class="role-sub">Aggressive Chaser</div></div></div>
          <span class="role-badge" id="badge-blinky">JOIN</span>
        </button>
        <button class="role-btn" id="btn-pinky" onclick="selectRole('pinky')">
          <div class="role-info"><span class="role-icon">🌸</span><div><div class="role-name" style="color:#ff99cc;">Pinky (Pink)</div><div class="role-sub">Speedy Ambusher</div></div></div>
          <span class="role-badge" id="badge-pinky">JOIN</span>
        </button>
        <button class="role-btn" id="btn-inky" onclick="selectRole('inky')">
          <div class="role-info"><span class="role-icon">🩵</span><div><div class="role-name" style="color:#00ffff;">Inky (Cyan)</div><div class="role-sub">Tactical Flanker</div></div></div>
          <span class="role-badge" id="badge-inky">JOIN</span>
        </button>
        <button class="role-btn" id="btn-clyde" onclick="selectRole('clyde')">
          <div class="role-info"><span class="role-icon">🟠</span><div><div class="role-name" style="color:#ffaa00;">Clyde (Orange)</div><div class="role-sub">Roaming Hunter</div></div></div>
          <span class="role-badge" id="badge-clyde">JOIN</span>
        </button>
      </div>
    </div>
  </div>

  <!-- GAMEPLAY SCREEN -->
  <div id="gameScreen">
    <div class="hud-bar">
      <div class="hero-chip">
        <span class="hero-chip-icon" id="activeIcon">🎀</span>
        <div>
          <div class="hero-chip-title" id="activeTitle">Ms. Pac-Man</div>
          <div class="hero-chip-sub" id="activeName">Player</div>
        </div>
      </div>
      <div class="hud-stat">
        <div class="hud-lbl">STAGE</div>
        <div class="hud-val" id="hudStage">1-1</div>
      </div>
      <div class="hud-stat">
        <div class="hud-lbl">SCORE</div>
        <div class="hud-val" id="hudScore">0</div>
      </div>
      <div class="hud-right">
        <div class="status-pill" id="statusPill">LIVE</div>
        <button class="fullscreen-btn" onclick="toggleFullscreen()" title="Toggle Fullscreen">⛶</button>
      </div>
    </div>

    <!-- RESPONSIVE STAGE LAYOUT -->
    <div class="stage-layout">
      <!-- LIVE INTERACTIVE HIGH-RES GAME CANVAS -->
      <div class="canvas-box">
        <canvas id="gameCanvas" width="630" height="630"></canvas>
      </div>

      <!-- SIDE DASHBOARD / CONTROLS -->
      <div class="side-panel">
        <div class="action-row">
          <button class="action-btn btn-peek" id="actionBtn" onclick="sendAction()">
            👁️ FOG PEEK (3 left)
            <div class="btn-subhint">Press [Space] or [F]</div>
          </button>
        </div>

        <!-- TOUCH D-PAD (1 TAP = 1 STEP) -->
        <div class="dpad-container">
          <button class="d-btn btn-up" ontouchstart="sendDir(0, event)" onclick="sendDir(0, event)">▲</button>
          <button class="d-btn btn-left" ontouchstart="sendDir(2, event)" onclick="sendDir(2, event)">◀</button>
          <div class="dpad-center" id="centerIcon">🕹️</div>
          <button class="d-btn btn-right" ontouchstart="sendDir(3, event)" onclick="sendDir(3, event)">▶</button>
          <button class="d-btn btn-down" ontouchstart="sendDir(1, event)" onclick="sendDir(1, event)">▼</button>
        </div>

        <!-- LAPTOP KEYBOARD GUIDE -->
        <div class="desktop-guide">
          <div class="guide-title">💻 LAPTOP KEYBOARD</div>
          <div class="guide-row"><span class="key-badge">W / ↑</span><span class="key-desc">Step Up</span></div>
          <div class="guide-row"><span class="key-badge">A / ←</span><span class="key-desc">Step Left</span></div>
          <div class="guide-row"><span class="key-badge">S / ↓</span><span class="key-desc">Step Down</span></div>
          <div class="guide-row"><span class="key-badge">D / →</span><span class="key-desc">Step Right</span></div>
          <div class="guide-row"><span class="key-badge">Space / F</span><span class="key-desc">Fog Peek</span></div>
          <div class="guide-note">⚡ 1 Press = 1 Step (Same as host)</div>
        </div>

        <button class="leave-btn" onclick="leaveRole()">⇄ Switch Role / Leave</button>
      </div>
    </div>
  </div>
</div>

<script>
  let ws = null;
  let currentRole = null;
  let latestState = null;
  let animTick = 0;
  let lastTouchTime = 0;
  let myName = localStorage.getItem('pacman_pname') || '';
  if (myName) document.getElementById('nameInput').value = myName;

  // High-Resolution 630x630 Coordinate System (30px per cell)
  const canvas = document.getElementById('gameCanvas');
  const ctx = canvas.getContext('2d');
  const C_SIZE = 30; // 630 / 21 = 30px per cell

  // Offscreen canvas for lantern fog spotlight
  const fogCanvas = document.createElement('canvas');
  fogCanvas.width = 630;
  fogCanvas.height = 630;
  const fogCtx = fogCanvas.getContext('2d');

  const roleMeta = {
    p1: { icon: "🟡", name: "Pac-Man", color: "#ffe600", isHero: true },
    p2: { icon: "🎀", name: "Ms. Pac-Man", color: "#ff69b4", isHero: true },
    p3: { icon: "🧢", name: "Jr. Pac-Man", color: "#00e5ff", isHero: true },
    p4: { icon: "🟢", name: "Miracle Pac-Man", color: "#00ff66", isHero: true },
    blinky: { icon: "🔴", name: "Blinky", color: "#ff3333", isHero: false, ghostIdx: 0 },
    pinky: { icon: "🌸", name: "Pinky", color: "#ff99cc", isHero: false, ghostIdx: 1 },
    inky: { icon: "🩵", name: "Inky", color: "#00ffff", isHero: false, ghostIdx: 2 },
    clyde: { icon: "🟠", name: "Clyde", color: "#ffaa00", isHero: false, ghostIdx: 3 }
  };

  function toggleFullscreen() {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen().catch(() => {});
    } else {
      if (document.exitFullscreen) document.exitFullscreen().catch(() => {});
    }
  }

  function connect() {
    const loc = window.location;
    const proto = (loc.protocol === 'https:') ? 'wss:' : 'ws:';
    ws = new WebSocket(proto + '//' + loc.host);

    ws.onmessage = (e) => {
      try {
        const msg = JSON.parse(e.data);
        if (msg.type === 'init' || msg.type === 'rolesUpdate') {
          updateRoles(msg.roles);
        } else if (msg.type === 'joined') {
          currentRole = msg.role;
          showGameScreen(msg.role, msg.name);
        } else if (msg.type === 'reject') {
          alert(msg.reason || "Slot unavailable!");
        } else if (msg.type === 'gameState') {
          latestState = msg;
          handleGameState(msg);
        }
      } catch (err) {}
    };

    ws.onclose = () => {
      const pill = document.getElementById('statusPill');
      if (pill) {
        pill.innerText = "OFFLINE";
        pill.style.color = "#ff5577";
        pill.style.borderColor = "#ff5577";
      }
      setTimeout(connect, 1500);
    };
  }

  function updateRoles(roles) {
    if (!roles) return;
    for (const [r, takenBy] of Object.entries(roles)) {
      const btn = document.getElementById('btn-' + r);
      const badge = document.getElementById('badge-' + r);
      if (btn && badge) {
        if (takenBy && takenBy.length > 0) {
          btn.disabled = (currentRole !== r);
          badge.innerText = takenBy;
        } else {
          btn.disabled = false;
          badge.innerText = "JOIN";
        }
      }
    }
  }

  function selectRole(role) {
    let name = document.getElementById('nameInput').value.trim();
    if (!name) name = "Player";
    localStorage.setItem('pacman_pname', name);
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: 'join', role: role, name: name }));
    }
  }

  function showGameScreen(role, name) {
    const meta = roleMeta[role] || { icon: "🎮", name: "Player", color: "#fff", isHero: true };
    document.getElementById('lobbyScreen').style.display = 'none';
    document.getElementById('gameScreen').style.display = 'flex';
    document.getElementById('activeIcon').innerText = meta.icon;
    document.getElementById('activeTitle').innerText = meta.name;
    document.getElementById('activeTitle').style.color = meta.color;
    document.getElementById('activeName').innerText = name;
    document.getElementById('centerIcon').innerText = meta.icon;

    const actionBtn = document.getElementById('actionBtn');
    if (meta.isHero) {
      actionBtn.className = "action-btn btn-peek";
      actionBtn.innerHTML = '👁️ FOG PEEK (3 left)<div class="btn-subhint">Press [Space] or [F]</div>';
      actionBtn.disabled = false;
    } else {
      actionBtn.className = "action-btn btn-ghost-status";
      actionBtn.innerHTML = '🎯 HUNT PAC-MAN!<div class="btn-subhint">Catch the Pac-Heroes!</div>';
      actionBtn.disabled = true;
    }
  }

  // Single step movement (1 tap/click = 1 step)
  function sendDir(dir, ev) {
    if (ev) {
      if (ev.type === 'touchstart') {
        lastTouchTime = Date.now();
      } else if (ev.type === 'click' && (Date.now() - lastTouchTime < 450)) {
        return; // ignore synthetic ghost click
      }
      ev.preventDefault();
    }
    if (navigator.vibrate) navigator.vibrate(20);
    if (ws && ws.readyState === WebSocket.OPEN && currentRole) {
      ws.send(JSON.stringify({ type: 'move', dir: dir }));
    }
  }

  function sendAction() {
    if (navigator.vibrate) navigator.vibrate(35);
    const meta = roleMeta[currentRole];
    if (meta && meta.isHero) {
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type: 'peek' }));
      }
    }
  }

  function leaveRole() {
    if (ws && ws.readyState === WebSocket.OPEN && currentRole) {
      ws.send(JSON.stringify({ type: 'leave' }));
    }
    currentRole = null;
    document.getElementById('gameScreen').style.display = 'none';
    document.getElementById('lobbyScreen').style.display = 'block';
  }

  function handleGameState(state) {
    const pill = document.getElementById('statusPill');
    const scoreVal = document.getElementById('hudScore');
    const stageVal = document.getElementById('hudStage');
    const actionBtn = document.getElementById('actionBtn');

    if (scoreVal && state.score !== undefined) scoreVal.innerText = state.score;
    if (stageVal && state.world !== undefined && state.stage !== undefined) {
      stageVal.innerText = state.world + '-' + state.stage;
    }

    const meta = roleMeta[currentRole];
    if (meta && meta.isHero) {
      let peeks = 3;
      if (state.peeks && state.peeks[currentRole] !== undefined) {
        peeks = state.peeks[currentRole];
      } else if (state.fogPeeks !== undefined) {
        peeks = state.fogPeeks;
      }
      if (peeks > 0) {
        actionBtn.innerHTML = '👁️ FOG PEEK (' + peeks + ' left)<div class="btn-subhint">Press [Space] or [F]</div>';
        actionBtn.disabled = false;
        actionBtn.style.opacity = "1";
      } else {
        actionBtn.innerHTML = '🔒 0 PEEKS (Eat fruit for +1)<div class="btn-subhint">Collect fruits on map</div>';
        actionBtn.disabled = true;
        actionBtn.style.opacity = "0.6";
      }
    }

    if (state.frightened) {
      pill.innerText = "ENERGIZED!";
      pill.classList.add('frightened');
      if (meta && !meta.isHero) {
        actionBtn.innerHTML = '😱 DANGER! ESCAPE PAC-MAN!<div class="btn-subhint">Pac-Man is energized!</div>';
        actionBtn.style.background = "#0044aa";
        actionBtn.style.color = "#00ffff";
      }
    } else {
      pill.classList.remove('frightened');
      if (state.gameOver) {
        pill.innerText = "GAME OVER";
        pill.style.color = "#ff4466";
      } else if (state.levelComplete) {
        pill.innerText = "STAGE CLEAR!";
        pill.style.color = "#00ff88";
      } else if (state.peek) {
        pill.innerText = "PEEK ACTIVE";
        pill.style.color = "#ffe600";
      } else {
        pill.innerText = "LIVE";
        pill.style.color = "#00ff88";
      }

      if (meta && !meta.isHero) {
        actionBtn.innerHTML = '🎯 HUNT PAC-MAN!<div class="btn-subhint">Catch the Pac-Heroes!</div>';
        actionBtn.style.background = "#1b1328";
        actionBtn.style.color = "#da70d6";
      }
    }
  }

  // Keyboard navigation for laptop users: 1 keypress = 1 step
  window.addEventListener('keydown', (e) => {
    if (!currentRole) return;
    if (e.repeat) return; // Prevent continuous fast repeating
    if (e.key === 'ArrowUp' || e.key === 'w' || e.key === 'W') { sendDir(0, e); }
    else if (e.key === 'ArrowDown' || e.key === 's' || e.key === 'S') { sendDir(1, e); }
    else if (e.key === 'ArrowLeft' || e.key === 'a' || e.key === 'A') { sendDir(2, e); }
    else if (e.key === 'ArrowRight' || e.key === 'd' || e.key === 'D') { sendDir(3, e); }
    else if (e.key === ' ' || e.key === 'f' || e.key === 'F') { sendAction(); e.preventDefault(); }
  });

  // ==========================================
  // HTML5 Canvas 2D Game Renderer (630x630)
  // ==========================================
  function render() {
    animTick++;
    ctx.clearRect(0, 0, 630, 630);

    if (!latestState) {
      ctx.fillStyle = "#070914";
      ctx.fillRect(0, 0, 630, 630);
      ctx.fillStyle = "#4a5980";
      ctx.font = "bold 15px sans-serif";
      ctx.textAlign = "center";
      ctx.fillText("Connecting to classroom game server...", 315, 305);
      requestAnimationFrame(render);
      return;
    }

    if (latestState.screen === 'map') {
      drawMapStandby();
      requestAnimationFrame(render);
      return;
    }

    // Live maze rendering
    drawMaze(latestState);
    drawExitPortal(latestState);
    drawFruits(latestState);
    drawHeroes(latestState);
    drawGhosts(latestState);
    applyFogOfWar(latestState);
    drawOverlays(latestState);

    requestAnimationFrame(render);
  }

  function drawMapStandby() {
    ctx.fillStyle = "#070914";
    ctx.fillRect(0, 0, 630, 630);

    ctx.strokeStyle = "rgba(40, 55, 100, 0.25)";
    ctx.lineWidth = 1.5;
    for (let i = 0; i < 630; i += 45) {
      ctx.beginPath(); ctx.moveTo(i, 0); ctx.lineTo(i, 630); ctx.stroke();
      ctx.beginPath(); ctx.moveTo(0, i); ctx.lineTo(630, i); ctx.stroke();
    }

    const pulse = Math.sin(animTick * 0.05) * 6;
    ctx.strokeStyle = "#00e5ff";
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.arc(315, 240, 55 + pulse, 0, Math.PI * 2);
    ctx.stroke();

    ctx.fillStyle = "#00e5ff";
    ctx.font = "44px sans-serif";
    ctx.textAlign = "center";
    ctx.fillText("🗺️", 315, 255);

    ctx.fillStyle = "#ffffff";
    ctx.font = "bold 22px sans-serif";
    ctx.fillText("WORLD ADVENTURE MAP", 315, 345);

    ctx.fillStyle = "#8aa0c4";
    ctx.font = "15px sans-serif";
    ctx.fillText("Stage selection in progress on host console...", 315, 380);
    ctx.fillStyle = "#ffe600";
    ctx.font = "bold 14px sans-serif";
    ctx.fillText("⚡ Get ready to play!", 315, 415);
  }

  function drawMaze(state) {
    ctx.fillStyle = "#070914";
    ctx.fillRect(0, 0, 630, 630);

    const board = state.board;
    const explored = state.explored;
    const isFog = state.fog && !state.peek;

    if (!board || board.length < 441) return;

    for (let r = 0; r < 21; ++r) {
      for (let c = 0; c < 21; ++c) {
        const idx = r * 21 + c;
        const cell = board[idx];
        const wasExplored = (explored && explored[idx] === '1');
        const cx = c * C_SIZE;
        const cy = r * C_SIZE;

        if (cell === '1') {
          if (isFog && !wasExplored) {
            ctx.fillStyle = "#030408";
            ctx.fillRect(cx, cy, C_SIZE, C_SIZE);
          } else {
            ctx.fillStyle = wasExplored ? "#141c48" : "#0c112e";
            ctx.strokeStyle = wasExplored ? "#2844a8" : "#1a2c6d";
            ctx.lineWidth = 2;
            roundRect(ctx, cx + 1.5, cy + 1.5, C_SIZE - 3, C_SIZE - 3, 5);
            ctx.fill();
            ctx.stroke();
          }
        } else {
          ctx.fillStyle = "#070914";
          ctx.fillRect(cx, cy, C_SIZE, C_SIZE);

          if (cell === '2') {
            if (!isFog || wasExplored) {
              ctx.fillStyle = "#ffe6b0";
              ctx.beginPath();
              ctx.arc(cx + 15, cy + 15, 3.5, 0, Math.PI * 2);
              ctx.fill();
            }
          } else if (cell === '3') {
            if (!isFog || wasExplored) {
              const pelletPulse = Math.sin(animTick * 0.15) * 2;
              ctx.fillStyle = "#ffffff";
              ctx.shadowColor = "#ffcc00";
              ctx.shadowBlur = 10;
              ctx.beginPath();
              ctx.arc(cx + 15, cy + 15, 7.5 + pelletPulse, 0, Math.PI * 2);
              ctx.fill();
              ctx.shadowBlur = 0;
            }
          } else if (cell === '4') {
            ctx.strokeStyle = "#ff69b4";
            ctx.lineWidth = 3;
            ctx.beginPath();
            ctx.moveTo(cx, cy + 15);
            ctx.lineTo(cx + C_SIZE, cy + 15);
            ctx.stroke();
          }
        }
      }
    }
  }

  function roundRect(ctx, x, y, w, h, r) {
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.arcTo(x + w, y, x + w, y + h, r);
    ctx.arcTo(x + w, y + h, x, y + h, r);
    ctx.arcTo(x, y + h, x, y, r);
    ctx.arcTo(x, y, x + w, y, r);
    ctx.closePath();
  }

  function drawExitPortal(state) {
    if (!state.hasExit) return;
    const isFog = state.fog && !state.peek;
    const exploredIdx = state.exitY * 21 + state.exitX;
    const wasExplored = (state.explored && state.explored[exploredIdx] === '1');

    if (isFog && !wasExplored) return;

    const ex = state.exitX * C_SIZE + 15;
    const ey = state.exitY * C_SIZE + 15;
    const pulse = Math.sin(animTick * 0.1) * 4;

    const grad = ctx.createRadialGradient(ex, ey, 4, ex, ey, 24 + pulse);
    grad.addColorStop(0, "rgba(0, 255, 128, 0.9)");
    grad.addColorStop(0.5, "rgba(0, 200, 100, 0.4)");
    grad.addColorStop(1, "rgba(0, 0, 0, 0)");
    ctx.fillStyle = grad;
    ctx.beginPath();
    ctx.arc(ex, ey, 24 + pulse, 0, Math.PI * 2);
    ctx.fill();

    ctx.strokeStyle = "#00ff88";
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.arc(ex, ey, 11, 0, Math.PI * 2);
    ctx.stroke();

    ctx.fillStyle = "#ffffff";
    ctx.font = "bold 11px sans-serif";
    ctx.textAlign = "center";
    ctx.fillText("EXIT", ex, ey + 4);
  }

  function drawFruits(state) {
    if (!state.fruits || state.fruits.length === 0) return;
    const fruitIcons = ["🍒", "🍓", "🍊", "🍎", "🍈", "🔔", "🔑"];
    for (const f of state.fruits) {
      const fx = f.x * C_SIZE + 15;
      const fy = f.y * C_SIZE + 22;
      ctx.font = "20px sans-serif";
      ctx.textAlign = "center";
      ctx.fillText(fruitIcons[f.type % fruitIcons.length] || "🍒", fx, fy);
    }
  }

  // Draw ONLY active players who have actually joined!
  function drawHeroes(state) {
    // P1 (Host - Yellow Pac-Man)
    if (state.p1 && (state.p1.alive || (state.p1.respawnTicks > 0 && Math.floor(animTick / 4) % 2 === 0))) {
      drawPacHero(state.p1, "#ffe600", "p1", false, false, false, "P1 (Host)");
    }
    // P2 (Ms. Pac-Man) - only if active!
    if (state.p2 && state.p2.active && (state.p2.alive || (state.p2.respawnTicks > 0 && Math.floor(animTick / 4) % 2 === 0))) {
      drawPacHero(state.p2, "#ff69b4", "p2", true, false, false, state.p2.name || "Ms. Pac");
    }
    // P3 (Jr. Pac-Man) - only if active!
    if (state.p3 && state.p3.active && (state.p3.alive || (state.p3.respawnTicks > 0 && Math.floor(animTick / 4) % 2 === 0))) {
      drawPacHero(state.p3, "#00e5ff", "p3", false, true, false, state.p3.name || "Jr. Pac");
    }
    // P4 (Miracle Pac-Man) - only if active!
    if (state.p4 && state.p4.active && (state.p4.alive || (state.p4.respawnTicks > 0 && Math.floor(animTick / 4) % 2 === 0))) {
      drawPacHero(state.p4, "#00ff66", "p4", false, false, true, state.p4.name || "Miracle");
    }
  }

  function drawPacHero(h, color, roleKey, bow, cap, headband, defaultTag) {
    if (!h) return;
    const isRespawning = (!h.alive && h.respawnTicks > 0);
    if (!h.alive && !isRespawning) return;

    const hx = h.x * C_SIZE + 15;
    const hy = h.y * C_SIZE + 15;
    const dir = h.dir; // 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT

    let angle = 0;
    if (dir === 3) angle = 0;
    else if (dir === 1) angle = Math.PI / 2;
    else if (dir === 2) angle = Math.PI;
    else if (dir === 0) angle = -Math.PI / 2;

    const mouthDeg = (h.mouth !== undefined) ? h.mouth : 25;
    const mouthRad = (mouthDeg * Math.PI) / 180;

    ctx.save();
    if (isRespawning) {
      ctx.globalAlpha = 0.55;
    }
    ctx.translate(hx, hy);
    ctx.rotate(angle);

    ctx.fillStyle = color;
    ctx.beginPath();
    ctx.arc(0, 0, 12.5, mouthRad, Math.PI * 2 - mouthRad);
    ctx.lineTo(0, 0);
    ctx.closePath();
    ctx.fill();

    ctx.restore();

    // Accessories
    ctx.save();
    if (isRespawning) {
      ctx.globalAlpha = 0.55;
    }
    if (bow) {
      ctx.fillStyle = "#ff1744";
      ctx.beginPath();
      ctx.arc(hx - 5, hy - 10, 3.8, 0, Math.PI * 2);
      ctx.arc(hx + 5, hy - 10, 3.8, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = "#fff";
      ctx.fillRect(hx - 1.5, hy - 12, 3, 3);
    }
    if (cap) {
      ctx.fillStyle = "#0066ff";
      ctx.fillRect(hx - 7, hy - 13, 14, 3.5);
      ctx.strokeStyle = "#ff3333";
      ctx.lineWidth = 1.8;
      ctx.beginPath();
      ctx.moveTo(hx, hy - 13); ctx.lineTo(hx, hy - 18);
      ctx.moveTo(hx - 5, hy - 18); ctx.lineTo(hx + 5, hy - 18);
      ctx.stroke();
    }
    if (headband) {
      ctx.fillStyle = "#00a833";
      ctx.fillRect(hx - 9, hy - 6, 18, 3);
    }
    ctx.restore();

    // Role Indicator Tag
    const isMe = (currentRole === roleKey);
    const isHost = (latestState && (latestState.hostRole === roleKey || (!latestState.hostRole && roleKey === 'p1')));
    ctx.font = "bold 11px sans-serif";
    ctx.textAlign = "center";
    if (isRespawning) {
      ctx.fillStyle = "#ffb300";
      const sLeft = Math.ceil((h.respawnTicks || 30) * 0.13);
      ctx.fillText("⏳ RESPAWN (" + sLeft + "s)", hx, hy - 15);
    } else if (isMe) {
      ctx.fillStyle = "#00f0ff";
      ctx.fillText("▼ YOU", hx, hy - 15);
    } else if (isHost) {
      ctx.fillStyle = "#ffe600";
      ctx.fillText("👑 HOST", hx, hy - 15);
    } else {
      ctx.fillStyle = "#ffffff";
      const tag = (h.name && h.name.length > 0) ? h.name : defaultTag;
      ctx.fillText(tag.substring(0, 10), hx, hy - 15);
    }
  }

  function drawGhosts(state) {
    if (!state.ghosts || state.ghosts.length === 0) return;
    const ghostColors = ["#ff3333", "#ff99cc", "#00ffff", "#ffaa00"];
    const ghostNames = ["Blinky", "Pinky", "Inky", "Clyde"];
    const ghostRoleNames = ["blinky", "pinky", "inky", "clyde"];

    for (const g of state.ghosts) {
      const gx = g.x * C_SIZE + 15;
      const gy = g.y * C_SIZE + 15;
      const isFrightened = state.frightened;

      let bodyColor = ghostColors[g.color] || "#ff3333";
      if (isFrightened) {
        bodyColor = (state.frightenedTicks <= 8 && (state.frightenedTicks % 2 === 0)) ? "#ffffff" : "#2233cc";
      }

      ctx.save();
      ctx.fillStyle = bodyColor;
      ctx.beginPath();
      // Dome
      ctx.arc(gx, gy - 3, 11, Math.PI, 0, false);
      // Sides
      ctx.lineTo(gx + 11, gy + 10);
      // Wavy Skirt
      const waveOffset = Math.sin(animTick * 0.2) * 2;
      ctx.lineTo(gx + 6, gy + 7 + waveOffset);
      ctx.lineTo(gx, gy + 10);
      ctx.lineTo(gx - 6, gy + 7 - waveOffset);
      ctx.lineTo(gx - 11, gy + 10);
      ctx.closePath();
      ctx.fill();

      // Eyes
      if (!isFrightened) {
        ctx.fillStyle = "#ffffff";
        ctx.beginPath();
        ctx.arc(gx - 4.5, gy - 3, 3.8, 0, Math.PI * 2);
        ctx.arc(gx + 4.5, gy - 3, 3.8, 0, Math.PI * 2);
        ctx.fill();

        ctx.fillStyle = "#002288";
        let ox = (g.dir === 2 ? -1.5 : (g.dir === 3 ? 1.5 : 0));
        let oy = (g.dir === 0 ? -1.5 : (g.dir === 1 ? 1.5 : 0));
        ctx.beginPath();
        ctx.arc(gx - 4.5 + ox, gy - 3 + oy, 1.8, 0, Math.PI * 2);
        ctx.arc(gx + 4.5 + ox, gy - 3 + oy, 1.8, 0, Math.PI * 2);
        ctx.fill();
      } else {
        ctx.fillStyle = "#ffaaaa";
        ctx.fillRect(gx - 6, gy - 4, 3, 3);
        ctx.fillRect(gx + 3, gy - 4, 3, 3);
        ctx.strokeStyle = "#ffaaaa";
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.moveTo(gx - 7, gy + 4);
        ctx.lineTo(gx - 3, gy + 1.5);
        ctx.lineTo(gx, gy + 4);
        ctx.lineTo(gx + 3, gy + 1.5);
        ctx.lineTo(gx + 7, gy + 4);
        ctx.stroke();
      }
      ctx.restore();

      // Ghost Badge
      const isMe = (currentRole && currentRole.toLowerCase() === ghostRoleNames[g.color]);
      const isHost = (state.hostRole === ghostRoleNames[g.color]);
      ctx.font = "bold 11px sans-serif";
      ctx.textAlign = "center";
      if (isMe) {
        ctx.fillStyle = "#ff5577";
        ctx.fillText("▼ YOU (Ghost)", gx, gy - 16);
      } else if (isHost) {
        ctx.fillStyle = "#ffe600";
        ctx.fillText("👑 HOST (Ghost)", gx, gy - 16);
      } else if (g.isHuman && g.name) {
        ctx.fillStyle = bodyColor;
        ctx.fillText("👻 " + g.name.substring(0, 10), gx, gy - 16);
      } else {
        ctx.fillStyle = "#8a9ab8";
        ctx.fillText("👻 " + ghostNames[g.color], gx, gy - 16);
      }
    }
  }

  function applyFogOfWar(state) {
    if (!state.fog || state.peek) return;

    fogCtx.clearRect(0, 0, 630, 630);
    fogCtx.fillStyle = "rgba(4, 6, 15, 0.95)";
    fogCtx.fillRect(0, 0, 630, 630);

    // Carve out illuminated spotlights for alive & active heroes using destination-out
    fogCtx.globalCompositeOperation = 'destination-out';
    const heroes = [];
    if (state.p1 && (state.p1.alive || state.p1.respawnTicks > 0)) heroes.push(state.p1);
    if (state.p2 && state.p2.active && (state.p2.alive || state.p2.respawnTicks > 0)) heroes.push(state.p2);
    if (state.p3 && state.p3.active && (state.p3.alive || state.p3.respawnTicks > 0)) heroes.push(state.p3);
    if (state.p4 && state.p4.active && (state.p4.alive || state.p4.respawnTicks > 0)) heroes.push(state.p4);

    const rPx = (state.radius || 3.5) * C_SIZE;

    for (const h of heroes) {
      const hx = h.x * C_SIZE + 15;
      const hy = h.y * C_SIZE + 15;

      const grad = fogCtx.createRadialGradient(hx, hy, rPx * 0.25, hx, hy, rPx);
      grad.addColorStop(0, "rgba(0,0,0,1.0)");
      grad.addColorStop(0.7, "rgba(0,0,0,0.85)");
      grad.addColorStop(1, "rgba(0,0,0,0.0)");

      fogCtx.fillStyle = grad;
      fogCtx.beginPath();
      fogCtx.arc(hx, hy, rPx, 0, Math.PI * 2);
      fogCtx.fill();
    }

    fogCtx.globalCompositeOperation = 'source-over';
    ctx.drawImage(fogCanvas, 0, 0);
  }

  function drawOverlays(state) {
    if (state.peek) {
      ctx.fillStyle = "rgba(255, 200, 0, 0.25)";
      ctx.fillRect(0, 0, 630, 36);
      ctx.strokeStyle = "#ffe600";
      ctx.lineWidth = 1.5;
      ctx.strokeRect(0, 0, 630, 36);
      ctx.fillStyle = "#ffe600";
      ctx.font = "bold 15px sans-serif";
      ctx.textAlign = "center";
      ctx.fillText("👁️ FOG PEEK ACTIVE! FULL MAZE REVEALED", 315, 24);
    }

    if (state.gameOver) {
      ctx.fillStyle = "rgba(8, 2, 10, 0.88)";
      ctx.fillRect(0, 220, 630, 190);
      ctx.strokeStyle = "#ff3355";
      ctx.lineWidth = 3;
      ctx.strokeRect(0, 220, 630, 190);

      ctx.fillStyle = "#ff3355";
      ctx.font = "900 34px sans-serif";
      ctx.textAlign = "center";
      ctx.fillText("GAME OVER", 315, 300);
      ctx.fillStyle = "#ffffff";
      ctx.font = "16px sans-serif";
      ctx.fillText("Host can restart stage on main screen", 315, 345);
    } else if (state.levelComplete) {
      ctx.fillStyle = "rgba(2, 20, 10, 0.88)";
      ctx.fillRect(0, 220, 630, 190);
      ctx.strokeStyle = "#00ff88";
      ctx.lineWidth = 3;
      ctx.strokeRect(0, 220, 630, 190);

      ctx.fillStyle = "#00ff88";
      ctx.font = "900 32px sans-serif";
      ctx.textAlign = "center";
      ctx.fillText("STAGE COMPLETE! 🏆", 315, 300);
      ctx.fillStyle = "#ffffff";
      ctx.font = "16px sans-serif";
      ctx.fillText("Entering next labyrinth sector...", 315, 345);
    }
  }

  connect();
  requestAnimationFrame(render);
</script>
</body>
</html>)rawhtml";
}
