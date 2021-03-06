#include "Q4SServerProtocol.h"

#include <stdio.h>
#include <vector>
#include <sstream>
#include <math.h>
//#include "ETime.h"
#include "../q4sCommon/Q4SMathUtils.h"
#include "Q4SServerConfigFile.h"
#include <sys/time.h>

//#include "EKey.h"
#include "../q4sCommon/Q4SMessage.h"
#include "../q4sCommon/Q4SMessageTools.h"

#define     DEFAULT_CONN_ID     1

Q4SServerProtocol::Q4SServerProtocol ()
{
    clear();
}

Q4SServerProtocol::~Q4SServerProtocol ()
{
    done();
}

bool Q4SServerProtocol::init()
{
    // Prevention done call
    //done();

    bool ok = true;
    bool okReceivedMessages = true;
    //flagEoS= false; 

    if (ok)
    {
     okReceivedMessages = mReceivedMessages.init( ); 
    }

    if (ok)
    {
        ok &= openConnectionListening(); 
    }

    if (ok)
    {
        ok &= mServerSocket.startAlertSender();
    }
    if (ok)
    {
        sem_init(&UDPSem, 0, 0);

    }

    return ok;
}

void Q4SServerProtocol::done()
{
    
    bool closedConnectionAlertSender = mServerSocket.closeConnection(SOCK_DGRAM);
    if (!closedConnectionAlertSender)
    {
        printf( "Error closing sender socket connection.\n" );
    }

    //closeConnectionListening();
    mReceivedMessages.done( );
    
}

void Q4SServerProtocol::clear()
{
    //lastAlertTimeStamp = ULONG_MAX;
}

bool Q4SServerProtocol::openConnectionListening()
{
    bool ok = true;
    int thread_error;
    /*
    thread_error= pthread_mutex_init(&mut_Timestamp, NULL);
    if (thread_error< 0)
        {
            printf("ERRor mut_Timestamp\n");
        }

    thread_error= pthread_mutex_init(&mut_flag, NULL);
    if (thread_error< 0)
        {
            printf("ERRor mut_Timestamp\n");
        }
    */

    thread_error = pthread_create( &marrthrListenHandle[0], NULL, manageTcpConnectionsFn, ( void* ) this);
    if (thread_error< 0)
        {
            printf("ERRor marrthrListenHandle[0]\n");
        }
        
    thread_error = pthread_create( &marrthrListenHandle[1], NULL,manageUdpReceivedDataFn , ( void* ) this);
    
    if (thread_error< 0)
        {
            printf("ERRor marrthrListenHandle[1]\n");
        }

    //thread_error = pthread_create( &marrthrListenHandle[2], NULL,checkConnectionsFn , ( void* ) this);
    
  
    return ok;
}


void Q4SServerProtocol::closeConnectionListening()
{
    bool ok = true;

    if( ok )
    {
        mServerSocket.stopWaiting( );
        pthread_join( marrthrListenHandle[0], NULL);
        pthread_join( marrthrListenHandle[1], NULL);
    }
     
        /*
        pthread_mutex_destroy(&mut_flag);
        pthread_mutex_destroy(&mut_Timestamp);
        */

        //pthread_join( marrthrListenHandle[2], NULL);
    

    if( !ok )
    {
        //TODO: launch error
        printf( "Error closing connections.\n" );
    }
}

