#pragma once

#include <unordered_map>
#include <future>
#include <memory>

#include "soundbag/HttpServer.hpp"

struct ImageInfo {
  std::string fileName;
  std::string parameters;

  ImageInfo(const std::string& fileName);
};

class ImageServer : public soundbag::HttpServer {
    typedef soundbag::HttpServer super;

    std::future<std::unordered_map<std::string,std::shared_ptr<ImageInfo>>> f_infoDB;
    std::unordered_map<std::string,std::shared_ptr<ImageInfo>> m_infoDB;

  protected:
    virtual soundbag::HttpResponse handleApi(const soundbag::HttpRequest& request);

    inline const std::unordered_map<std::string,std::shared_ptr<ImageInfo>>& infoDB() {
      if (f_infoDB.valid()) {
        return (m_infoDB = f_infoDB.get());
      }
      else {
        return m_infoDB;
      }
    }

  public:
    ImageServer(const std::string& host, int port, const std::string& configFile = "server_config.json");
    inline virtual ~ImageServer() {}
};
