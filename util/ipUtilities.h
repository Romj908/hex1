/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ipUtilities.h
 * Author: jlaclavere
 *
 * Created on April 7, 2016, 4:38 PM
 */

#ifndef IPUTILITIES_H
#define IPUTILITIES_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */



void display_network_addresses();
int  get_ipv4_address(struct sockaddr_in *recipient, char *ifa_name);


#endif /* IPUTILITIES_H */

