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
    for (ConnectedClient *c : clients) {
        if (c->role == role) return true;
    }
    return false;
}

QString GameServer::getRolePlayerName(const QString &role) const
{
    for (ConnectedClient *c : clients) {
        if (c->role == role) return c->playerName;
    }
    return "";
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
            emit playerTriggeredPeek(client->id);
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
    background: #090a14;
    color: #fff;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    overflow-x: hidden;
  }
  .app-container {
    width: 100%;
    max-width: 440px;
    padding: 16px;
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 100vh;
  }
  h1 {
    font-size: 22px;
    font-weight: 900;
    letter-spacing: 1.5px;
    text-align: center;
    background: linear-gradient(90deg, #ffe600, #ff0077, #00f0ff);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    margin-bottom: 2px;
  }
  .subtitle {
    font-size: 11px;
    letter-spacing: 2px;
    color: #8b9bb4;
    text-transform: uppercase;
    margin-bottom: 16px;
  }
  .card {
    background: #14172b;
    border: 1px solid #232a4e;
    border-radius: 16px;
    padding: 16px;
    width: 100%;
    box-shadow: 0 10px 25px rgba(0,0,0,0.5);
    margin-bottom: 14px;
  }
  .input-group {
    margin-bottom: 14px;
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
    padding: 12px 14px;
    border-radius: 10px;
    background: #0d0f1e;
    border: 1px solid #2c355f;
    color: #fff;
    font-size: 15px;
    font-weight: 600;
    outline: none;
  }
  .input-group input:focus {
    border-color: #00e5ff;
    box-shadow: 0 0 10px rgba(0,229,255,0.4);
  }
  .section-title {
    font-size: 12px;
    font-weight: 800;
    letter-spacing: 1.5px;
    text-transform: uppercase;
    margin: 12px 0 8px 0;
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .role-grid {
    display: grid;
    grid-template-columns: 1fr;
    gap: 8px;
    width: 100%;
  }
  .role-btn {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 10px 14px;
    border-radius: 12px;
    border: 1.5px solid #27305c;
    background: #0f1224;
    color: #fff;
    cursor: pointer;
    transition: all 0.15s ease;
  }
  .role-btn:active:not(:disabled) {
    transform: scale(0.97);
  }
  .role-info {
    display: flex;
    align-items: center;
    gap: 10px;
  }
  .role-icon {
    font-size: 22px;
  }
  .role-name {
    font-size: 14px;
    font-weight: 700;
    text-align: left;
  }
  .role-sub {
    font-size: 10px;
    color: #8395b0;
    text-align: left;
  }
  .role-badge {
    font-size: 11px;
    font-weight: 700;
    padding: 4px 10px;
    border-radius: 20px;
    background: #1c2242;
    color: #00ffcc;
  }
  .role-btn:disabled {
    opacity: 0.45;
    cursor: not-allowed;
    border-color: #1a1e36;
  }
  .role-btn:disabled .role-badge {
    background: #2a1b24;
    color: #ff5577;
  }

  /* Controller Screen */
  #controllerScreen {
    display: none;
    width: 100%;
    flex-direction: column;
    align-items: center;
    height: 100%;
    flex: 1;
    justify-content: space-between;
  }
  .hero-header {
    width: 100%;
    background: #14172b;
    border: 1px solid #283363;
    border-radius: 14px;
    padding: 12px 16px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    box-shadow: 0 4px 15px rgba(0,0,0,0.4);
    margin-bottom: 12px;
  }
  .hero-left {
    display: flex;
    align-items: center;
    gap: 10px;
  }
  .hero-tag {
    font-size: 16px;
    font-weight: 800;
  }
  .hero-pname {
    font-size: 11px;
    color: #8b9bb4;
  }
  .status-pill {
    font-size: 11px;
    font-weight: 800;
    padding: 4px 10px;
    border-radius: 12px;
    background: #00ff8822;
    color: #00ff88;
    border: 1px solid #00ff8866;
  }
  .status-pill.frightened {
    background: #00e5ff22;
    color: #00e5ff;
    border-color: #00e5ff;
    animation: pulse 0.6s infinite alternate;
  }
  @keyframes pulse { from { opacity: 0.6; } to { opacity: 1; } }

  /* D-Pad controls */
  .dpad-container {
    width: 250px;
    height: 250px;
    position: relative;
    margin: 15px auto;
  }
  .d-btn {
    position: absolute;
    width: 76px;
    height: 76px;
    border-radius: 20px;
    background: #191f3e;
    border: 2px solid #36447e;
    color: #00f0ff;
    font-size: 26px;
    display: flex;
    align-items: center;
    justify-content: center;
    box-shadow: 0 6px 14px rgba(0,0,0,0.5);
    cursor: pointer;
    touch-action: manipulation;
  }
  .d-btn:active {
    background: #00e5ff;
    color: #090a14;
    border-color: #fff;
    transform: scale(0.92);
    box-shadow: 0 0 20px #00e5ff;
  }
  .btn-up { top: 0; left: 87px; }
  .btn-down { bottom: 0; left: 87px; }
  .btn-left { top: 87px; left: 0; }
  .btn-right { top: 87px; right: 0; }
  .dpad-center {
    position: absolute;
    top: 87px;
    left: 87px;
    width: 76px;
    height: 76px;
    border-radius: 20px;
    background: #111428;
    border: 1px dashed #283363;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 20px;
    color: #4b588c;
  }

  .action-btn {
    width: 100%;
    padding: 14px;
    border-radius: 14px;
    border: none;
    font-size: 15px;
    font-weight: 800;
    letter-spacing: 1px;
    cursor: pointer;
    box-shadow: 0 6px 18px rgba(0,0,0,0.4);
    margin-top: 10px;
    transition: transform 0.1s;
  }
  .action-btn:active { transform: scale(0.97); }
  .btn-peek {
    background: linear-gradient(135deg, #00d2ff, #0066ff);
    color: #fff;
    box-shadow: 0 6px 18px rgba(0,150,255,0.4);
  }
  .btn-ghost-status {
    background: #1e1329;
    border: 1px solid #5b2880;
    color: #da70d6;
    cursor: default;
  }
  .leave-btn {
    background: transparent;
    border: 1px solid #323b66;
    color: #7b8baa;
    font-size: 12px;
    font-weight: 600;
    padding: 8px 16px;
    border-radius: 8px;
    margin-top: 12px;
    cursor: pointer;
  }
</style>
</head>
<body>

<div class="app-container">
  <h1>PAC-MAN LABYRINTH</h1>
  <div class="subtitle">Classroom Multiplayer</div>

  <!-- LOBBY SCREEN -->
  <div id="lobbyScreen" style="width:100%;">
    <div class="card">
      <div class="input-group">
        <label>Your Name</label>
        <input type="text" id="nameInput" placeholder="Enter name (e.g. Alex)" maxlength="15">
      </div>

      <div class="section-title" style="color: #ff99cc;">🟡 Pac-Family Co-op (Mode A)</div>
      <div class="role-grid">
        <button class="role-btn" id="btn-p2" onclick="selectRole('p2')">
          <div class="role-info"><span class="role-icon">🎀</span><div><div class="role-name" style="color:#ff69b4;">Ms. Pac-Man</div><div class="role-sub">Pink Hero with Red Bow</div></div></div>
          <span class="role-badge" id="badge-p2">JOIN</span>
        </button>
        <button class="role-btn" id="btn-p3" onclick="selectRole('p3')">
          <div class="role-info"><span class="role-icon">🧢</span><div><div class="role-name" style="color:#00e5ff;">Jr. Pac-Man</div><div class="role-sub">Cyan Hero with Cap</div></div></div>
          <span class="role-badge" id="badge-p3">JOIN</span>
        </button>
        <button class="role-btn" id="btn-p4" onclick="selectRole('p4')">
          <div class="role-info"><span class="role-icon">🟢</span><div><div class="role-name" style="color:#00ff66;">Miracle Pac-Man</div><div class="role-sub">Emerald Green Hero</div></div></div>
          <span class="role-badge" id="badge-p4">JOIN</span>
        </button>
      </div>

      <div class="section-title" style="color: #ff4466;">👻 Playable Ghost Hunters (Mode B)</div>
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

  <!-- CONTROLLER SCREEN -->
  <div id="controllerScreen">
    <div class="hero-header">
      <div class="hero-left">
        <span id="activeIcon" style="font-size:26px;">🎀</span>
        <div>
          <div class="hero-tag" id="activeTitle">Ms. Pac-Man</div>
          <div class="hero-pname" id="activeName">Player</div>
        </div>
      </div>
      <div class="status-pill" id="statusPill">CONNECTED</div>
    </div>

    <!-- TOUCH D-PAD -->
    <div class="dpad-container">
      <button class="d-btn btn-up" ontouchstart="sendDir(0, event)" onclick="sendDir(0)">▲</button>
      <button class="d-btn btn-left" ontouchstart="sendDir(2, event)" onclick="sendDir(2)">◀</button>
      <div class="dpad-center" id="centerIcon">🕹️</div>
      <button class="d-btn btn-right" ontouchstart="sendDir(3, event)" onclick="sendDir(3)">▶</button>
      <button class="d-btn btn-down" ontouchstart="sendDir(1, event)" onclick="sendDir(1)">▼</button>
    </div>

    <!-- ACTION BUTTON -->
    <button class="action-btn btn-peek" id="actionBtn" onclick="sendAction()">👁️ FOG PEEK (GLIMPSE)</button>

    <button class="leave-btn" onclick="leaveRole()">Switch Character / Leave</button>
  </div>
</div>

<script>
  let ws = null;
  let currentRole = null;
  let myName = localStorage.getItem('pacman_pname') || '';
  if (myName) document.getElementById('nameInput').value = myName;

  const roleMeta = {
    p2: { icon: "🎀", name: "Ms. Pac-Man", color: "#ff69b4", isHero: true },
    p3: { icon: "🧢", name: "Jr. Pac-Man", color: "#00e5ff", isHero: true },
    p4: { icon: "🟢", name: "Miracle Pac-Man", color: "#00ff66", isHero: true },
    blinky: { icon: "🔴", name: "Blinky (Red Ghost)", color: "#ff3333", isHero: false },
    pinky: { icon: "🌸", name: "Pinky (Pink Ghost)", color: "#ff99cc", isHero: false },
    inky: { icon: "🩵", name: "Inky (Cyan Ghost)", color: "#00ffff", isHero: false },
    clyde: { icon: "🟠", name: "Clyde (Orange Ghost)", color: "#ffaa00", isHero: false }
  };

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
          showController(msg.role, msg.name);
        } else if (msg.type === 'reject') {
          alert(msg.reason || "Slot unavailable!");
        } else if (msg.type === 'gameState') {
          handleGameState(msg);
        }
      } catch (err) {}
    };

    ws.onclose = () => {
      document.getElementById('statusPill').innerText = "DISCONNECTED";
      document.getElementById('statusPill').style.color = "#ff5577";
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

  function showController(role, name) {
    const meta = roleMeta[role] || { icon: "🎮", name: "Player", color: "#fff", isHero: true };
    document.getElementById('lobbyScreen').style.display = 'none';
    document.getElementById('controllerScreen').style.display = 'flex';
    document.getElementById('activeIcon').innerText = meta.icon;
    document.getElementById('activeTitle').innerText = meta.name;
    document.getElementById('activeTitle').style.color = meta.color;
    document.getElementById('activeName').innerText = name;
    document.getElementById('centerIcon').innerText = meta.icon;

    const actionBtn = document.getElementById('actionBtn');
    if (meta.isHero) {
      actionBtn.className = "action-btn btn-peek";
      actionBtn.innerText = "👁️ FOG PEEK (GLIMPSE)";
      actionBtn.style.display = "block";
    } else {
      actionBtn.className = "action-btn btn-ghost-status";
      actionBtn.innerText = "🎯 HUNT PAC-MAN!";
      actionBtn.style.display = "block";
    }
  }

  function sendDir(dir, ev) {
    if (ev) ev.preventDefault();
    if (navigator.vibrate) navigator.vibrate(25);
    if (ws && ws.readyState === WebSocket.OPEN && currentRole) {
      ws.send(JSON.stringify({ type: 'move', dir: dir }));
    }
  }

  function sendAction() {
    if (navigator.vibrate) navigator.vibrate(40);
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
    document.getElementById('controllerScreen').style.display = 'none';
    document.getElementById('lobbyScreen').style.display = 'block';
  }

  function handleGameState(state) {
    const pill = document.getElementById('statusPill');
    const actionBtn = document.getElementById('actionBtn');
    if (state.frightened) {
      pill.innerText = "ENERGIZED!";
      pill.classList.add('frightened');
      if (currentRole && !roleMeta[currentRole].isHero) {
        actionBtn.innerText = "😱 DANGER! RUN FROM PAC-MAN!";
        actionBtn.style.background = "#0044aa";
        actionBtn.style.color = "#00ffff";
      }
    } else {
      pill.innerText = "LIVE";
      pill.classList.remove('frightened');
      if (currentRole && !roleMeta[currentRole].isHero) {
        actionBtn.innerText = "🎯 HUNT PAC-MAN!";
        actionBtn.style.background = "#1e1329";
        actionBtn.style.color = "#da70d6";
      }
    }
  }

  // Keyboard navigation for laptop testers
  window.addEventListener('keydown', (e) => {
    if (!currentRole) return;
    if (e.key === 'ArrowUp' || e.key === 'w' || e.key === 'W') sendDir(0);
    else if (e.key === 'ArrowDown' || e.key === 's' || e.key === 'S') sendDir(1);
    else if (e.key === 'ArrowLeft' || e.key === 'a' || e.key === 'A') sendDir(2);
    else if (e.key === 'ArrowRight' || e.key === 'd' || e.key === 'D') sendDir(3);
    else if (e.key === ' ' || e.key === 'f' || e.key === 'F') sendAction();
  });

  connect();
</script>
</body>
</html>
)rawhtml";
}
