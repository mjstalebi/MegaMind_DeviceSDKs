/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "SampleApp/KeywordObserver.h"
//Mohammad start
#include "sock.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <thread>
#include <unistd.h>
#include <fstream>
//Mohammad end
uint64_t global_end_index = 0;
bool global_start_sending = false;
namespace alexaClientSDK {
namespace sampleApp {


capabilityAgents::aip::AudioProvider * static_audioProvider = NULL;
std::shared_ptr<defaultClient::DefaultClient> static_client;
//void report(const char* msg, int terminate) {
//  perror(msg);
//  if (terminate) exit(-1); /* failure */
//}
void KeywordObserver::network_start_callback(){

    int option =1;
    int fd = socket(AF_INET,     /* network versus AF_LOCAL */
                    SOCK_STREAM, /* reliable, bidirectional: TCP */
                    0);          /* system picks underlying protocol */
    if (fd < 0) report("socket", 1); /* terminate */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    /* bind the server's local address in memory */
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));          /* clear the bytes */
    saddr.sin_family = AF_INET;                /* versus AF_LOCAL */
    saddr.sin_addr.s_addr = htonl(INADDR_ANY); /* host-to-network endian */
    saddr.sin_port = htons(PortNumber_start);        /* for listening */
    if (bind(fd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0)
     report("bind", 1); /* terminate */
    /* listen to the socket */
    if (listen(fd, MaxConnects) < 0) /* listen for clients, up to MaxConnects */
    report("listen", 1); /* terminate */
       fprintf(stderr, "Listening on port %i for clients...\n", PortNumber);
    int client_fd;
    while (1) {
       struct sockaddr_in caddr; /* client address */
       int len = sizeof(caddr);  /* address length could change */
 
       client_fd = accept(fd, (struct sockaddr*) &caddr,(socklen_t*) &len);  /* accept blocks */
       if (client_fd < 0) {
         report("accept", 0); /* don't terminated, though there's a problem */
         continue;
       }
       setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
       //char buffer[1024];
       uint64_t uend_index;
       int count = read(client_fd, &uend_index, sizeof(uend_index));   
       if (count > 0) {
         //write(client_fd, buffer, sizeof(buffer)); /* echo as confirmation */
            std::cout<<count<<" bytes read\n";
            std::cout<<"end index is: "<<uend_index<<"\n";
            std::cout<<"global end index is: "<<global_end_index<<"\n";
	    if (static_client) {
	        //m_client->notifyOfTapToTalk(m_audioProvider, endIndex);
		std::cout<<"Keyword detection Signal recieved\n";
                if(static_audioProvider == NULL){
			std::cout<<"static_audio_provider is NULL\n";
			continue;
		}
                if(static_client == NULL){
			std::cout<<"static_client is NULL\n";
			continue;
		}
	        //static_client->notifyOfTapToTalk(*static_audioProvider, uend_index);
	        static_client->notifyOfTapToTalk(*static_audioProvider, global_end_index);
	    }
       }else{
           std::cout<<" count is negative\n";
           continue;
       }
       //global_start_sending = true;
          //ssize_t returnCode = wrapper->m_writer->write(buffer, count);
       
    }  /* while(1) */
 
    close(fd);
    close(client_fd); /* break connection */
}
KeywordObserver::KeywordObserver(
    std::shared_ptr<defaultClient::DefaultClient> client,
    capabilityAgents::aip::AudioProvider audioProvider,
    std::shared_ptr<esp::ESPDataProviderInterface> espProvider) :
        m_client{client},
        m_audioProvider{audioProvider},
        m_espProvider{espProvider} {
    static_client = m_client;
    static_audioProvider = &m_audioProvider;
    std::thread th_start(network_start_callback);
    th_start.detach();
}

void KeywordObserver::onKeyWordDetected(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    std::string keyword,
    avsCommon::avs::AudioInputStream::Index beginIndex,
    avsCommon::avs::AudioInputStream::Index endIndex,
    std::shared_ptr<const std::vector<char>> KWDMetadata) {
    if (endIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX &&
        beginIndex == avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX) {
        if (m_client) {
            m_client->notifyOfTapToTalk(m_audioProvider, endIndex);
        }
    } else if (
        endIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX &&
        beginIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX) {
        auto espData = capabilityAgents::aip::ESPData::EMPTY_ESP_DATA;
        if (m_espProvider) {
            espData = m_espProvider->getESPData();
        }

        if (m_client) {
            m_client->notifyOfWakeWord(m_audioProvider, beginIndex, endIndex, keyword, espData, KWDMetadata);
        }
    }
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
