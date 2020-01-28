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
#include <AIP/AudioInputProcessor.h>
//Mohammad add
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
//const char* books[] = {"War and Peace",
//                       "Pride and Prejudice",
//                       "The Sound and the Fury"};
//
void report(const char* msg, int terminate) {
  perror(msg);
  if (terminate) exit(-1); /* failure */
}

//Mohammad end

namespace alexaClientSDK {
namespace sampleApp {

capabilityAgents::aip::AudioProvider * static_audioProvider = NULL;
void notify_keyword_detection_over_network(avsCommon::avs::AudioInputStream::Index end_index);
void send_audio_to_SDK2();
void wait_for_MegaMind_engine_response(int * allowed, std::string ** text_cmd){
   int option = 1;
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
   saddr.sin_port = htons(PortNumber_MegaMindEngine);        /* for listening */
   if (bind(fd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0)
    report("bind", 1); /* terminate */
   /* listen to the socket */
   if (listen(fd, MaxConnects) < 0) /* listen for clients, up to MaxConnects */
   report("listen", 1); /* terminate */
      fprintf(stderr, "Listening on port %i for clients...\n", PortNumber_stop);
   int client_fd;
   struct sockaddr_in caddr; /* client address */
   int len = sizeof(caddr);  /* address length could change */
  
   client_fd = accept(fd, (struct sockaddr*) &caddr,(socklen_t*) &len);  /* accept blocks */
   if (client_fd < 0) {
     report("accept", 0); /* don't terminated, though there's a problem */
   }
   setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
   char buffer[1024];
   std::string * cmd = NULL; 
   int count = read(client_fd, buffer, sizeof(buffer));
   if (count > 0) {
	cmd = new std::string(buffer, count );
        std::cout<<count<<"bytes read;  cmd= "<<(*cmd)<<"\n";
   }else{
       std::cout<<" count is negative\n";
       return;
   }
   if(cmd !=NULL){
       *text_cmd = cmd; 
   }
   int trueVar = 1;
   setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&trueVar,sizeof(int));
   setsockopt(client_fd,SOL_SOCKET,SO_REUSEADDR,&trueVar,sizeof(int));

   close(fd);
   close(client_fd); /* break connection */
    *allowed = 1;
}
void wait_for_start(){
      std::cout<<"wait_for_start [0]\n";
      while(1){
         std::cout<<"wait_for_start: before going to busy wait stage\n";
         while(1){
		if(static_audioProvider == NULL){
		   std::cout<<"static_audioProvider is NULL[1]\n";
		   break;
		}
		if(*(static_audioProvider->MegaMind_StartRecording) == 1){
		     break;
		}
	 }
         std::cout<<"wait_for_start: after coming out of busy wait stage\n";
	if(static_audioProvider == NULL){
	   std::cout<<"static_audioProvider is NULL[2]\n";
	   continue;
	}
         *(static_audioProvider->MegaMind_StartRecording)  = 0;
         notify_keyword_detection_over_network(0);
         //send_audio_to_SDK2();
         int allowed = 0 ;
         std::string * text_cmd = new std::string("play hidden brain");
         wait_for_MegaMind_engine_response(&allowed, &text_cmd);
	 std::cout << "MJ MJ MJ\t"<<*text_cmd<<"\n";
	 *(static_audioProvider->MegaMind_text_cmd)= *text_cmd;
         *(static_audioProvider->MegaMind_Allowed) = allowed;
         *(static_audioProvider->MegaMind_Desision_Isready)  = 1;
         std::cout<<"should start recording\n"; 
      }
}
KeywordObserver::KeywordObserver(
    std::shared_ptr<defaultClient::DefaultClient> client,
    capabilityAgents::aip::AudioProvider audioProvider,
    std::shared_ptr<esp::ESPDataProviderInterface> espProvider) :
        m_client{client},
        m_audioProvider{audioProvider},
        m_espProvider{espProvider} {
 	static_audioProvider = &m_audioProvider;
        std::cout<<"before run the wait for start thread\n";
        std::thread th2(wait_for_start);
        th2.detach();

        
}
int continue_to_record;
void wait_for_stop(){
//	int counter = 0;
   int option = 1;
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
   saddr.sin_port = htons(PortNumber_stop);        /* for listening */
   if (bind(fd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0)
    report("bind", 1); /* terminate */
   /* listen to the socket */
   if (listen(fd, MaxConnects) < 0) /* listen for clients, up to MaxConnects */
   report("listen", 1); /* terminate */
      fprintf(stderr, "Listening on port %i for clients...\n", PortNumber_stop);
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
      char buffer[1024];
      int count = read(client_fd, buffer, sizeof(buffer));
      if (count > 0) {
       //write(client_fd, buffer, sizeof(buffer)); /* echo as confirmation */
          std::cout<<count<<" bytes read , recived to stop recording\n";
	  continue_to_record --;
          if (continue_to_record ==0){
	     break;
	  }
	  continue;
      }else{
          std::cout<<" count is negative\n";
          continue;
      }

      int trueVar = 1;
      setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&trueVar,sizeof(int));
      setsockopt(client_fd,SOL_SOCKET,SO_REUSEADDR,&trueVar,sizeof(int));

      close(fd);
      close(client_fd); /* break connection */
   }  /* while(1) */