void Q4SServerProtocol::closeConnections()
{
    bool ok = true;

    if( ok )
    {
        ok &= mServerSocket.closeConnection( SOCK_STREAM );
        ok &= mServerSocket.closeConnection( SOCK_DGRAM );
        //WaitForMultipleObjects( 2, marrthrDataHandle, true, INFINITE );
        pthread_join( marrthrDataHandle[0], NULL);
    }

    if( !ok )
    {
        //TODO: launch error
        printf( "Error closing connections.\n" );
    }
}
bool Q4SServerProtocol::handshake(Q4SSDPParams &params)
{
    printf("----------Handshake Phase\n");
    printf("WAITING FOR BEGIN\n");
    std::string message;

    bool ok = true;
    
    if ( ok ) 
    {
        // Wait for a message
        mReceivedMessages.readFirst( message );
    }

    if (ok)
    {
        std::string pattern;
        pattern.assign( "BEGIN" );
        if ( message.substr( 0, pattern.size( ) ).compare( pattern ) != 0)
        {
            ok = false;
        }
    }
    
    if( ok )
    {
        Q4SMessage message200;

        params.qosLevelUp = 0;
        params.qosLevelDown = 0;
        if (q4SServerConfigFile.isReactive)
        {
           params.q4SSDPAlertingMode = Q4SSDPALERTINGMODE_REACTIVE;
        }
        else
        {
           params.q4SSDPAlertingMode = Q4SSDPALERTINGMODE_Q4SAWARENETWORK;
        }
        params.alertPause = q4SServerConfigFile.alertPause;
        params.recoveryPause = q4SServerConfigFile.recoveryPause;
        params.latency = q4SServerConfigFile.latency;
        params.jitterUp = q4SServerConfigFile.jitterUp;
        params.jitterDown = q4SServerConfigFile.jitterDown;
        params.bandWidthUp = q4SServerConfigFile.bandwidthUp[0];
        params.bandWidthDown = q4SServerConfigFile.bandwidthDown[0];
        params.packetLossUp = q4SServerConfigFile.packetLossUp;
        params.packetLossDown = q4SServerConfigFile.packetLossDown;
        params.procedure.negotiationTimeBetweenPingsUplink = q4SServerConfigFile.timeBetweenPingsNegotiationUp;
        params.procedure.negotiationTimeBetweenPingsDownlink = q4SServerConfigFile.timeBetweenPingsNegotiationDown;
        params.procedure.continuityTimeBetweenPingsUplink = q4SServerConfigFile.timeBetweenPingsContinuityUp;
        params.procedure.continuityTimeBetweenPingsDownlink = q4SServerConfigFile.timeBetweenPingsContinuityDown;
        params.procedure.bandwidthTime = q4SServerConfigFile.bandwidthTime;
        params.procedure.windowSizeLatencyCalcUplink= q4SServerConfigFile.numberOfPingsUp;
        params.procedure.windowSizeLatencyCalcDownlink= q4SServerConfigFile.numberOfPingsDown;
        params.procedure.windowSizePacketLossCalcUplink= q4SServerConfigFile.numberOfBwidthsUp;
        params.procedure.windowSizePacketLossCalcDownlink= q4SServerConfigFile.numberOfBwidthsDown;
        ok &= message200.init200OKBeginResponse(params);
        ok &= mServerSocket.sendTcpData( DEFAULT_CONN_ID, message200.getMessageCChar());
    }

    return ok;
}

 bool Q4SServerProtocol::negotiation(Q4SSDPParams &params, Q4SMeasurementResult &results) 
 {
    printf("----------Negotiation Phase\n");
    bool ok = true;
    int QOS_negotiation;
    bool measureOk= false; 
    /*
    pthread_mutex_lock (&mut_flag);
    bool flagEoS_negotiation=flagEoS;
    pthread_mutex_unlock (&mut_flag);
    */

    for( QOS_negotiation = 0; (measureOk  == false ) && ( QOS_negotiation!= 11 ); QOS_negotiation++ )
    {
    /*
    pthread_mutex_lock (&mut_flag);
    flagEoS_negotiation=flagEoS;
    pthread_mutex_unlock (&mut_flag);
    */
    ok &= Q4SServerProtocol::ready(params);
        if (ok)
        {
            printf("MEASURING\n");
            Q4SMeasurementResult upResults;
            params.bandWidthUp = q4SServerConfigFile.bandwidthUp[QOS_negotiation];
            params.bandWidthDown = q4SServerConfigFile.bandwidthDown[QOS_negotiation];
            params.qosLevelUp = QOS_negotiation;
            params.qosLevelDown = QOS_negotiation;
            measureOk = Q4SServerProtocol::measureStage0(params, results, upResults, 20);
            if (measureOk)
            {
               ok &= Q4SServerProtocol::ready(params);
               if (ok)
               {

                    measureOk = Q4SServerProtocol::measureStage1(params, results, upResults);
               }
            }
            if(!measureOk)
            {

                std::string alertMessage;
                alertMessage.append("Latency: " + std::to_string((long double)results.values.latency));
                alertMessage.append(" Jitter: " + std::to_string((long double)results.values.jitter));
                alertMessage.append(" PacketLoss: " + std::to_string((long double)results.values.packetLoss));
                alertMessage.append(" BandWidth: " + std::to_string((long double)results.values.bandwidth));
                
                //Alert
                Q4SServerProtocol::alert(alertMessage);
            }           
        }
        qosLevel= QOS_negotiation; 
    }
        qosLevelMax= qosLevel; 
        return ok;
 }
