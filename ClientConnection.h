/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClientConnection.h
 * Author: jlaclavere
 *
 * Created on April 8, 2016, 12:32 PM
 */
#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <signal.h>
#include <list>  
#include <set>  
#include <mutex>  
#include "ClientServerRequest.h"

#include "ClientSocket.h"

class ClientConnection
{
    enum class CnxState {
        NO_SOCKET = 0,
        CNX_ERROR = 1,
        NO_CONNECTION = 2, // NO_CONNECTION has to be higher than CNX_ERROR
        CONNECTING,
        CONNECTED_NO_CREDENTIALS,
        CONNECTED,
        REGISTERING,
        REGISTERED,
        DISCONNECTING,     // having sent the DISCONNECT_REQ
        CLOSING,           // DISCONNECT_CNF 
        CNX_REJECTED,
        AUTH_REJECTED,
    };
    CnxState    cnx_state;
    
    struct sockaddr_in server_ip_addr;
    std::string        ip_interface_name;
    
    std::unique_ptr<ClientSocket> clientSocket; // unique pointer to the socket object.
    
    ConnectionCnf::Cyphering ciphering;
    CipheringKeyData         ciph_kea;
    
    /*
     * External events : Interface with other threads(UI in particular)
     */
    
    enum class CnxExtEvent : uint8_t {
        NONE = 0,
        NEW_CREDENTIALS,
    };
    
    std::set<CnxExtEvent> ext_event{};
    std::mutex            ext_mutext;
    
    void _setExtEvent(CnxExtEvent ev);
    void _handleOneExtEvent(CnxExtEvent ev);
    void _handleExtEvents();
    
    // logging/registration data. 
    std::string user_name;
    std::string user_passwd;
    std::string user_email;
    std::mutex  registration_mutext;
    
    /*
        one unique instance of this class is allowed. Use a Singleton pattern.
     */
    static ClientConnection *clientModeConnection; // the unique instance of the class.
    
    ClientConnection(ClientConnection&) = delete;  // prevent override of the copy constructor
    ClientConnection& operator=(ClientConnection&) = delete;  // prevent copy
    
    ClientConnection(ClientConnection&&) = delete;  // prevent move constructor (C++ 2011)
    ClientConnection& operator=(ClientConnection&&) = delete;  // prevent move (C++ 2011).
    
    ClientConnection() = delete;  // default constructor not used
    
    ClientConnection(const struct sockaddr_in &serverAddr, std::string &ip_interface_name);  // the constructor is private.
    
public:
    static void 
    createObject(const struct sockaddr_in &serverAddr, std::string &ip_interface_name)
    {
        if (clientModeConnection == nullptr)
            clientModeConnection = new ClientConnection( serverAddr, ip_interface_name);
        
    }
    
    static ClientConnection *
    object() 
    {
        return clientModeConnection;
    };
    
    virtual ~ClientConnection()
    {
        assert(clientModeConnection != nullptr);
        delete clientModeConnection;  
        clientModeConnection = nullptr;
    }
    
    /*************************************************************************** 
     * 
     *          Interface to the user interface thread 
     * 
     **************************************************************************/
public:
    
    void 
    RegistrationDataFromUserInterface(const std::string& uname, 
                                    const std::string& upasswd, 
                                    const std::string& email);
    
    void 
    sendMsgToServer(ClientServerMsgRequestUPtr&& msg_ptr);
    
    ClientServerMsgBodyPtr 
    receiveMsgFromServer();
    
    void StartConnectionToServer();
    
    void StopConnectionToServer( bool flush);
    
    /* poll all pending incoming/outcoming data, if any. This function doesn't block. */
    void 
    poll();
    
private:
    void 
    configureSocket();
    
    void 
    handle_server_message(ClientServerL1MsgPtr p_msg);
    
    void sendConnectionReq();
    void handleConnectionCnf(ClientServerL1MsgPtr p_msg);
    void handleConnectionRej(ClientServerL1MsgPtr p_msg);
    
    void sendRegistrationReq();
    void handleRegistrationCnf(ClientServerL1MsgPtr p_msg);
    void handleRegistrationRej(ClientServerL1MsgPtr p_msg);
    
    void sendUserLoginReq();
    void handleUserLoginCnf(ClientServerL1MsgPtr p_msg);
    void handleUserLoginRej(ClientServerL1MsgPtr p_msg);
    
    void handleDisconnectionReq(ClientServerL1MsgPtr p_msg);
    void handleDisconnectionCnf(ClientServerL1MsgPtr p_msg);
    void sendDisconnectionReq();
    void sendDisconnectionCnf();
    
    
protected:
    virtual void 
    setState(CnxState new_state);
    
    void 
    socketDisconnected(const socket_disconnection& disc);
    
    
    
public:
};

#endif
