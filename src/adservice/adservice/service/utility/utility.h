//
// Created by guoze.lin on 16/1/20.
//

#ifndef ADCORE_UTILITY_H
#define ADCORE_UTILITY_H

#include <array>
#include <ctime>
#include <random>
#include <stddef.h>
#include <map>
#include <vector>
#include <iostream>
#include <strings.h>
#include <cstring>
#include <sstream>
#include <tuple>
#include <fstream>
#include "common/types.h"
#include "common/functions.h"
#include "hash.h"
#include "cypher.h"
#include "json.h"
#include "url.h"
#include "mttytime.h"
#include "escape.h"
#include "file.h"
#include "userclient.h"
#include "http.h"
#include "compress.h"
#include "template_engine.h"
#include "promise.h"
#include <tuple>
#include <functional>
#include <boost/algorithm/string.hpp>
#include "google/protobuf/message.h"
#include "logging.h"

namespace std {

template <>
struct hash<std::pair<int, int>> {
    size_t operator()(const std::pair<int, int> & arg) const noexcept
    {
        auto h = std::hash<int>();
        return h(arg.first) ^ h(arg.second);
    }
};

template <>
struct hash<std::tuple<int, int, int>> {
    size_t operator()(const std::tuple<int, int, int> & arg) const noexcept
    {
        auto h = std::hash<int>();
        return h(std::get<0>(arg)) ^ h(std::get<1>(arg)) ^ h(std::get<2>(arg));
    }
};

}

namespace adservice{
   namespace utility{

       namespace rng{

           extern int32_t randomInt();

           extern double randomDouble();
       }

       namespace memory{


#ifndef OPTIMIZED_MALLOC

//不作内存分配优化,使用库函数

           inline void* adservice_malloc(size_t size){
               return malloc(size);
           }

           inline void* adservice_calloc(size_t num, size_t size){
               return calloc(num,size);
           }


           inline void* adservice_realloc(void* ptr, size_t size){
               return realloc(ptr,size);
           }

           inline void adservice_free(void* ptr){
               free(ptr);
           }

           template<typename T>
           inline T* adservice_new(size_t num = 0){
               if(num == 0)
                   return new T;
               else
                   return new T[num];
           }

//           template<typename T>
//           inline T* adservice_new(void* placement,size_t num = 0){
//               if(num == 0){
//                   return new(placement) T;
//               }else{
//                   return new(placement) T[num];
//               }
//           }

           template<typename T>
           inline void adservice_delete(T* ptr,size_t num = 0){
                if(num>0){
                   delete[] ptr;
                }else{
                    delete ptr;
                }
           }

#else

           //作内存分配优化,重新实现内存管理函数

void* adservice_malloc(size_t size);
void* adservice_calloc(size_t nmemb,size_t size);
void* adservice_realloc(void *ptr,size_t size);
void adservice_free(void* ptr);

#endif

           inline char* strdup(const char* str){
               size_t size = strlen(str);
               char* ret = (char*)adservice_malloc(size+1);
               memcpy(ret,str,size+1);
               return ret;
           }
       }


       namespace serialize{
           //support of protobuf object
           using google::protobuf::Message;

           template<typename T>
           inline bool getProtoBufObject(T& obj,const std::string& data){
               return obj.ParseFromString(data);
           }

           template<typename T>
           inline bool writeProtoBufObject(T& obj,std::string* out){
               return obj.SerializeToString(out);
           }

           template<typename T>
           inline bool getProtoBufObject(T& obj,const std::stringstream& stream){
               return obj.ParseFromIstream(&stream);
           }

           template<typename T>
           inline bool writeProtoBufObject(T& obj,std::stringstream& stream){
               return obj.SerializeToOstream(&stream);
           }

           /**
            * 本方法从avro字节流中获取对象
            */
           template<typename T>
           inline T& getAvroObject(T& obj,const uint8_t* bytes,int size){
               std::unique_ptr<avro::InputStream> in = avro::memoryInputStream(bytes,size); //需要考虑对象重用
               avro::DecoderPtr decoderPtr = avro::binaryDecoder();
               decoderPtr->init(*in);
               avro::codec_traits<T>::decode(*decoderPtr,obj);
               return obj;
           }

