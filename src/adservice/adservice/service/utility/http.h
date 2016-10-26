//
// Created by guoze.lin on 16/9/21.
//

#ifndef MTCORENGINX_HTTP_H
#define MTCORENGINX_HTTP_H

#include <map>
#include <string>
#include <sstream>

namespace adservice {
    namespace utility {


#define URI             "URI"
#define QUERYSTRING     "QUERY"
#define COOKIE          "Cookie"
#define QUERYMETHOD     "Method"
#define USERAGENT       "User-Agent"
#define REMOTEADDR      "X-Forwarded-For"
#define REFERER         "Referer"
#define CONTENTTYPE     "Content-Type"
#define CONTENTLENGTH   "Content-Length"
#define SETCOOKIE       "Set-Cookie"

        class HttpRequest{
        public:
            void set(const std::string& key,const std::string& value){
                headers.insert(std::make_pair(key,value));
            }

            std::string get(const std::string& key) const{
                std::map<std::string,std::string>::const_iterator iter = headers.find(key);
                if(iter !=headers.end()){
                    return iter->second;
                }
                return "";
            }

            std::string path_info(){
                return get(URI);
            }

            std::string request_method(){
                return get(QUERYMETHOD);
            }

            std::string query_string(){
                return get(QUERYSTRING);
            }

            std::string http_cookie(){
                return get(COOKIE);
            }

            std::string http_user_agent(){
                return get(USERAGENT);
            }

            std::string remote_addr(){
                return get(REMOTEADDR);
            }

            std::string http_referer(){
                return get(REFERER);
            }

            void set_post_data(const std::string& data){
                postdata = data;
            }

            const std::string& raw_post_data() const{
                return postdata;
            }

            std::string to_string(){
                std::stringstream ss;
                for(auto iter = headers.begin();iter!=headers.end();iter++){
                    if(!iter->first.empty()&&!iter->second.empty()) {
                        ss << iter->first << ":" << iter->second << "\r\n";
                    }
                }
                if(postdata.length()>0){
                    ss<<"\r\n"<<postdata;
                }
                return ss.str();
            }


        private:
            std::map<std::string,std::string> headers;
            std::string postdata;
        };

        class HttpResponse{
        public:
            void set(const std::string& key,const std::string& value){
                headers[key] = value;
            }

            void set_header(const std::string& key,const std::string& value){
                headers[key] = value;
            }

            std::string get(const std::string& key) const{
                std::map<std::string,std::string>::const_iterator iter = headers.find(key);
                if(iter !=headers.end()){
                    return iter->second;
                }
                return "";
            }

            void status(int code,const std::string& msg = ""){
                respStatus = code;
                statusMsg = msg;
            }

            int status() const{
                return respStatus;
            }

            const std::string& status_message() const{
                return statusMsg;
            }

            void set_content_header(const std::string& contenttype){
                headers[CONTENTTYPE] = contenttype;
            }

            std::string content_header() const{
                std::map<std::string,std::string>::const_iterator iter;
                if((iter=headers.find(CONTENTTYPE))==headers.end())
                    return "";
                return iter->second;
            }

            std::string cookies() const{
                std::map<std::string,std::string>::const_iterator iter;
                if((iter=headers.find(SETCOOKIE))==headers.end())
                    return "";
                return iter->second;
            }

            void set_body(const std::string& bodyMessage){
                if(bodyMessage.length()==0)
                    return;
                bodyStream.str("");
                bodyStream<<bodyMessage;
                headers[CONTENTLENGTH] = std::to_string(bodyStream.tellp());
            }

            const std::string& get_body(){
                if(bodyStream.tellp()>0)
                    return std::move(bodyStream.str());
                else
                    return std::move(std::string(""));
            }

            void appendToBody(const std::string& more){
                bodyStream << more;
            }

            template<typename T>
            HttpResponse& operator<<(const T& t){
                bodyStream<<t;
                return *this;
            }

            std::string to_string(){
                std::stringstream ss;
                ss<<"HTTP/1.1 "<<respStatus<<" "<<statusMsg<<"\r\n";
                for(auto iter = headers.begin();iter!=headers.end();iter++){
                    if(!iter->first.empty()&&!iter->second.empty()) {
                        ss << iter->first << ":" << iter->second << "\r\n";
                    }
                }
                if(bodyStream.tellp()>0){
                    ss<<"\r\n"<<bodyStream.str();
                }
                return ss.str();
            }

            const std::map<std::string,std::string>& get_headers() const{
                return headers;
            };

        private:
            int respStatus;
            std::string statusMsg;
            std::map<std::string,std::string> headers;
            std::stringstream bodyStream;
        };
    }
}


#endif //MTCORENGINX_HTTP_H