void Q4SServerProtocol::continuity(Q4SSDPParams params)
{
    printf("----------Continuity Phase\n");
    bool measureOk = true;
    /*
    pthread_mutex_lock (&mut_flag);
    bool flagEoS_continuity=flagEoS;
    pthread_mutex_unlock (&mut_flag);
    */
    while ( !stop)
    {
        /*
        pthread_mutex_lock (&mut_flag);
        flagEoS_continuity=flagEoS;
        pthread_mutex_unlock (&mut_flag);
        */
        printf("MEASURING\n");
        Q4SMeasurementResult upResults;
        Q4SMeasurementResult results;
        stop= mReceivedMessages.readCancelMessage();
        
        measureOk = Q4SServerProtocol::measureContinuity(params, results, upResults, 20);
        if (!measureOk)
        {
            //Alert
            std::string alertMessage = generateNotificationAlertMessage(params, upResults, results);
            alert(alertMessage);
        }
        else
        {
           // Recovery
           std::string alertMessage = generateNotificationAlertMessage(params, upResults, results);

           recovery(alertMessage);
        }

    }
}
bool Q4SServerProtocol::ready(Q4SSDPParams &params)
{
    printf("WAITING FOR READY\n");
    std::string message;

    bool ok = false;
    while (!ok)
    {
        
        ok = mReceivedMessages.readFirst( message );
        //printf("No hay mensaje\n");
        

        if (ok)
        {
            std::string patternReady;
            patternReady.assign( "READY" );
            if ( message.substr( 0, patternReady.size( ) ).compare( patternReady ) != 0)
            {
                ok = false;
            }
            else
            {
                std::string patternStage;
                patternStage.assign("Stage:");
                if ( message.find( patternStage) == std::string::npos)
                {
                   ok = false;
                }
            }
        }
    }
    if( ok )
    {
        Q4SMessage message200;
        ok &= message200.init200OKBeginResponse(params);
        ok &= mServerSocket.sendTcpData( DEFAULT_CONN_ID, message200.getMessageCChar());
    }

    return ok;
}



void Q4SServerProtocol::alert(std::string alertMessage)
{   
    struct timeval time_s;
    int time_error = gettimeofday(&time_s, NULL); 

    unsigned long actualTime =  time_s.tv_sec*1000 + time_s.tv_usec/1000;
    unsigned long timeFromLastAlert = actualTime - lastAlertTimeStamp;
    if ( timeFromLastAlert > q4SServerConfigFile.alertPause)
    {
        qosLevel++;
        lastAlertTimeStamp = actualTime;

        std::string message = "ALERT "+alertMessage;
        mServerSocket.sendAlertData(message.c_str());

        printf("METHOD: alert\n");
    }
}

void Q4SServerProtocol::end()
{
    closeConnections();
}

void Q4SServerProtocol::recovery(std::string recoveryMessage)
{
    if (qosLevel ==  qosLevelMax)
    {
       printf("No recovery send because QOS Level = %d\n", qosLevelMax);
    }
    else
    {
       if (lastAlertTimeStamp > recoveryTimeStamp)
       {
           recoveryTimeStamp = lastAlertTimeStamp;
       }

        struct timeval time_s;
        int time_error = gettimeofday(&time_s, NULL);
        unsigned long actualTime =  time_s.tv_sec*1000 + time_s.tv_usec/1000;
            
       
       unsigned long timeForRecovery = actualTime - recoveryTimeStamp;
       if ( timeForRecovery > q4SServerConfigFile.recoveryPause)
       {
           qosLevel--;

           recoveryTimeStamp = actualTime;

           std::string message = "RECOVERY " + recoveryMessage;
           mServerSocket.sendAlertData(message.c_str());

           printf("METHOD: recovery\n");
           printf("QOS Level: %d\n", qosLevel);
       }
    }
}


