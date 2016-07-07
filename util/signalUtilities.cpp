/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdlib.h>
#include <time.h>

#include <errno.h>
#include <assert.h>

#include <iostream>

void waitUsec(int usec)
{
    // divide the u-sec to determine the number of seconds and save the rest.
    div_t div_res = ::div(usec, 1000000);

    // prepare the wait in the ::nanosleep() format.)
    struct timespec req;
    req.tv_sec = div_res.quot ;
    req.tv_nsec = div_res.rem * 1000;
    
    struct timespec rem;
    
    do
    {
        int nsStatus = ::nanosleep( &req, &rem);
        if (nsStatus == -1)
        {
            // some signal occured. 
            assert (errno == EINTR);
            // continue the pause. The remaining time is is 'rem'.
            std::cout << "\nwaitUsec INTR, rem.tv_sec " << rem.tv_sec << ", rem.tv_nsec " <<rem.tv_nsec;
            req = rem;
        }
        else
        {
            break;
        }
        
    } while (1);
    
        
}
