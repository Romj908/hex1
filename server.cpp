/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
void handle_client_message()
{
    // analyse the received message and route it accordingly
    ;
}



bool existing_connection()
{
    // check if the indicated connection is already know.
    return false;
}

void open_new_client_connection()
{
    // registrate a the new remote client 
    
    // send an answer to the client. First connection's state is for waiting for MSG2 correct tranmission.
    
    // create a new threat dedicated to that client. 
    // It will be waiting for input from the main socket through a wait queue 
}

void server_socket_receive()
{
        // poll on the main socket for new clients or for messages from registered clients.
        ;
        // is it a message from a new client or from a already registerd one ?
        if (0)
        {
            // message from a know client. A connection already exist for it.
            handle_client_message();
        }
        else
        {
            // it's a MSG1
            open_new_client_connection();
        }
    
}

void server_loop()
{
    while (1) // endless loop
    {
        try
        {
            // receive one message from any client
            server_socket_receive();
        }
        // handle typical socket exceptions (broken pipe,...))
        catch(...)
        {
            
        }
    }
}

void server_main()
{
    // initialize the server's main objects.
    //...
    ;
    
    // 
    try
    {
        server_loop();
    }
    // handling al the exceptions, in particular the shutdown of the server itself
    catch(...)
    {
        // 
    }
}

