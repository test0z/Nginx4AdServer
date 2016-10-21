#ifndef GZIP_H
#define GZIP_H

#include <zlib.h>
#include <sstream>
#include <string>


class ZlibOutputStream
{
 public:
  explicit ZlibOutputStream():
      zerror_(Z_OK)
  {
    bzero(&zstream_, sizeof zstream_);
    zerror_=  deflateInit2(&zstream_, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS+16, 8,  Z_DEFAULT_STRATEGY);
  }

  ~ZlibOutputStream()
  {
    finish();
  }


  const char* zlibErrorMessage() const { return zstream_.msg; }

  int zlibErrorCode() const { return zerror_; }
  int64_t inputBytes() const { return zstream_.total_in; }
  int64_t outputBytes() const { return zstream_.total_out; }
  int internalOutputBufferSize() const { return bufferSize_; }


  bool write(const char* input,int64_t contentLength)
  {
    if (zerror_ != Z_OK)
      return false;

    void* in = static_cast<void*>(input);
    zstream_.next_in = static_cast<Bytef*>(in);
    zstream_.avail_in = static_cast<int>(contentLength);
    if (zstream_.avail_in > 0 && zerror_ == Z_OK)
    {
      zerror_ = compress(Z_NO_FLUSH);
    }
    return zerror_ == Z_OK;
  }

  bool finish()
  {
    if (zerror_ != Z_OK)
      return false;

    while (zerror_ == Z_OK)
    {
      zerror_ = compress(Z_FINISH);
    }
    zerror_ = deflateEnd(&zstream_);
    bool ok = zerror_ == Z_OK;
    zerror_ = Z_STREAM_END;
    return ok;
  }

  std::string getResult(){
        return sstream_.str();
  }

 private:
  int compress(int flush)
  {
    zstream_.next_out = reinterpret_cast<Bytef*>(buffer_);
    zstream_.avail_out = sizeof(buffer_);
    int error = ::deflate(&zstream_, flush);
    sstream_<<std::string(buffer_,buffer_+zstream_.avail_out);
    return error;
  }

  char buffer_[1024];
  z_stream zstream_;
  int zerror_;
  std::stringstream sstream_;
};

#endif // GZIP_H