bool Q4SServerProtocol::sendRegularPings(std::vector<unsigned long> &arrSentPingTimestamps, unsigned long pingsToSend, unsigned long timeBetweenPings)
{
    bool ok = true;

    Q4SMessage message;
    unsigned long timeStamp = 0;
    int pingNumber = 0;
    int pingNumberToSend = pingsToSend;
    int time_error;

    for( pingNumber = 0; pingNumber < pingNumberToSend; pingNumber++ )
    {
        // Store the timestamp
        struct timeval time_s;
        time_error = gettimeofday(&time_s, NULL); 
        unsigned long timeStamp =  time_s.tv_sec*1000 + time_s.tv_usec/1000;
        
        arrSentPingTimestamps.push_back( timeStamp );
        // Prepare message and send
        message.initPing("myIp", q4SServerConfigFile.defaultUDPPort, pingNumber, timeStamp);
        ok &= mServerSocket.sendUdpData( DEFAULT_CONN_ID, message.getMessageCChar() );
        

        // Wait the established time between pings



        usleep(timeBetweenPings*1000 );
    }
    return ok;
}

bool Q4SServerProtocol::measureStage0(Q4SSDPParams params, Q4SMeasurementResult &results, Q4SMeasurementResult &upResults, unsigned long pingsToSend)
{
    bool ok = true;

    std::vector<unsigned long> arrSentPingTimestamps;
    Q4SMeasurementValues upMeasurements;

    if ( ok ) 
    {
        // Wait to recive the first Ping
        Q4SMessageInfo  messageInfo;
        ok &= mReceivedMessages.readPingMessage( 0, messageInfo, false );
    }

    if(!ok)
    {
        printf( "ERROR:PING 0 is not the first message.\n" );
    }

    if( ok )
    {
        // Send regular pings
        ok &= sendRegularPings(arrSentPingTimestamps, pingsToSend, params.procedure.negotiationTimeBetweenPingsDownlink);
    }

    if(!ok)
    {
        printf( "ERROR:sendUdpData PING.\n" );
    }

    if (ok)
    {
        usleep(params.procedure.negotiationTimeBetweenPingsUplink*1000);
        // Calculate Latency
        calculateLatency(
            mReceivedMessages, 
            arrSentPingTimestamps, 
            results.values.latency, 
            pingsToSend, 
            q4SServerConfigFile.showMeasureInfo);
        printf( "MEASURING RESULT - Latency Down: %.3f ms\n", results.values.latency );

        // Calculate Jitter
        calculateJitterStage0(
            mReceivedMessages, 
            results.values.jitter,
            params.procedure.negotiationTimeBetweenPingsDownlink, 
            pingsToSend,
            q4SServerConfigFile.showMeasureInfo);
        printf( "MEASURING RESULT - Jitter Down: %.3f ms\n", results.values.jitter );
    }

    if ( ok ) 
    {

        results.values.packetLoss= 0;  
        results.values.bandwidth= 0; 

        ok &= interchangeMeasurementProcedure(upMeasurements, results);
        printf( "MEASURING RESULT - Latency Up: %.3f ms\n", upMeasurements.latency );
        printf( "MEASURING RESULT - Jitter Up: %.3f ms\n", upMeasurements.jitter );
    }
    if ( ok )
    {
        upResults.values = upMeasurements;

        ok &= checkStage0(params.latency, params.jitterUp, params.latency, params.jitterDown, upResults, results);
    }

    if (!ok)
    {
        printf("STAGE 0\n");
        showCheckMessage(upResults, results);
    }

    return ok;
}

