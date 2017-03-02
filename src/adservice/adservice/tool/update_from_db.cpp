#include <ostream>
#include <string>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>


int main(int argc,const char** argv){
    std::string filename = "res/contenttype_map.txt";
    if(argc>1){
        filename = argv[1];
    }
    try {
      sql::Driver *driver;
      sql::Connection *con;
      sql::Statement *stmt;
      sql::ResultSet *res;
      FILE* latestContentTypeMappingFile = NULL;
      if((latestContentTypeMappingFile=fopen(filename.c_str(),"w"))==NULL){
          std::cerr<<"failed to open res/contenttype_map.txt for write.";
          return 0;
      }
      driver = get_driver_instance();
      con = driver->connect("tcp://rm-2zek1ct2g06ergoum.mysql.rds.aliyuncs.com:3306", "mtty_root", "Mtkj*8888");
      con->setSchema("mtdb");
      stmt = con->createStatement();
      res = stmt->executeQuery("select CONCAT(adxid,'|',adx_category_id,'|',common_category_id) as mapping from content_category_map");

      while (res->next()) {
        std::string mapping = res->getString("mapping");
        fprintf(latestContentTypeMappingFile,"%s\n",mapping.c_str());
      }
      delete res;
      delete stmt;
      delete con;
      fclose(latestContentTypeMappingFile);
    } catch (sql::SQLException &e) {
      std::cerr << "sql::SQLException:"<<e.what()<<",errorcode:"<<e.getErrorCode()<<",sqlstate:"<<e.getSQLState()<<std::endl;
    }
    return 0;
}
