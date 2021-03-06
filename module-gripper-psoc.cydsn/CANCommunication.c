/* ========================================
 *
 * Copyright BioBot Instrumentation, 2015
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF BioBot Instrumentation.
 *
 * WRITTEN BY : Philippe-Antoine Major &
 *              Louis-Vincent Major
 * ========================================
*/

#include "CANCommunication.h"


int8 can_sendMessage(CAN_MSG* msgPtr, uint32 CAN_ID);


// Retrieve the messages in the order they were received by the master. (FIFO)
CAN_MSG can_getReceivedMessage()
{
    CAN_MSG msgToTreat = receivedMsgs[msgToBeTreatedIndex];
    msgToBeTreatedIndex = (msgToBeTreatedIndex+1) % MAX_MSG_BUFFER_SIZE;
    messageToBeTreatedCount -= 1;
    
    return msgToTreat;
}

void sendDataToCAN(uint8 instruction, uint8 byte1, uint8 byte2, uint8 byte3, uint8 byte4, uint8 byte5, uint8 byte6, uint8 byte7)
{
    CAN_MSG msg = {{  instruction,
                      byte1,byte2,byte3,byte4,byte5,byte6,byte7},
                      8,{0xFF}}; // Sets the first bytes of the message.
    can_sendMessage(&msg, CAN_COMPUTER_ID);  
}


// Send a message on the CAN Bus.
int8 can_sendMessage(CAN_MSG* msgPtr, uint32 CAN_ID)
{
    CAN_1_TX_MSG msgToSend;
    
    msgToSend.id = CAN_ID;   
    msgToSend.rtr = MSG_RTR;
    msgToSend.ide = MSG_IDE;
    msgToSend.irq = MSG_IRQ;
    msgToSend.msg = &msgPtr->msgBytes;
    msgToSend.dlc = msgPtr->DLC;
    
    CAN_1_SendMsg(&msgToSend);
    return 1;
}

/* [] END OF FILE */
