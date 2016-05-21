/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClientServerFileTransferRequest.cpp
 * Author: jlaclavere
 * 
 * Created on May 9, 2016, 5:55 AM
 */
#include <cstring>     
#include <libexplain/fopen.h>     
#include <libexplain/fclose.h>     
#include <libexplain/read.h>     
#include "ClientServerFileTransferRequest.h"


DataTransferId ClientServerTransferRequest::next_transfer_id = 1;

bool ClientServerFileTransferRequest::eof() const
{
    return ::feof(file);
}

void ClientServerFileTransferRequest::rewind() const
{
    ::fseek(file,0,SEEK_SET);
}

int ClientServerFileTransferRequest::file_size(void) const
{
    int save_pos = ::ftell(file);
    ::fseek(file,0,SEEK_END);
    int end_pos = ::ftell(file);
    ::fseek(file,save_pos,SEEK_SET);
    return end_pos;
    
}

ssize_t ClientServerFileTransferRequest::file_read_data(char *to, size_t nb_bytes)
{
    ssize_t nb = ::read(fileno(this->file), to, nb_bytes);
    if (nb < 0)
    {
        ::fprintf(stderr, "%s\n", explain_read(fileno(this->file), to, nb_bytes));
        throw("ClientServerFileTransferRequest::read_data");
    }
    return nb;
}


ClientServerFileTransferRequest::
ClientServerFileTransferRequest(std::string f_name, 
                                unsigned short l2_action)
:       ClientServerRequest(0),             /* set temporarily the size of the transfer to 0. */
        file_name(f_name), 
        buffer_pointer(new ClientServerMsg) /* shared_ptr => no delete required in the destructor.*/
{
    // try to open the target file at object construction.
    file = ::fopen(file_name.c_str(), "r");
    
    if (file == NULL)
    {
        ::fprintf(stderr, "%s\n", explain_fopen(file_name.c_str(), "r"));
        throw("ClientServerFileTransferRequest"); // panic
    }
    
    // set the correct number of bytes to transfer to finalize the init.
    this->l2_payload_length = this->file_size();
}
        
ClientServerFileTransferRequest::~ClientServerFileTransferRequest() 
{
    if (::fclose(this->file))
    {
        ::fprintf(stderr, "%s\n", explain_fclose(this->file));
        throw("~ClientServerFileTransferRequest"); // temp
    }
}


ClientServerMsgPointer 
ClientServerFileTransferRequest::buildNextMsg(ClientServerL1MessageId &msg_id, int &length)
{

    if (this->someDataToSend())
    {
        if (this->nb_payload_bytes_sent == 0)
        {
            // it's the begin of the transfer. The first block is the transfer descriptor.
            msg_id = ClientServerL1MessageId::FILE_TRANSFER_REQ;
            FileTransferReq *transfer_req = &buffer_pointer->file_transfer_req;
            ::memset(transfer_req->header.file_name, ' ', CLIENTSERVER_FILE_NAME_LENGTH);
            
            // copy the file's name into the message
            this->file_name.copy(transfer_req->header.file_name,CLIENTSERVER_FILE_NAME_LENGTH);
            
            // set the size of the file
            transfer_req->header.file_size = htonl(this->l2_payload_length); // total size.
            
            // additionnal information
            transfer_req->header.transfer_id = htonl(this->transfer_id);
            transfer_req->header.l2_action = htons(this->l2_action);
            
            ssize_t nb = this->file_read_data(transfer_req->l2_payload, 
                                        sizeof(transfer_req->l2_payload));
            this->nb_payload_bytes_sent = nb;
            
            length = static_cast<int>(nb) + sizeof(FileTransferReqHeader);
            
        }
        else
        {
            // the transfer alread began. The next message is then a data block.
            msg_id = ClientServerL1MessageId::TRANSFER_DATA_BLK;
            DataTransferBlk *data_blk = &buffer_pointer->data_transfer_blk;
            data_blk->header.byte_position = htonl(this->nb_payload_bytes_sent);
            data_blk->header.transfer_id = htonl(this->transfer_id);
            
            ssize_t nb = this->file_read_data(data_blk->l2_payload, sizeof(data_blk->l2_payload));
            
            nb_payload_bytes_sent += nb;
            length = static_cast<int>(nb) + sizeof(DataTransferHeader);
            
        }
        // chack that no more than the file size bytes have been sent.
        assert(nb_payload_bytes_sent <= l2_payload_length);
        
    }
    return this->buffer_pointer;
}


ClientServerMsgPointer 
ClientServerMsgTransferRequest::buildNextMsg(ClientServerL1MessageId &msg_id, int &length)
{

    if (this->someDataToSend())
    {
        if (this->nb_payload_bytes_sent == 0)
        {
            // it's the begin of the transfer. The first block is the transfer descriptor.
            msg_id = ClientServerL1MessageId::DATA_TRANSFER_REQ;
            DataTransferReq *data_transfer_req = &buffer_pointer->data_transfer_req;
                        
            // set the size of the file
            data_transfer_req->header.total_size = htonl(this->l2_payload_length); // total size.
            
            // additionnal information
            data_transfer_req->header.transfer_id = htonl(this->transfer_id);
            data_transfer_req->header.l2_action = htons(this->l2_action);
            
            ssize_t nb = (this->l2_payload_length - this->nb_payload_bytes_sent);
            if (nb > sizeof(data_transfer_req->l2_payload))
            {
                nb = sizeof(data_transfer_req->l2_payload);
            }
            ::memcpy(data_transfer_req->l2_payload, 
                    this->source_buffer, 
                    nb);
            
            this->nb_payload_bytes_sent = nb;
            length = static_cast<int>(nb) + sizeof(FileTransferReqHeader);
            
        }
        else
        {
            // the transfer alread began. The next message is then a data block.
            msg_id = ClientServerL1MessageId::TRANSFER_DATA_BLK;
            DataTransferBlk *data_blk = &buffer_pointer->data_transfer_blk;
            data_blk->header.byte_position = htonl(this->nb_payload_bytes_sent);
            data_blk->header.transfer_id = htonl(this->transfer_id);
            
            ssize_t nb = (this->l2_payload_length - this->nb_payload_bytes_sent);
            if (nb > sizeof(data_blk->l2_payload))
            {
                nb = sizeof(data_blk->l2_payload);
            }
            ::memcpy(data_blk->l2_payload, 
                    this->source_buffer + this->nb_payload_bytes_sent, 
                    nb);
            
            nb_payload_bytes_sent += nb;
            length = static_cast<int>(nb) + sizeof(DataTransferHeader);
            
        }
        // chack that no more than the file size bytes have been sent.
        assert(nb_payload_bytes_sent <= l2_payload_length);
        
    }
    return this->buffer_pointer;
}