bool Q4SServerProtocol::interchangeMeasurementProcedure(Q4SMeasurementValues &upMeasurements, Q4SMeasurementResult results)
{
    bool ok = false;

  
        // Wait to recive the measurements Ping
    Q4SMessageInfo  messageInfo;
    while(!ok)
    {

        ok = mReceivedMessages.readPingMessage( 0, messageInfo, true );
        if (ok)
        {
            ok &= Q4SMeasurementValues_parse(messageInfo.message, upMeasurements);
            if (!ok)
            {
                printf( "ERROR:Interchange Read measurements fail\n");
            }
        }
      
    }
    
   

    if ( ok )
    {
        // Send Info Ping with sequenceNumber 0
        Q4SMessage infoPingMessage;
        ok &= infoPingMessage.initPing("myIp", q4SServerConfigFile.defaultUDPPort, 0, 0, results.values);
        ok &= mServerSocket.sendTcpData(DEFAULT_CONN_ID, infoPingMessage.getMessageCChar());
    }

    return ok;
}
bool Q4SServerProtocol::measureContinuity(Q4SSDPParams params, Q4SMeasurementResult &results, Q4SMeasurementResult &upResults, unsigned long pingsToSend)
{
    bool ok = true;

    std::vector<unsigned long> arrSentPingTimestamps;
    Q4SMeasurementValues upMeasurements;

    if ( ok ) 
    {
        // Wait to recive the first Ping
        Q4SMessageInfo  messageInfo;
        ok &= mReceivedMessages.readPingMessage( 0, messageInfo, false );
    }

    if(!ok)
    {
        printf( "ERROR:PING 0 is not the first message.\n" );
        stop= true; 
    }

    if( ok )
    {
        // Send regular pings
        ok &= sendRegularPings(arrSentPingTimestamps, pingsToSend, params.procedure.continuityTimeBetweenPingsDownlink);
    }
    if(!ok)
    {
        printf( "ERROR:sendUdpData PING.\n" );
    }

    if (ok)
    {
        // Wait the established time to start calculation
        usleep(params.procedure.negotiationTimeBetweenPingsUplink *1000);
 


        // Calculate Latency
        calculateLatency(mReceivedMessages, arrSentPingTimestamps, results.values.latency, pingsToSend, q4SServerConfigFile.showMeasureInfo);
        printf( "MEASURING RESULT - Latency Down: %.3f ms\n", results.values.latency );

        // Calculate Jitter
        calculateJitterAndPacketLossContinuity(mReceivedMessages, results.values.jitter, params.procedure.continuityTimeBetweenPingsDownlink, pingsToSend,results.values.packetLoss, q4SServerConfigFile.showMeasureInfo);
        printf( "MEASURING RESULT - Jitter Down: %.3f ms\n", results.values.jitter );
        printf( "MEASURING RESULT - PacketLoss Down: %.3f %\n", results.values.packetLoss );
    }
        // Check latency and jitter limits
    if (ok)
    {
        ok &= interchangeMeasurementProcedure(upMeasurements, results);
        printf( "MEASURING RESULT - Latency Up: %.3f ms\n", upMeasurements.latency );
        printf( "MEASURING RESULT - Jitter Up: %.3f ms\n", upMeasurements.jitter );
        printf( "MEASURING RESULT - PacketLoss Up: %.3f %\n", upMeasurements.packetLoss );
    }

    if (ok)
    {
        // Check
       upResults.values = upMeasurements;

        ok &= Q4SCommonProtocol::checkContinuity(
            params.latency, params.jitterUp, params.packetLossUp,
            params.latency, params.jitterDown, params.packetLossDown,
            upResults,
            results);
    }
    if (!ok)
    {
        showCheckMessage(upResults, results);  
    }

    

    return ok;
}
bool Q4SServerProtocol::measureStage1(Q4SSDPParams params, Q4SMeasurementResult &results, Q4SMeasurementResult upResults)
{
    bool ok = true;

    Q4SMeasurementValues upMeasurements;



 

    Q4SMessage message;    

    bandWidthUp= params.bandWidthUp;
    pthread_create(&sendUDPBW_thread, NULL, sendUDPBWFn, ( void* ) this);


    struct timeval time_t_aux;
    int time_error = gettimeofday(&time_t_aux, NULL); 
    unsigned long initialTimeStamp =  time_t_aux.tv_sec*1000 + time_t_aux.tv_usec/1000;

    unsigned long TimeStamp2;
    unsigned long sequenceNumber = 0;
    int diffTime; 
    int interval= 0; 

    while(ok && interval != params.procedure.bandwidthTime)
    {
       
        sem_post( &UDPSem);
        usleep(1000);
        interval++;
    }
    //usleep(1000000);
    sleep(2); 
    pthread_cancel(sendUDPBW_thread) ; 

    printf("Paquetes enviados: %d\n", sequenceNumber);
    if (ok)
    {
        // Calculate PacketLoss
        bool okCalculated = calculateBandwidthPacketLossStage1(mReceivedMessages, results.values.packetLoss, params.procedure.bandwidthTime, results.values.bandwidth);
        if (!okCalculated)
        {
            printf( "PacketLoss Calculation Error");
        }
        printf( "MEASURING RESULT - BandWidth Down: %0.2f kb/s\n", results.values.bandwidth );

        printf( "MEASURING RESULT - PacketLoss Down: %.3f %\n", results.values.packetLoss );
    }

    if(!ok)
    {
        printf( "ERROR:sendUdpData BWidth.\n" );
    }

    if (ok)
    {
        ok &= interchangeMeasurementProcedure(upMeasurements, results);
        printf( "MEASURING RESULT - Bandwidth Up: %.3f kb/s\n", upMeasurements.bandwidth);
        printf( "MEASURING RESULT - PacketLoss Up: %.3f %\n", upMeasurements.packetLoss );
    }

    if (ok)
    {
        // Check stage 1
        upResults.values = upMeasurements;

        ok &= checkStage1(params.bandWidthUp, params.packetLossUp, params.bandWidthDown, params.packetLossDown, upResults, results);
    }

    if (!ok)
    {
        printf("showCheckMessage\n");
        showCheckMessage(upResults, results);
    }

    return ok;
}


