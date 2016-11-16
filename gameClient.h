/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gameClient.h
 * Author: jlaclavere
 *
 * Created on April 9, 2016, 6:45 PM
 */

#ifndef GAMECLIENT_H
#define GAMECLIENT_H

#include <thread>


/**
 * Class handling the application in client mode.
 * 
 */

extern void client_main(const char *ip_interface_name = "lo");

extern void client_account_data(std::string uname, std::string upasswd);

#endif /* GAMECLIENT_H */