   close(fd);
   close(client_fd); /* break connection */
}

void notify_keyword_detection_over_network(avsCommon::avs::AudioInputStream::Index end_index){


    int sockfd = socket(AF_INET,      /* versus AF_LOCAL */
			SOCK_STREAM,  /* reliable, bidirectional */
			0);           /* system picks protocol (TCP) */
    if (sockfd < 0) report("socket", 1); /* terminate */
  
    /* get the address of the host */
    struct hostent* hptr = gethostbyname(Host); /* localhost: 127.0.0.1 */
    if (!hptr) report("gethostbyname", 1); /* is hptr NULL? */
    if (hptr->h_addrtype != AF_INET)       /* versus AF_LOCAL */
      report("bad address family", 1);
  
    /* connect to the server: configure server's address 1st */
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr =
       ((struct in_addr*) hptr->h_addr_list[0])->s_addr;
    saddr.sin_port = htons(PortNumber_start); /* port number in big-endian */
  
    if (connect(sockfd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0)
      report("connect", 1);
  
    /* Write some stuff and read the echoes. */
    puts("Connect to server, about to write some stuff...");
    uint64_t uend_index = end_index;
   
    if (write(sockfd, &uend_index, sizeof(uend_index)) < 0) {
           std::cout<<"error: could not send the packets\n";
    }
    close(sockfd); /* close the connection */

}
void send_audio_to_SDK2(){

    int sockfd = socket(AF_INET,      /* versus AF_LOCAL */
			SOCK_STREAM,  /* reliable, bidirectional */
			0);           /* system picks protocol (TCP) */
    if (sockfd < 0) report("socket", 1); /* terminate */
  
    /* get the address of the host */
    struct hostent* hptr = gethostbyname(Host); /* localhost: 127.0.0.1 */
    if (!hptr) report("gethostbyname", 1); /* is hptr NULL? */
    if (hptr->h_addrtype != AF_INET)       /* versus AF_LOCAL */
      report("bad address family", 1);
  
    /* connect to the server: configure server's address 1st */
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr =
       ((struct in_addr*) hptr->h_addr_list[0])->s_addr;
    saddr.sin_port = htons(PortNumber); /* port number in big-endian */
  
    if (connect(sockfd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0)
      report("connect", 1);
  
    puts("Connect to server, about to write some stuff...");

    std::cout<<"Javad: add all the magic here\n";
    if(static_audioProvider == NULL){
           std::cout<<"MegaMind: static audio provider is NULL\n";
    }
    auto my_stream = static_audioProvider->stream;
    auto my_reader = my_stream->createReader(avsCommon::avs::AudioInputStream::Reader::Policy::NONBLOCKING);
    avsCommon::avs::AudioInputStream::Index beginIndex = *(static_audioProvider->MegaMind_begin_index);
    if(beginIndex == capabilityAgents::aip::AudioInputProcessor::INVALID_INDEX){
	beginIndex = my_reader->tell();
    }
    if( !my_reader->seek(beginIndex) ){
	 std::cout<<"Javad: seek failed";
    }
    int read_words;
    int total_read_words = 0;
    int n_words = 10000;
    int word_size = my_reader->getWordSize();
    int size = n_words * word_size;
    void * buffer; 
    buffer = new char [size];
    std::ofstream oFile;
    oFile.open("/tmp/oFile.pcm");
    continue_to_record = 2;
    std::thread th1(wait_for_stop);
    //while(total_read_words < 300000){
    while(continue_to_record>0){
	memset(buffer, 0 , size);
	read_words = my_reader->read(buffer,n_words);
	if( (read_words<0) || (read_words>1000) ){
	    continue;
	}
	 //std::cout<<"read_words: "<<read_words<<"\t total read words:"<<total_read_words<<"\n";

	if (write(sockfd, buffer, read_words * word_size) < 0) {
	   std::cout<<"error: could not send the packets\n";
	}
	oFile.write((char*)buffer,read_words * word_size);
	total_read_words += read_words;
    }	
    th1.join();

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
//Mohammad
            /* fd for the socket */
	    std::cout<< " HEYYYY: \t"<<beginIndex<<"\t\t" <<endIndex;
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
