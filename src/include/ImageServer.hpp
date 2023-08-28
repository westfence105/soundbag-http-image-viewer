#pragma once

#include <list>
#include <future>
#include <memory>

#include "soundbag/HttpServer.hpp"

struct ImageInfo {
  std::string library;
  std::string fileName;
  std::string parameters;
  std::list<std::string> tags;
  int stars;

  ImageInfo();
  ImageInfo(const std::string& fileName);
  void updateDB() const;
};

class ImageServer : public soundbag::HttpServer {
    typedef soundbag::HttpServer super;

    std::mutex m_infoDB_mutex;
    std::future<std::unordered_map<std::string,std::shared_ptr<ImageInfo>>> f_infoDB;
    std::unordered_map<std::string,std::shared_ptr<ImageInfo>> m_infoDB;

    std::list<std::future<void>> m_threads;

  protected:
    virtual soundbag::HttpResponse handleApi(const soundbag::HttpRequest& request);

    inline std::unordered_map<std::string,std::shared_ptr<ImageInfo>>& infoDB() {
      if (f_infoDB.valid()) {
        return (m_infoDB = f_infoDB.get());
      }
      else {
        return m_infoDB;
      }
    }

    void onStop();

  public:
    ImageServer(const std::string& host, int port, const std::string& configFile = "server_config.json");
    inline virtual ~ImageServer() {}

};
