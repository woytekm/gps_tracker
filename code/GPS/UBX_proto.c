#include "ubxmessage.h"
#include "tracker_includes.h" 
#include "tracker_routines.h"
 
void fletcherChecksum(unsigned char* buffer, int size, unsigned char* checkSumA, unsigned char* checkSumB)
{
    int i = 0;
    *checkSumA = 0;
    *checkSumB = 0;
    for(; i < size; i++)
    {
        *checkSumA += buffer[i];
        *checkSumB += *checkSumA;
    }
}
 
UBXMsgBuffer createBuffer(int payloadSize)
{
    UBXMsgBuffer buffer = {0, 0};
    buffer.size = UBX_HEADER_SIZE + payloadSize + UBX_CHECKSUM_SIZE;
    buffer.data = (char*)malloc(buffer.size);
    memset(buffer.data, 0, buffer.size);
    return buffer;
}
 
extern void clearUBXMsgBuffer(const UBXMsgBuffer* buffer)
{
    free(buffer->data);
}
 
void completeMsg(UBXMsgBuffer* buffer, int payloadSize)
{
    unsigned char* checkSumA = (unsigned char*)(buffer->data + UBX_HEADER_SIZE  + payloadSize);
    unsigned char* checkSumB = (unsigned char*)(buffer->data + UBX_HEADER_SIZE  + payloadSize + 1);
    fletcherChecksum((unsigned char*)(buffer->data + sizeof(UBX_PREAMBLE)), payloadSize + 4, checkSumA, checkSumB);
}
 
void initMsg(UBXMsg* msg, int payloadSize, UBXMessageClass msgClass, UBXMessageId msgId)
{
    msg->preamble = htobe16(UBX_PREAMBLE);
    msg->hdr.msgClass = msgClass;
    msg->hdr.msgId = msgId;
    msg->hdr.length = payloadSize;
}
 
UBXMsgBuffer getCFG_RXM(UBXU1_t lpMode)
{
    int payloadSize = sizeof(UBXCFG_RXM);
    UBXMsgBuffer buffer = createBuffer(payloadSize);
    UBXMsg* msg = (UBXMsg*)buffer.data;
    initMsg(msg, payloadSize, UBXMsgClassCFG, UBXMsgIdCFG_RXM);
    msg->payload.CFG_RXM.reserved1 = 8;
    msg->payload.CFG_RXM.lpMode = lpMode;
    completeMsg(&buffer, payloadSize);
    return buffer;
}
 
UBXMsgBuffer getCFG_RXM_POLL()
{
    int payloadSize = 0;
    UBXMsgBuffer buffer = createBuffer(payloadSize);
    UBXMsg* msg = (UBXMsg*)buffer.data;
    initMsg(msg, payloadSize, UBXMsgClassCFG, UBXMsgIdCFG_RXM);
    completeMsg(&buffer, payloadSize);
    return buffer;
}
 
UBXMsgBuffer getCFG_PM2_POLL()
{
    int payloadSize = 0;
    UBXMsgBuffer buffer = createBuffer(payloadSize);
    UBXMsg* msg = (UBXMsg*)buffer.data;
    initMsg(msg, payloadSize, UBXMsgClassCFG, UBXMsgIdCFG_PM2);
    completeMsg(&buffer, payloadSize);
    return buffer;
}
 
UBXMsgBuffer getCFG_PM2(UBXCFG_PM2Flags flags, UBXU4_t updatePeriod, UBXU4_t searchPeriod, UBXU4_t gridOffset, UBXU2_t onTime, UBXU2_t minAcqTime)
{
    int payloadSize = sizeof(UBXCFG_PM2);
    UBXMsgBuffer buffer = createBuffer(payloadSize);
    UBXMsg* msg = (UBXMsg*)buffer.data;
    initMsg(msg, payloadSize, UBXMsgClassCFG, UBXMsgIdCFG_PM2);
    msg->payload.CFG_PM2.flags = flags;
    msg->payload.CFG_PM2.updatePeriod = updatePeriod;
    msg->payload.CFG_PM2.searchPeriod = searchPeriod;
    msg->payload.CFG_PM2.gridOffset = gridOffset;
    msg->payload.CFG_PM2.onTime = onTime;
    msg->payload.CFG_PM2.minAcqTime = minAcqTime;
    completeMsg(&buffer, payloadSize);
    return buffer;
}

