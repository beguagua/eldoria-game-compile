#ifndef ELDORIA_NET_H
#define ELDORIA_NET_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "raylib.h"

typedef struct EldoriaRemotePlayer {
    bool active;
    Vector3 position;
    float yaw;
    float health;
} EldoriaRemotePlayer;

typedef struct EldoriaNetwork {
    bool initialized;
    bool hosting;
    bool connected;
    intptr_t listenSocket;
    intptr_t peerSocket;
    int port;
    float sendTimer;
    char address[64];
    char status[128];
    EldoriaRemotePlayer remote;
} EldoriaNetwork;

void EldoriaNetInit(EldoriaNetwork *net);
bool EldoriaNetHost(EldoriaNetwork *net, int port);
bool EldoriaNetJoin(EldoriaNetwork *net, const char *address, int port);
void EldoriaNetUpdate(EldoriaNetwork *net, float dt, Vector3 localPosition, float localYaw, float localHealth);
void EldoriaNetStop(EldoriaNetwork *net);
const EldoriaRemotePlayer *EldoriaNetRemote(const EldoriaNetwork *net);
const char *EldoriaNetStatus(const EldoriaNetwork *net);

#endif
