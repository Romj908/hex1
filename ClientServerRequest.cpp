/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClientServerRequest.cpp
 * Author: jlaclavere
 * 
 * Created on May 8, 2016, 10:24 PM
 */
#include <iostream>

#include "ClientServerRequest.h"

ClientServerMsgPointer 
ClientServerMsgRequest::buildNextMsg(ClientServerL1MessageId &msg_id, int &length)
{
    if (someDataToSend())
    {
        msg_id = this->l1_msg_id;
        length = this->l2_payload_length;
        nb_payload_bytes_sent += l2_payload_length;
        return msg_ptr;
    }
    else 
        return nullptr;
}