           template<typename T>
           inline std::unique_ptr<avro::OutputStream> writeAvroObject(T& obj){
               std::unique_ptr<avro::OutputStream> out = avro::memoryOutputStream();
               avro::EncoderPtr encoderPtr = avro::binaryEncoder();
               encoderPtr->init(*out);
               avro::codec_traits<T>::encode(*encoderPtr,obj);
               encoderPtr->flush();
               //out->flush();
               return out;
           }

           /**
            * 本方法从给定对象获取avro二进制串
            */
           template<typename T>
           inline void writeAvroObject(T& obj,adservice::types::string& output){
               std::unique_ptr<avro::OutputStream> out = writeAvroObject(obj);
               std::unique_ptr<avro::InputStream> is = avro::memoryInputStream(*out);
               const uint8_t* data;
               size_t size;
               while(is->next(&data,&size)){
                    output.append((const char*)data,size);
               }
           }

           template<typename T,int BUFFSIZE>
           class AvroObjectReader{
           public:
               AvroObjectReader(){
                   in = avro::memoryInputStream(buffer,BUFFSIZE);
                   decoderPtr = avro::binaryDecoder();
                   decoderPtr->init(*in);
               }
               std::tuple<uint8_t*,int> getBuffer(){
                   return std::make_tuple<uint8_t*,int>(buffer,BUFFSIZE);
               }
               void read(T& obj){
                   avro::codec_traits<T>::decode(*decoderPtr,obj);
               }
               void rewind(){
                   //in.release();
                   //in = avro::memoryInputStream(buffer,BUFFSIZE);
                   decoderPtr->init(*in);
               }

           private:
               uint8_t buffer[BUFFSIZE];
               std::unique_ptr<avro::InputStream> in;
               avro::DecoderPtr decoderPtr;
           };

           template<typename T>
           class AvroObjectWriter{
           public:
               typedef std::function<bool(const uint8_t*,int)> WriteDataCallback;
               AvroObjectWriter(){
                   out = avro::memoryOutputStream();
                   encoderPtr = avro::binaryEncoder();
                   encoderPtr->init(*out);
               }
               void write(std::vector<T>& v){
                   using Iter = typename std::vector<T>::iterator;
                   for(Iter iter = v.begin();iter!=v.end();iter++){
                       avro::codec_traits<T>::encode(*encoderPtr,*iter);
                   }
                   encoderPtr->flush();
                   out->flush();
               }

               void write(T* array,int size){
                   for(int i=0;i<size;i++) {
                       avro::codec_traits<T>::encode(*encoderPtr,array[i]);
                   }
                   encoderPtr->flush();
                   out->flush();
               }

               void write(const T& obj){
                   avro::codec_traits<T>::encode(*encoderPtr,obj);
                   encoderPtr->flush();
                   out->flush();
               }

               void commit(WriteDataCallback cb){
                   std::unique_ptr<avro::InputStream> is = avro::memoryInputStream(*out);
                   const uint8_t* nextBuffer = NULL;
                   size_t bufferSize = 0;
                   while(is->next(&nextBuffer,&bufferSize)){
                        cb(nextBuffer,bufferSize);
                   }
               }

           private:
               std::unique_ptr<avro::OutputStream> out;
               avro::EncoderPtr encoderPtr;
           };

       }

       namespace ip{
           inline int ipStringToInt(const std::string& ipString){
               union{
                   uchar_t c[4];
                   int d;
               } what;
               const char* p = ipString.c_str(),*p2=p;
               int i=3;
               while(i>=0){
                   if(*p!='.'&&*p!='\0'){
                       p++;
                   }else{
                       what.c[i] = atoi(p2);
                       p2=++p;
                       i--;
                       if(*p=='\0')
                           break;
                   }
               }
               return what.d;
           }

           inline std::string ipIntToString(int ip){
               union{
                   uchar_t c[4];
                   int d;
               } what;
               what.d = ip;
               char buf[16];
               sprintf(buf,"%d.%d.%d.%d",0xff&what.c[3],0xff&what.c[2],0xff&what.c[1],0xff&what.c[0]);
               return std::string(buf);
           }
       }

       namespace stringtool{

            inline std::string toupper(const std::string& input){
                return boost::to_upper_copy<std::string>(input);
            }

       }

       namespace rankingtool{

           int randomIndex(int indexCnt, const std::function<double(int)> && getScore,
                       const std::vector<int> & rankWeight = { 45, 75, 100 },
                       bool remainUseLastRank = false ,
                       bool descend = true);

       }

   }
}
 





#endif
