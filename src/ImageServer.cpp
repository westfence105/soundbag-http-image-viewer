#include <iostream>
#include <cstring>
#include <functional>
#include <filesystem>

#include "PNG.hpp"

#include "ImageServer.hpp"

using json = nlohmann::json;
using namespace soundbag;

ImageInfo::ImageInfo(const std::string& fileName) {
  std::ifstream ifs(fileName);
  if (ifs) {
    if (fileName.size() > 4 && fileName.substr(fileName.size() - 4) == ".png") {
      PNG::File pngFile(ifs, {"tEXt"});
      for (const PNG::Chunk* chunk : pngFile.chunks()) {
        if (memcmp(chunk->name(),"tEXt", 4) == 0 && memcmp(chunk->data(), "parameters", 10) == 0 && chunk->size() > 12) {
          parameters = chunk->data() + 11;
        }
      }
    }
  }
}

ImageServer::ImageServer(const std::string& host, int port, const std::string& configFile) : super(host, port, configFile) {
  f_infoDB = std::async(std::launch::async, [&] () {
    std::unordered_map<std::string, std::shared_ptr<ImageInfo>> result;
    const auto& config = super::config();
    if (config.is_object() && config.count("media") && config["media"].is_object()) {
      for (auto iter = config["media"].begin(); iter != config["media"].end(); ++iter) {
        for (const auto& file : std::filesystem::directory_iterator(iter.value())) {
          if (!file.is_regular_file()) continue;
          std::string path = file.path();
          auto info = std::make_shared<ImageInfo>(path);
          result[path] = info;

          if (info->parameters.size()) {
            std::string parameters = info->parameters;
            size_t rtnPos = parameters.find_first_of('\n');
            if (rtnPos != std::string::npos && rtnPos != 0) {
              // ignore after newline (Negative Params)
              parameters = parameters.substr(0, rtnPos);
            }
          }

          if (!is_loop()) break;
        }
      }
    }
    return result;
  });
}

HttpResponse ImageServer::handleApi(const HttpRequest& request) {
  const std::string& path = request.path().substr(5);
  const auto parsedPath = splitPath(path);
  if (parsedPath.size() == 2 && parsedPath[0] == "media") {
    if (parsedPath[1] == "library-list") {
      const auto& config = super::config();
      if (!config.is_object() || !config.count("media") || !(config["media"].is_object())) {
        return HttpResponse(404);
      }
      else {
        std::list<std::string> names;
        for (auto iter = config["media"].begin(); iter != config["media"].end(); ++iter) {
          names.push_back(iter.key());
        }
        return HttpResponse(json(names));
      }
    }
    else if (parsedPath[1] == "search") {
      json reqData = json::parse(request.body());
      std::cout << reqData << std::endl;
      if (!reqData.is_object() || !reqData.count("library")) {
        return HttpResponse(400);
      }

      std::string library = findMediaLibrary(reqData["library"]);
      std::cout << "library='" << library << "'" << std::endl;
      if (library == "") {
        return HttpResponse(404);
      }

      std::list<std::string> files;
      for (const auto& e : infoDB()) {
        std::cout << e.first << std::endl;
        if (e.first.size() > library.size() && e.first.substr(0, library.size()) == library) {
          if (reqData.count("parameters") && reqData["parameters"].is_array()) {
            std::string parameters = e.second->parameters;
            size_t rtnPos = parameters.find_first_of('\n');
            if (rtnPos != std::string::npos && rtnPos != 0) {
              // ignore after newline (Negative Params)
              parameters = parameters.substr(0, rtnPos);
            }
            bool match = true;
            for (const auto& param : reqData["parameters"]) {
              if (parameters.find(param) == std::string::npos) {
                match = false;
                break;
              }
            }
            if (!match) continue;
          }

          files.push_back(e.first);
        }
      }

      for (std::string& file : files) {
        if (file.size() <= library.size()) {
          std::cerr << "Path could not replaced: " << file << std::endl;
          continue;
        }
        file.replace(0, library.size() + 1, "");
      }

      return HttpResponse(json(files));
    }
  }
  return HttpResponse(404);
}
