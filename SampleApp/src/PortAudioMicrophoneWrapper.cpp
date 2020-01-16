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

#include <cstring>
#include <string>

#include <rapidjson/document.h>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include "SampleApp/PortAudioMicrophoneWrapper.h"
#include "SampleApp/ConsolePrinter.h"
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
#include <unistd.h>

extern uint64_t global_end_index;
extern bool global_start_sending;
bool global_pre_is_done = false;
void report(const char* msg, int terminate) {
  perror(msg);
  if (terminate) exit(-1); /* failure */
}

//Mohammad end
namespace alexaClientSDK {
namespace sampleApp {

using avsCommon::avs::AudioInputStream;

static const int NUM_INPUT_CHANNELS = 1;
static const int NUM_OUTPUT_CHANNELS = 0;
static const double SAMPLE_RATE = 16000;
static const unsigned long PREFERRED_SAMPLES_PER_CALLBACK = paFramesPerBufferUnspecified;

static const std::string SAMPLE_APP_CONFIG_ROOT_KEY("sampleApp");
static const std::string PORTAUDIO_CONFIG_ROOT_KEY("portAudio");
static const std::string PORTAUDIO_CONFIG_SUGGESTED_LATENCY_KEY("suggestedLatency");

/// String to identify log entries originating from this file.
static const std::string TAG("PortAudioMicrophoneWrapper");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<PortAudioMicrophoneWrapper> PortAudioMicrophoneWrapper::create(
    std::shared_ptr<AudioInputStream> stream) {
    if (!stream) {
        ACSDK_CRITICAL(LX("Invalid stream passed to PortAudioMicrophoneWrapper"));
        return nullptr;
    }
    std::unique_ptr<PortAudioMicrophoneWrapper> portAudioMicrophoneWrapper(new PortAudioMicrophoneWrapper(stream));
    if (!portAudioMicrophoneWrapper->initialize()) {
        ACSDK_CRITICAL(LX("Failed to initialize PortAudioMicrophoneWrapper"));
        return nullptr;
    }
    return portAudioMicrophoneWrapper;
}

PortAudioMicrophoneWrapper::PortAudioMicrophoneWrapper(std::shared_ptr<AudioInputStream> stream) :
        m_audioInputStream{stream},
        m_paStream{nullptr} {
}

PortAudioMicrophoneWrapper::~PortAudioMicrophoneWrapper() {
    Pa_StopStream(m_paStream);
    Pa_CloseStream(m_paStream);
    Pa_Terminate();
}



void PortAudioMicrophoneWrapper::pre_audio_write_thread( std::shared_ptr<avsCommon::avs::AudioInputStream::Writer> my_writer ){
       char buffer2[256];
       std::ifstream iFile;
       std::ofstream ooFile;
    while(1){ 
       if(global_start_sending){

	       std::cout<<"start pre audio sending\n";
               std::streamsize bytesRead =1;
       	       iFile.open("/tmp/pre1.pcm");
       	       ooFile.open("/tmp/oo.pcm");
	       while(1){
		   if( bytesRead <= 0 || iFile.eof() || iFile.fail()){
			iFile.close();
			std::cout<<"iFile is closed\n";
			break;
		   }

       		   memset(buffer2,0, sizeof(buffer2));
		  // bytesRead = iFile.readsome(buffer2,128);
		  iFile.read(buffer2,128);
                  bytesRead = iFile.gcount();  
		   if( bytesRead <= 0 || iFile.eof() || iFile.fail()){
			iFile.close();
			std::cout<<"iFile is closed\n";
			break;
		   }
		   std::cout<<"."<<std::flush;
           	   my_writer->write(buffer2, bytesRead/2);
		   //usleep(100); 
                   ooFile.write(buffer2,bytesRead);
		   global_end_index += bytesRead/2;
		}
		global_start_sending = false;
		usleep(4000000); 
		std::cout<<"after delay\n";
               iFile.open("/tmp/pre2.pcm");
	       while(1){
		   if( bytesRead <= 0 || iFile.eof() || iFile.fail()){
			iFile.close();
			std::cout<<"iFile is closed\n";
			break;
		   }

       		   memset(buffer2,0, sizeof(buffer2));
		  // bytesRead = iFile.readsome(buffer2,128);
		  iFile.read(buffer2,128);
                  bytesRead = iFile.gcount();  
		   if( bytesRead <= 0 || iFile.eof() || iFile.fail()){
			iFile.close();
			std::cout<<"iFile is closed\n";
			break;
		   }
		   std::cout<<"."<<std::flush;
           	   my_writer->write(buffer2, bytesRead/2);
		   //usleep(100); 
                   ooFile.write(buffer2,bytesRead);
		   global_end_index += bytesRead/2;
		}
	
		global_pre_is_done = true;
        }
    }

}
//void PortAudioMicrophoneWrapper::network_callback(PortAudioMicrophoneWrapper* wrapper ){
void PortAudioMicrophoneWrapper::network_callback( std::shared_ptr<avsCommon::avs::AudioInputStream::Writer> my_writer ){
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
    saddr.sin_port = htons(PortNumber);        /* for listening */
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
       struct timeval tv;
       tv.tv_sec = 1;
       setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
       std::ofstream oFile;
       oFile.open("/tmp/oFile3.pcm");

       char buffer[1024];
       //std::ifstream iFile;
       //iFile.open("/tmp/pre1.pcm");
       int first = 1;
       global_pre_is_done = false;
       while(1){
          memset(buffer,0, sizeof(buffer));
          int count = read(client_fd, buffer, sizeof(buffer));   
          if (count > 0) {
              std::cout<<count<<" bytes read , lets write them back\n";
          }else{
              std::cout<<" count is negative\n";
             break;
          }
          if(first ==1 ){
		global_start_sending = true ;
		first = 0;
          }
          while(global_pre_is_done == false){
	  }
          //memset(buffer,0, sizeof(buffer));
          //count =128;
          //iFile.read(buffer,count);
          //count = iFile.gcount();
          //ssize_t returnCode = wrapper->m_writer->write(buffer, count);
          //wrapper->m_writer->write(buffer, count);
          //my_writer->write(buffer, count);
          my_writer->write(buffer, count/2);
          //oFile.write(buffer,count);
          global_end_index += count/2;
       }
       
       //close(fd);
       close(client_fd); /* break connection */
    }  /* while(1) */
 