void* Q4SServerProtocol::manageTcpConnectionsFn( void* lpData )
{
    Q4SServerProtocol* q4sCFI = ( Q4SServerProtocol* )lpData;
    return q4sCFI->manageTcpConnection( );
}

void* Q4SServerProtocol::manageUdpReceivedDataFn( void* lpData )
{
    Q4SServerProtocol* q4sCP = ( Q4SServerProtocol* )lpData;
    return q4sCP->manageUdpReceivedData( );
}
void* Q4SServerProtocol::manageTcpConnection( )
{
    bool        ok = true;
    int         newConnId = 1;
    int         thread_error;
    if( ok )
    {
        ok &= mServerSocket.startTcpListening( );
    }

    while( ok )
    {
        ok &= mServerSocket.waitForTcpConnection( newConnId );
        if( ok )
        {
            ManageTcpConnectionsFnInfo  fnInfo;
            fnInfo.pThis = this;
            fnInfo.connId = newConnId;
            thread_error = pthread_create( &marrthrDataHandle[0], NULL, manageTcpReceivedDataFn, ( ManageTcpConnectionsFnInfo* ) &fnInfo);
            if (thread_error< 0)
            {
                printf("ERRor marrthrDataHandle\n");
            }
            
        }
        newConnId++;
    }  
}

void* Q4SServerProtocol::manageTcpReceivedDataFn( void* lpData )
{
    ManageTcpConnectionsFnInfo* q4sCFI = ( ManageTcpConnectionsFnInfo* )lpData;
    return q4sCFI->pThis->manageTcpReceivedData( q4sCFI->connId );
}

