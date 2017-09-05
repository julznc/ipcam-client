
#include "media.h"
#include "bubble.h"
#include "utils.h"

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