    close(fd);
    close(client_fd); /* break connection */

}

bool PortAudioMicrophoneWrapper::initialize() {
    m_writer = m_audioInputStream->createWriter(AudioInputStream::Writer::Policy::NONBLOCKABLE);
    if (!m_writer) {
        ACSDK_CRITICAL(LX("Failed to create stream writer"));
        return false;
    }
    std::cout<<"before lunching the thread\n";
    std::thread th1(network_callback,m_writer);
    th1.detach();
    std::cout<<"after lunching the thread\n";
    std::thread th2(pre_audio_write_thread,m_writer);
    th2.detach();
    //usleep(200000);
//    PaError err;
//    err = Pa_Initialize();
//    if (err != paNoError) {
//        ACSDK_CRITICAL(LX("Failed to initialize PortAudio"));
//        return false;
//    }
//
//    PaTime suggestedLatency;
//    bool latencyInConfig = getConfigSuggestedLatency(suggestedLatency);
//
//    if (!latencyInConfig) {
//        err = Pa_OpenDefaultStream(
//            &m_paStream,
//            NUM_INPUT_CHANNELS,
//            NUM_OUTPUT_CHANNELS,
//            paInt16,
//            SAMPLE_RATE,
//            PREFERRED_SAMPLES_PER_CALLBACK,
//            PortAudioCallback,
//            this);
//    } else {
//        ACSDK_INFO(
//            LX("PortAudio suggestedLatency has been configured to ").d("Seconds", std::to_string(suggestedLatency)));
//
//        PaStreamParameters inputParameters;
//        std::memset(&inputParameters, 0, sizeof(inputParameters));
//        inputParameters.device = Pa_GetDefaultInputDevice();
//        inputParameters.channelCount = NUM_INPUT_CHANNELS;
//        inputParameters.sampleFormat = paInt16;
//        inputParameters.suggestedLatency = suggestedLatency;
//        inputParameters.hostApiSpecificStreamInfo = nullptr;
//
//        err = Pa_OpenStream(
//            &m_paStream,
//            &inputParameters,
//            nullptr,
//            SAMPLE_RATE,
//            PREFERRED_SAMPLES_PER_CALLBACK,
//            paNoFlag,
//            PortAudioCallback,
//            this);
//    }
//
//    if (err != paNoError) {
//        ACSDK_CRITICAL(LX("Failed to open PortAudio default stream"));
//        return false;
//    }
    return true;
}

bool PortAudioMicrophoneWrapper::startStreamingMicrophoneData() {
//    std::lock_guard<std::mutex> lock{m_mutex};
//    PaError err = Pa_StartStream(m_paStream);
//    if (err != paNoError) {
//        ACSDK_CRITICAL(LX("Failed to start PortAudio stream"));
//        return false;
//    }
    return true;
}

bool PortAudioMicrophoneWrapper::stopStreamingMicrophoneData() {
//    std::lock_guard<std::mutex> lock{m_mutex};
//    PaError err = Pa_StopStream(m_paStream);
//    if (err != paNoError) {
//        ACSDK_CRITICAL(LX("Failed to stop PortAudio stream"));
//        return false;
//    }
    std::cout<<"KARIM\n";
    return true;
}

int PortAudioMicrophoneWrapper::PortAudioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long numSamples,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
//    PortAudioMicrophoneWrapper* wrapper = static_cast<PortAudioMicrophoneWrapper*>(userData);
  //  ssize_t returnCode = wrapper->m_writer->write(inputBuffer, numSamples);
    ssize_t returnCode = 0;
    if (returnCode <= 0) {
        ACSDK_CRITICAL(LX("Failed to write to stream."));
        return paAbort;
    }
    return paContinue;
}

//bool PortAudioMicrophoneWrapper::getConfigSuggestedLatency(PaTime& suggestedLatency) {
//    bool latencyInConfig = false;
//    auto config = avsCommon::utils::configuration::ConfigurationNode::getRoot()[SAMPLE_APP_CONFIG_ROOT_KEY]
//                                                                               [PORTAUDIO_CONFIG_ROOT_KEY];
//    if (config) {
//        latencyInConfig = config.getValue(
//            PORTAUDIO_CONFIG_SUGGESTED_LATENCY_KEY,
//            &suggestedLatency,
//            suggestedLatency,
//            &rapidjson::Value::IsDouble,
//            &rapidjson::Value::GetDouble);
//    }
//
//    return latencyInConfig;
//}

}  // namespace sampleApp
}  // namespace alexaClientSDK
