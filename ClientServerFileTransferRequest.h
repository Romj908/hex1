/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClientServerFileTransferRequest.h
 * Author: jlaclavere
 *
 * Created on May 9, 2016, 5:55 AM
 */

#ifndef CLIENTSERVERFILETRANSFERREQUEST_H
#define CLIENTSERVERFILETRANSFERREQUEST_H

#include <iostream>
#include <cstdlib>
#include <cstdarg>
#include <string>

#include "ClientServerRequest.h"

/*******************************************************************************
 * 
 * ClientServerTransferRequest : 
 * The request is to transfer some long data in several messages.
 * This is an abstract class
 *******************************************************************************/

class ClientServerTransferRequest : public ClientServerRequest
{
protected:    
    DataTransferId  transfer_id; // the transaction identifier, the same during all the transfer.
    unsigned short  l2_action;         // some additionnal related upper level's private information. temporary.
    
    ClientServerMsgBodyPtr msg_ptr; // the buffer containing the current message is not dynamicaly allocated, 
                                            // so keep a shared_pointer on it to keep it safe.
    
private:
    // the default constructor is not visible.
    ClientServerTransferRequest() = default;  
    
    // copy constructor not visible
    ClientServerTransferRequest(const ClientServerTransferRequest& orig) = delete;
    // move constructor not visible
    ClientServerTransferRequest(ClientServerTransferRequest&& orig) = delete;
    
protected:    
    static DataTransferId next_transfer_id;
    static DataTransferId nextTransferId() { return next_transfer_id++; }
    
    // the constructor to be used.
    ClientServerTransferRequest(size_t nb_bytes, unsigned short l2_action = 0 )
    : ClientServerRequest(nb_bytes), l2_action(l2_action)
    {
        msg_ptr.reset(new ClientServerMsgBody); /* smart ptr => no delete required in the destructor.*/
    }
    
    virtual ~ClientServerTransferRequest() = default;
    
//    virtual char *
//    getNextDataBlock(size_t &nb_bytes) = 0;
    
};

/**
 * transfer a file of any size. It is split in several messages if required.
 */
class ClientServerFileTransferRequest : public ClientServerTransferRequest
{
protected:    
    FILE *file;
    std::string file_name;
    
    void    rewind() const;
    bool    eof() const;
    int     file_size(void) const;
    ssize_t file_read_data(char *to, size_t nb_bytes);
    
//    virtual char *
//    getNextDataBlock(size_t &nb_bytes) override;
    
private:
    // the default constructor is not visible.
    ClientServerFileTransferRequest() = default;
    
public:
    // the constructor to be used.
    ClientServerFileTransferRequest(std::string f_name, unsigned short l2_action = 0);
    
    virtual ~ClientServerFileTransferRequest();
        
 public:
   virtual ClientServerMsgBodyPtr 
   buildNextL2Msg(ClientServerL1MessageId &l1_msg_id, int &length) override;
     
};

/*
 * send a long message whose smart_ptr is given. The message is not in a file but
 * in a contiguous area of memory.
 * There is no limit in the lenght,  the data will be split into several messages.
 */
class ClientServerMsgTransferRequest : public ClientServerTransferRequest
{
protected:    
    const char *source_buffer;  // the address where the message to send is stored.
        
//    virtual char *
//    getNextDataBlock(size_t &nb_bytes) override;
    
private:
    // the default constructor is not visible.
    ClientServerMsgTransferRequest() = default;
    
public:
    // the constructor to be used.
    ClientServerMsgTransferRequest(const char *buffer, size_t lenght, unsigned short l2_action = 0);
    
    virtual ~ClientServerMsgTransferRequest();
        
 public:
    virtual ClientServerMsgBodyPtr 
    buildNextL2Msg(ClientServerL1MessageId &l1_msg_id, int &length) override;
     
};


#endif /* CLIENTSERVERFILETRANSFERREQUEST_H */

