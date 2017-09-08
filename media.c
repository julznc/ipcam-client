
#include "media.h"
#include "bubble.h"
#include "utils.h"

media_context_t *new_media(session_context_t *session)
{
    media_context_t *media = (media_context_t *)malloc(sizeof(media_context_t));
    if (NULL==media) {
        return NULL;
    }

    memset(media, 0, sizeof(media_context_t));
    media->session = session;

    media->display = new_display();
    if (NULL==media->display) {
        free(media);
        return NULL;
    }

    avcodec_register_all();
    return media;
}

void free_media(media_context_t *media)
{
    if (NULL==media) {
        return;
    }

    if (media->display) {
        free_display(media->display);
    }

    free(media);
}

int process_packet(void *packet)
{
    PackHead *packhead = (PackHead *)packet;
    uint32_t uiPackLen = ntohl(packhead->uiLength);
    size_t packsize = packsize = uiPackLen + STRUCT_MEMBER_POS(PackHead, cPackType);

    MediaPackData *media_packhead = (MediaPackData *)packhead->pData;
    uint32_t uiMediaPackLen = ntohl(media_packhead->uiLength);
    if (uiMediaPackLen + STRUCT_MEMBER_POS(PackHead, pData) +
            STRUCT_MEMBER_POS(MediaPackData, pData) > packsize) {
        ERR("Frame data is larger than packet size");
        return -1;
    }

    DBG("media packet chl: %d type: %d", media_packhead->cId, media_packhead->cMediaType);
    unsigned char *framedata = (unsigned char *)media_packhead->pData;
    switch (media_packhead->cMediaType)
    {
    case MT_IDR:
        DBG("MT_IDR");
        dumpbytes(framedata, uiMediaPackLen);
        break;
    case MT_PSLICE:
        DBG("MT_PSLICE");
        dumpbytes(framedata, uiMediaPackLen);
        break;
    case MT_AUDIO:
        DBG("MT_AUDIO");
        dumpbytes(framedata, uiMediaPackLen);
        break;
    default:
        ERR("unknown media pack type");
        return -1;
    }

    return 0;
}

