#include "eldoria/net.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET EldoriaRawSocket;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int EldoriaRawSocket;
#endif

static intptr_t InvalidSocketValue(void)
{
    return (intptr_t)-1;
}
static bool IsInvalidRawSocket(EldoriaRawSocket value)
{
#ifdef _WIN32
    return value == INVALID_SOCKET;
#else
    return value < 0;
#endif
}

static void CloseSocketValue(intptr_t value)
{
    if (value == InvalidSocketValue()) return;
#ifdef _WIN32
    closesocket((EldoriaRawSocket)value);
#else
    close((int)value);
#endif
}

static bool SetNonBlocking(intptr_t value)
{
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket((EldoriaRawSocket)value, FIONBIO, &mode) == 0;
#else
    int flags = fcntl((int)value, F_GETFL, 0);
    return flags >= 0 && fcntl((int)value, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

static bool SocketWouldBlock(void)
{
#ifdef _WIN32
    int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS || error == WSAEALREADY;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS;
#endif
}

static void SetStatus(EldoriaNetwork *net, const char *status)
{
    snprintf(net->status, sizeof(net->status), "%s", status);
}

void EldoriaNetInit(EldoriaNetwork *net)
{
    memset(net, 0, sizeof(*net));
    net->listenSocket = InvalidSocketValue();
    net->peerSocket = InvalidSocketValue();
    net->port = 27815;
    snprintf(net->address, sizeof(net->address), "127.0.0.1");
#ifdef _WIN32
    WSADATA data;
    net->initialized = WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    net->initialized = true;
#endif
    SetStatus(net, net->initialized ? "Rede pronta." : "Não foi possível iniciar a rede.");
}

bool EldoriaNetHost(EldoriaNetwork *net, int port)
{
    if (!net->initialized) return false;
    EldoriaRawSocket socketValue = socket(AF_INET, SOCK_STREAM, 0);
    if (IsInvalidRawSocket(socketValue)) {
        SetStatus(net, "Falha ao criar o socket do host.");
        return false;
    }
    int reuse = 1;
    setsockopt(socketValue, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
    struct sockaddr_in endpoint;
    memset(&endpoint, 0, sizeof(endpoint));
    endpoint.sin_family = AF_INET;
    endpoint.sin_addr.s_addr = htonl(INADDR_ANY);
    endpoint.sin_port = htons((uint16_t)port);
    if (bind(socketValue, (struct sockaddr *)&endpoint, sizeof(endpoint)) != 0 || listen(socketValue, 1) != 0) {
        CloseSocketValue((intptr_t)socketValue);
        SetStatus(net, "Não foi possível abrir essa porta.");
        return false;
    }
    net->listenSocket = (intptr_t)socketValue;
    net->peerSocket = InvalidSocketValue();
    net->hosting = true;
    net->connected = false;
    net->port = port;
    SetNonBlocking(net->listenSocket);
    snprintf(net->address, sizeof(net->address), "0.0.0.0");
    SetStatus(net, "Hospedando: aguardando outro jogador...");
    return true;
}

bool EldoriaNetJoin(EldoriaNetwork *net, const char *address, int port)
{
    if (!net->initialized || address == NULL || address[0] == '\0') return false;
    EldoriaRawSocket socketValue = socket(AF_INET, SOCK_STREAM, 0);
    if (IsInvalidRawSocket(socketValue)) {
        SetStatus(net, "Falha ao criar o socket do cliente.");
        return false;
    }
    if (!SetNonBlocking((intptr_t)socketValue)) {
        CloseSocketValue((intptr_t)socketValue);
        SetStatus(net, "Falha ao configurar o socket do cliente.");
        return false;
    }
    struct sockaddr_in endpoint;
    memset(&endpoint, 0, sizeof(endpoint));
    endpoint.sin_family = AF_INET;
    endpoint.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, address, &endpoint.sin_addr) != 1) {
        CloseSocketValue((intptr_t)socketValue);
        SetStatus(net, "Endereço IPv4 inválido.");
        return false;
    }
    int rc = connect(socketValue, (struct sockaddr *)&endpoint, sizeof(endpoint));
    if (rc != 0 && !SocketWouldBlock()) {
        CloseSocketValue((intptr_t)socketValue);
        SetStatus(net, "Conexão recusada. Confira IP e porta.");
        return false;
    }
    fd_set writable;
    FD_ZERO(&writable);
    FD_SET(socketValue, &writable);
    struct timeval waitTime = { 2, 0 };
#ifdef _WIN32
    rc = select(0, NULL, &writable, NULL, &waitTime);
#else
    rc = select((int)socketValue + 1, NULL, &writable, NULL, &waitTime);
#endif
    if (rc <= 0 || !FD_ISSET(socketValue, &writable)) {
        CloseSocketValue((intptr_t)socketValue);
        SetStatus(net, "Tempo esgotado ao conectar.");
        return false;
    }
    int socketError = 0;
#ifdef _WIN32
    int errorLength = (int)sizeof(socketError);
#else
    socklen_t errorLength = (socklen_t)sizeof(socketError);
#endif
    getsockopt(socketValue, SOL_SOCKET, SO_ERROR, (char *)&socketError, &errorLength);
    if (socketError != 0) {
        CloseSocketValue((intptr_t)socketValue);
        SetStatus(net, "O host não aceitou a conexão.");
        return false;
    }
    net->peerSocket = (intptr_t)socketValue;
    net->listenSocket = InvalidSocketValue();
    net->hosting = false;
    net->connected = true;
    net->port = port;
    snprintf(net->address, sizeof(net->address), "%s", address);
    SetStatus(net, "Conectado ao host.");
    return true;
}

static void DisconnectPeer(EldoriaNetwork *net, const char *status)
{
    CloseSocketValue(net->peerSocket);
    net->peerSocket = InvalidSocketValue();
    net->connected = false;
    net->remote.active = false;
    SetStatus(net, status);
}

void EldoriaNetUpdate(EldoriaNetwork *net, float dt, Vector3 localPosition, float localYaw, float localHealth)
{
    if (!net->initialized) return;
    if (net->hosting && !net->connected && net->listenSocket != InvalidSocketValue()) {
        struct sockaddr_in peerAddress;
#ifdef _WIN32
        int peerLength = (int)sizeof(peerAddress);
#else
        socklen_t peerLength = (socklen_t)sizeof(peerAddress);
#endif
        EldoriaRawSocket peer = accept((EldoriaRawSocket)net->listenSocket, (struct sockaddr *)&peerAddress, &peerLength);
        if (!IsInvalidRawSocket(peer)) {
            net->peerSocket = (intptr_t)peer;
            SetNonBlocking(net->peerSocket);
            net->connected = true;
            net->remote.active = true;
            SetStatus(net, "Jogador conectado.");
        }
    }
    if (!net->connected || net->peerSocket == InvalidSocketValue()) return;

    char packet[128];
#ifdef _WIN32
    int received = recv((EldoriaRawSocket)net->peerSocket, packet, (int)sizeof(packet) - 1, 0);
#else
    int received = (int)recv((int)net->peerSocket, packet, sizeof(packet) - 1, MSG_DONTWAIT);
#endif
    if (received > 0) {
        packet[received] = '\0';
        float x, y, z, yaw, health;
        if (sscanf(packet, "P %f %f %f %f %f", &x, &y, &z, &yaw, &health) == 5) {
            net->remote.position = (Vector3){x, y, z};
            net->remote.yaw = yaw;
            net->remote.health = health;
            net->remote.active = true;
        }
    } else if (received == 0 || (received < 0 && !SocketWouldBlock())) {
        DisconnectPeer(net, "O outro jogador saiu da sessão.");
        return;
    }

    net->sendTimer -= dt;
    if (net->sendTimer <= 0.0f) {
        int length = snprintf(packet, sizeof(packet), "P %.3f %.3f %.3f %.4f %.1f\n", localPosition.x, localPosition.y, localPosition.z, localYaw, localHealth);
#ifdef _WIN32
        int sent = send((EldoriaRawSocket)net->peerSocket, packet, length, 0);
#else
        int sent;
#ifdef MSG_NOSIGNAL
        sent = (int)send((int)net->peerSocket, packet, (size_t)length, MSG_NOSIGNAL);
#else
        sent = (int)send((int)net->peerSocket, packet, (size_t)length, 0);
#endif
#endif
        if (sent < 0 && !SocketWouldBlock()) DisconnectPeer(net, "A conexão foi interrompida.");
        net->sendTimer = 0.08f;
    }
}

void EldoriaNetStop(EldoriaNetwork *net)
{
    if (net == NULL) return;
    CloseSocketValue(net->peerSocket);
    CloseSocketValue(net->listenSocket);
    net->peerSocket = InvalidSocketValue();
    net->listenSocket = InvalidSocketValue();
    net->connected = false;
    net->hosting = false;
    net->remote.active = false;
#ifdef _WIN32
    if (net->initialized) WSACleanup();
#endif
    net->initialized = false;
    SetStatus(net, "Rede encerrada.");
}

const EldoriaRemotePlayer *EldoriaNetRemote(const EldoriaNetwork *net)
{
    return net == NULL ? NULL : &net->remote;
}

const char *EldoriaNetStatus(const EldoriaNetwork *net)
{
    return net == NULL ? "Rede indisponível." : net->status;
}