void* Q4SServerProtocol::manageTcpReceivedData( int connId )
{
    bool                ok = true;
    char                buffer[ 65536 ];    
    struct timeval time_s;
    int time_error;            
    /*
    pthread_mutex_lock (&mut_Timestamp);
    expiratedTimeStamp =  time_s.tv_sec*1000 + time_s.tv_usec/1000;
    pthread_mutex_unlock (&mut_Timestamp); 
    */
    while( ok ) 
    {

        ok &= mServerSocket.receiveTcpData( connId, buffer, sizeof( buffer ) );
        if( ok )
        {

            std::string message = buffer;
            mReceivedMessages.addMessage ( message );
            /*
            time_error = gettimeofday(&time_s, NULL); 
            pthread_mutex_lock (&mut_Timestamp);
            expiratedTimeStamp =  time_s.tv_sec*1000 + time_s.tv_usec/1000;
            pthread_mutex_unlock (&mut_Timestamp);
            */
       
        }

    }
    Q4SServerProtocol::done();
}
void* Q4SServerProtocol::manageUdpReceivedData( )
{
    bool                ok = true;
    char                udpBuffer[ 65536 ];
    int                 connId;
    struct timeval time_s;
    int time_error; 
    unsigned long actualTimeStamp;
    std::string message;
    int pingNumber = 0;
    unsigned long receivedTimeStamp = 0;

    mServerSocket.startUdpListening( );

    while ( ok )
    {
        ok &= mServerSocket.receiveUdpData( udpBuffer, sizeof( udpBuffer ), connId );
        if( ok )
        {

            time_error = gettimeofday(&time_s, NULL); 

            actualTimeStamp =  time_s.tv_sec*1000 + time_s.tv_usec/1000;

            message = std::string(udpBuffer);

            pingNumber = 0;
            receivedTimeStamp = 0;

            // Comprobar que es un ping
            if ( Q4SMessageTools_isPingMessage(udpBuffer, &pingNumber, &receivedTimeStamp) )
            {
                if (q4SServerConfigFile.showReceivedPingInfo)
                {
                    printf( "Received Ping, number:%d, timeStamp: %d\n", pingNumber, receivedTimeStamp);
                }

                // mandar respuesta del ping
                char reasonPhrase[ 256 ];
                if (q4SServerConfigFile.showReceivedPingInfo)
                {
                    printf( "Ping responsed %d\n", pingNumber);
                }
                Q4SMessage message200;
                sprintf( reasonPhrase, "OK %d", pingNumber );
                ok &= message200.initResponse(Q4SRESPONSECODE_200, reasonPhrase);
                ok &= mServerSocket.sendUdpData( connId, message200.getMessageCChar() );

                if (q4SServerConfigFile.showReceivedPingInfo)
                {
                    printf( "Received Udp: <%s>\n", udpBuffer );
                }
            
               // encolar el ping y el timestamp para el calculo del jitter
               
            }

            mReceivedMessages.addMessage(message, actualTimeStamp);           
            
       /*
        pthread_mutex_lock (&mut_Timestamp);
        expiratedTimeStamp =  time_s.tv_sec*1000 + time_s.tv_usec/1000;
        pthread_mutex_unlock (&mut_Timestamp);
        */
            
            
           
        }

/*
        // Key management
        if (EKey_getKeyState(EK_A))
        {
            printf( "Alert Sended\n");
            std::string alertMessage;
            alertMessage= "Alert by keyboard";
            alert(alertMessage);
        }
*/
    }

}
/*
void* Q4SServerProtocol::checkConnectionsFn( void* lpData )
{
    ManageTcpConnectionsFnInfo* q4sCFI = ( ManageTcpConnectionsFnInfo* )lpData;
    return q4sCFI->pThis->checkConnections(q4sCFI->connId );
}

void* Q4SServerProtocol::checkConnections( int connId )
{    
    struct timeval time_s;
    unsigned long actualTimeStamp;      
    unsigned long lastMessageTimeStamp;      
    int           time_error;
    //pthread_mutex_lock(&mut_flag);
    bool flagEoS_measure= false; 
    //pthread_mutex_unlock(&mut_flag);

    while (!flagEoS_measure)
    {
        sleep(10); 
        time_error = gettimeofday(&time_s, NULL); 
        actualTimeStamp= time_s.tv_sec*1000 + time_s.tv_usec/1000;      
        pthread_mutex_lock (&mut_Timestamp);
        lastMessageTimeStamp= expiratedTimeStamp;
        pthread_mutex_unlock (&mut_Timestamp);
        if (actualTimeStamp>(lastMessageTimeStamp+q4SServerConfigFile.timeEndApp/1000))
        {      
            flagEoS_measure= true;  
            
        }
    }
    pthread_mutex_lock(&mut_flag);
    flagEoS= true; 
     
    pthread_mutex_unlock(&mut_flag);


}
*/void* Q4SServerProtocol::sendUDPBWFn(void* lpData )
{
    Q4SServerProtocol* q4sCP = ( Q4SServerProtocol* )lpData;    
    //unsigned long bandWidthDown=*(); 

    bool ret = q4sCP->sendUDPBW( q4sCP->bandWidthUp);
}
 void *Q4SServerProtocol::sendUDPBW(unsigned long bandWidthUp)
 {   
    int time_error; 
    bool ok= true; 
    //unsigned long bandWidthDown=*((unsigned long*)bandWidthDownParam); 
    float message_size= 1000*8; 
    float bandWidthUpInc= (float)bandWidthUp*1.16; // Se multiplica por 1.05 para dejar un margen superior 

    float messages_fract_per_ms = (bandWidthUpInc/ (float) message_size);
    printf("%f\n", messages_fract_per_ms);
    int   messages_int_per_ms = floor(messages_fract_per_ms);

    float messages_per_s[10];
    messages_per_s[0] = ((messages_fract_per_ms - (float) messages_int_per_ms) * 1000);
    int ms_per_message[11]={};
    ms_per_message[0] = 1;
    int divisor;
    for (int i = 0; i < 10; i++) 
    {
        divisor = 2;
        ////////////////////////////////////////////////
        // MAYOR O IGUAL 
        ////////////////////////////////////////////////
        //printf("message per s: %f\n", messages_per_s[i]);
        while ((1000/divisor) >= messages_per_s[i]) 
        {
            divisor++;
        }
        ms_per_message[i+1] = divisor;
        if (messages_per_s[i] - ((float)1000/divisor) == 0) 
        {
            //printf("multiplos 1: %d\n", ms_per_message[i+1]);

            break;
        } 
        else if (messages_per_s[i] - ((float)1000/divisor) <= 1) 
        {

            ms_per_message[i+1]--;
            //printf("multiplos 2: %d\n", ms_per_message[i+1]);

            break;
        } 
        else 
        {        
            //printf("multiplos 3: %d\n", ms_per_message[i+1]);
            messages_per_s[i+1] = messages_per_s[i] -((float)1000/divisor);
        }
    } 

    Q4SMessage message;
    char message_char[2048] = {0};
    struct timeval time_t_aux;
    time_error = gettimeofday(&time_t_aux, NULL); 
    unsigned long TimeStamp2;     
    unsigned long TimeStamp3; 
    unsigned long diffTime; 

    unsigned long sequenceNumber = 0;
    unsigned long initialTimeStamp =  time_t_aux.tv_sec*1000 + time_t_aux.tv_usec/1000;   
    int interval= 0;
    while(1)
    {
        sem_wait( &UDPSem);
        int j = 0; 
        
        time_error = gettimeofday(&time_t_aux, NULL); 
        TimeStamp2 =  time_t_aux.tv_sec*1000 + time_t_aux.tv_usec/1000;
        
        while (j < messages_int_per_ms) 
            {
                //ok &= message.initRequest(Q4SMTYPE_BWIDTH, "myIp", q4SServerConfigFile.defaultUDPPort, true, sequenceNumber, true, TimeStamp2);
                sprintf(message_char,
            "BWIDTH q4s://myIp:27016  Q4S/1.0\nSequence-Number:%d\nTimestamp:%lu\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
            sequenceNumber,TimeStamp2);
                ok &= mServerSocket.sendUdpData(DEFAULT_CONN_ID,   message_char);
                sequenceNumber++; 
                j++; 
            }

       for (int k = 1; k < 11; k++) 
        {
            if (ms_per_message[k] > 0 && interval % ms_per_message[k] == 0) 
            {        
                //printf("mensahe extra %d, %d, %d, %d\n", interval, ms_per_message[k], k, sizeof(ms_per_message));
                sprintf(message_char,
            "BWIDTH q4s://myIp:27016  Q4S/1.0\nSequence-Number:%d\nTimestamp:%lu\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
            sequenceNumber,TimeStamp2);
                ok &= mServerSocket.sendUdpData(DEFAULT_CONN_ID,   message_char);
                sequenceNumber++;  
            }
        }
        //printf("sequenceNumber: %d\n", sequenceNumber); 
        /* 
        time_error = gettimeofday(&time_t_aux, NULL);
        TimeStamp3 =  time_t_aux.tv_sec*1000 + time_t_aux.tv_usec/1000;
        printf("TimeStamp3 %lu\t",TimeStamp3);
        diffTime= (int)(TimeStamp3- TimeStamp2);    
        printf("diffTime %d\n", diffTime); 
        //usleep(1000-(diffTime*1000)); 
        */

        interval++; 
    } 
 }