
#include <string.h>
#include <time.h>

#include "bubble.h"
#include "utils.h"

#define PACKHEAD_MAGIC (0xaa)
#define PACKHEAD_SIZE STRUCT_MEMBER_POS(PackHead, pData)
#define GET_PACKSIZE(data_size) (STRUCT_MEMBER_POS(PackHead, pData) + (data_size))

#define BUF_SIZE (2 * 1024)

static PackHead* write_packhead(uint data_size, char cPackType, char *buffer)
{
    PackHead *packhead = (PackHead *)buffer;
    struct timespec timetic;

    packhead->cHeadChar = PACKHEAD_MAGIC;
    packhead->cPackType = cPackType;
    clock_gettime(CLOCK_MONOTONIC, &timetic);
    packhead->uiTicket = htonl(timetic.tv_sec + timetic.tv_nsec/1000);
    packhead->uiLength = htonl(STRUCT_MEMBER_POS(PackHead, pData)
                               - STRUCT_MEMBER_POS(PackHead, cPackType)
                               + data_size);

    return packhead;
}

static int send_user_creds(session_context_t *session, const char *username, const char *password)
{
    char buffer[GET_PACKSIZE(STRUCT_MEMBER_POS(MsgPackData, pMsg) + sizeof(UserVrf))];

    MsgPackData msg;
    memset(&msg, 0, sizeof(MsgPackData));
    msg.cMsgType[0] = MSGT_USERVRF;
    msg.uiLength = htonl(sizeof(msg.cMsgType) + sizeof(UserVrf));

    UserVrf usr_creds;
    memset(&usr_creds, 0, sizeof(UserVrf));
    strncpy((char*)usr_creds.sUserName, username, MAX_USERNAME_LEN);
    strncpy((char*)usr_creds.sPassWord, password, MAX_PASSWORD_LEN);

    PackHead* packhead = write_packhead(STRUCT_MEMBER_POS(MsgPackData, pMsg) + sizeof(UserVrf),
                                        PT_MSGPACK, buffer);
    memcpy(packhead->pData, &msg, STRUCT_MEMBER_POS(MsgPackData, pMsg));
    memcpy(packhead->pData + STRUCT_MEMBER_POS(MsgPackData, pMsg), &usr_creds, sizeof(UserVrf));

    int nbytes = session_send(session, buffer, sizeof(buffer));
    if (nbytes != (int)sizeof(buffer)) {
        ERR("Failed to send user credentials");
        return -1;
    }

    return 0;
}

static int check_packet_len(uint32_t uiPackLength, size_t expected_data_size)
{
    return uiPackLength == expected_data_size +
            (STRUCT_MEMBER_POS(PackHead, pData) -
             STRUCT_MEMBER_POS(PackHead, cPackType));
}

static int recv_verify_user_result(session_context_t *session)
{
    char buffer[GET_PACKSIZE(STRUCT_MEMBER_POS(MsgPackData, pMsg) + sizeof(UserVrfB))];

    int nbytes = session_read(session, buffer, sizeof(buffer));
    if (nbytes != (int)sizeof(buffer)) {
        ERR("Failed to get response");
        return -1;
    }

    PackHead *packhead = (PackHead *)buffer;

    if (0x00 != packhead->cPackType) {
        ERR("invalid packet type %02X", packhead->cPackType);
        return -1;
    }

    uint32_t uiPackLength = ntohl(packhead->uiLength);
    if (!check_packet_len(uiPackLength, sizeof(UserVrfB) +
                          STRUCT_MEMBER_POS(MsgPackData, pMsg))) {
        ERR("invalid packet size");
        return -1;
    }

    MsgPackData *msgpack = (MsgPackData *)packhead->pData;
    if (MSGT_USERVRF_B != msgpack->cMsgType[0]) {
        ERR("invalid message type %02X", msgpack->cMsgType[0]);
        return -1;
    }

    UserVrfB *vrfResult = (UserVrfB *)msgpack->pMsg;
    if (1 != vrfResult->bVerify) {
        ERR("invalid username/password");
        return -1;
    }

    return 0;
}

int verify_user(session_context_t *session, const char *username, const char *password)
{
    if (0 != send_user_creds(session, username, password)) {
        return -1;
    }

    return recv_verify_user_result(session);
}

int open_stream(session_context_t *session, unsigned int channel, unsigned int stream_id)
{
    char buffer[GET_PACKSIZE(sizeof(BubbleOpenStream))];

    BubbleOpenStream openStreamPack;
    memset(&openStreamPack, 0, sizeof(BubbleOpenStream));
    openStreamPack.uiChannel = channel;
    openStreamPack.uiStreamId = stream_id;
    openStreamPack.uiOpened = 1;

    PackHead *packhead = write_packhead(sizeof(BubbleOpenStream), PT_OPENSTREAM, buffer);
    memcpy(packhead->pData, &openStreamPack, sizeof(BubbleOpenStream));

    int nbytes = session_send(session, buffer, sizeof(buffer));
    if (nbytes != (int)sizeof(buffer)) {
        ERR("Failed to send open stream packet");
        return -1;
    }

    return 0;
}

int receive_packet(session_context_t *session, void *buffer, size_t buffer_size)
{
    if (buffer_size < PACKHEAD_SIZE) {
        ERR("too small buffer");
        return -1;
    }

    int nbytes = session_read(session, buffer, PACKHEAD_SIZE);
    if (nbytes != (int)PACKHEAD_SIZE) {
        ERR("corrupted packet header");
        return -1;
    }

    PackHead* packhead = (PackHead*)buffer;
    if (0x08 == packhead->cPackType) {
        ERR("server is full");
        return -2;
    }

    uint32_t uiPackLength = ntohl(packhead->uiLength);
    size_t packetSize = uiPackLength + STRUCT_MEMBER_POS(PackHead, cPackType);
    if (packetSize < PACKHEAD_SIZE) {
        ERR("corrupted length field");
        return -1;
    }

    if (packetSize > buffer_size) {
        ERR("not enough buffer (%zu/%zu)", buffer_size, packetSize);
        return -1;
    }

    size_t bytes_left = packetSize - PACKHEAD_SIZE;
    char *recv_start = (char *)buffer + PACKHEAD_SIZE;

    nbytes = session_read_full(session, recv_start, bytes_left);
    if (nbytes != (int)bytes_left) {
        ERR("corrupted packet data (%d/%zu)", nbytes, bytes_left);
        return -1;
    }

    return (int)packetSize;
}

