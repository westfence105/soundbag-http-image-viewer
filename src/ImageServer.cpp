#include <iostream>
#include <cstring>
#include <functional>
#include <filesystem>
#include <thread>

#include <sqlite3.h>

#include "PNG.hpp"

#include "ImageServer.hpp"

using namespace std::literals::string_literals;
using json = nlohmann::json;
using namespace soundbag;

ImageInfo::ImageInfo() {
  stars = 0;
}

ImageInfo::ImageInfo(const std::string& filePath) {
  std::ifstream ifs(filePath);
  if (ifs) {
    if (filePath.size() > 4 && filePath.substr(filePath.size() - 4) == ".png") {
      PNG::File pngFile(ifs, {"tEXt"});
      for (const PNG::Chunk* chunk : pngFile.chunks()) {
        if (memcmp(chunk->name(),"tEXt", 4) == 0 && memcmp(chunk->data(), "parameters", 10) == 0 && chunk->size() > 12) {
          parameters = chunk->data() + 11;
        }
      }
    }
  }
  stars = 0;
}

ImageServer::ImageServer(const std::string& host, int port, const std::string& configFile) : super(host, port, configFile) {
  f_infoDB = std::async(std::launch::async, [&] () {
    std::unordered_map<std::string,std::shared_ptr<ImageInfo>> result;
    sqlite3* db;
    sqlite3_stmt* stmt;
    int ret = sqlite3_open("./server_data.sqlite", &db);
    if (ret == SQLITE_OK) {
      const char* stmt1_str = "select count(*) from sqlite_master where type='table' and name='image_files';";
      sqlite3_prepare_v2(db, stmt1_str, strlen(stmt1_str), &stmt, NULL);
      sqlite3_reset(stmt);

      int table_count = 0;
      if (SQLITE_ROW == (ret = sqlite3_step(stmt))) {
        table_count = sqlite3_column_int(stmt, 0);
      }
      if (table_count == 0) {
        char* err;
        const char* create_stmt =
          "create table image_files("
            "library TEXT,"
            "fileName TEXT,"
            "parameters TEXT,"
            "tags TEXT,"
            "stars INTEGER,"
            "PRIMARY KEY(library, fileName)"
          ");";
        sqlite3_exec(db, create_stmt, NULL, NULL, &err);
      }
      sqlite3_reset(stmt);
      if (SQLITE_ROW == (ret = sqlite3_step(stmt))) {
        table_count = sqlite3_column_int(stmt, 0);
      }
      sqlite3_finalize(stmt);

      if (table_count) {
        const char* stmt2_str = "select * from image_files;";
        sqlite3_prepare_v2(db, stmt2_str, strlen(stmt2_str), &stmt, NULL);
        sqlite3_reset(stmt);

        while (SQLITE_ROW == (ret = sqlite3_step(stmt))) {
          auto info = std::make_shared<ImageInfo>();
          info->library = (const char*)sqlite3_column_text(stmt, 0);
          info->fileName = (const char*)sqlite3_column_text(stmt, 1);
          info->parameters = (const char*)sqlite3_column_text(stmt, 2);
          std::string tagStr = (const char*)sqlite3_column_text(stmt, 3);
          info->stars = sqlite3_column_int(stmt, 4);
          size_t pos;
          size_t offset = 0;
          while (offset < tagStr.size()) {
            pos = tagStr.find_first_of(',', offset);
            if (pos == std::string::npos) {
              info->tags.push_back(tagStr.substr(offset));
              break;
            }
            else if (offset < pos) {
              info->tags.push_back(tagStr.substr(offset, pos - offset));
            }
            offset = pos + 1;
          }
          result["/"s+ info->library +"/"+ info->fileName] = info;
        }
        sqlite3_finalize(stmt);
      }
      sqlite3_close(db);
    }

    m_threads.push_back(
      std::async(std::launch::async, [&] () {
        const auto& config = super::config();
        if (config.is_object() && config.count("media") && config["media"].is_object()) {
          for (auto iter = config["media"].begin(); iter != config["media"].end(); ++iter) {
            std::string libPath = iter.value();
            for (const auto& file : std::filesystem::directory_iterator(libPath)) {
              if (!file.is_regular_file()) continue;
              std::string path = file.path();
              std::string fileName = path.substr(libPath.size() + 1);

              std::shared_ptr<ImageInfo> info; {
                std::unique_lock lock(m_infoDB_mutex);
                for (const auto& e : infoDB()) {
                  if (e.second->library == iter.key() && e.second->fileName == fileName) {
                    info = e.second;
                  }
                }
              }

              if (!info) {
                info = std::make_shared<ImageInfo>(path);
                info->library = iter.key();
                info->fileName = fileName;

                if (info->parameters.size()) {
                  std::string parameters = info->parameters;
                  size_t rtnPos = parameters.find_first_of('\n');
                  if (rtnPos != std::string::npos && rtnPos != 0) {
                    // ignore after newline (Negative Params)
                    info->parameters = parameters.substr(0, rtnPos);
                  }
                }

                info->updateDB();
                {
                  std::unique_lock lock(m_infoDB_mutex);
                  infoDB()["/"s+ info->library +"/"+ info->fileName] = info;
                }
              }

              if (!is_loop()) break;
            }
          }
        }
      })
    );

    return result;
  });
}

void ImageServer::onStop() {
  for (auto& f : m_threads) {
    f.get();
  }
}

void ImageInfo::updateDB() const {
  std::ostringstream count_oss;
  count_oss << "select count(*) from image_files where library='"
            << library << "' and fileName='" << fileName << "';";
  sqlite3* db;
  sqlite3_stmt* stmt;
  int ret = sqlite3_open("./server_data.sqlite", &db);
  if (ret == SQLITE_OK) {
    sqlite3_prepare_v2(db, count_oss.str().c_str(), count_oss.str().size(), &stmt, NULL);
    sqlite3_reset(stmt);

    int count = 0;
    if (SQLITE_ROW == (ret = sqlite3_step(stmt))) {
      count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    std::ostringstream tags_oss;
    auto iter = tags.begin();
    while (iter != tags.end()) {
      tags_oss << *iter;
      ++iter;
      if (iter != tags.end()) {
        tags_oss << ",";
      }
    }
    std::ostringstream update_oss;
    if (count) {
      update_oss << "update image_files set "
                 << "parameters = '" << parameters << "',"
                 << "tags = '" << tags_oss.str() << "',"
                 << "stars = " << stars << " where"
                 << "library = '" << library << "',"
                 << "fileName = '" << fileName << "';";
    }
    else {
      update_oss << "insert into image_files values('"
                 << library << "','" << fileName << "','"
                 << parameters << "','" << tags_oss.str() << "'," << stars << ");";
    }
    sqlite3_prepare_v2(db, update_oss.str().c_str(), update_oss.str().size(), &stmt, NULL);
    sqlite3_reset(stmt);
    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_close(db);
  }
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
      if (!reqData.is_object() || !reqData.count("library")) {
        return HttpResponse(400);
      }

      std::string library = findMediaLibrary(reqData["library"]);
      if (library == "") {
        return HttpResponse(404);
      }

      std::unique_lock lock(m_infoDB_mutex);
      std::list<std::string> files;
      for (const auto& e : infoDB()) {
        const auto& info = e.second;
        if (info->library == reqData["library"]) {
          if (reqData.count("parameters") && reqData["parameters"].is_array()) {
            bool match = true;
            for (const auto& param : reqData["parameters"]) {
              if (info->parameters.find(param) == std::string::npos) {
                match = false;
                break;
              }
            }
            if (!match) continue;
          }

          files.push_back(info->fileName);
        }
      }

      return HttpResponse(json(files));
    }
    else if (parsedPath[1] == "favorites") {
      std::unique_lock lock(m_infoDB_mutex);
      if (request.type() == HttpRequestType::POST) {
        json reqData = json::parse(request.body());
        if (!reqData.is_object()) {
          return HttpResponse(400);
        }

        if (!reqData.count("add") && reqData["add"].is_array()) {
          for (auto it = reqData["add"].begin(); it != reqData["add"].end(); ++it) {
            std::string key = *it;
            if (infoDB().count(key)) {
              infoDB()[key]->stars = 4;
              infoDB()[key]->updateDB();
            }
          }
        }
        if (!reqData.count("remove") && reqData["remove"].is_array()) {
          for (auto it = reqData["remove"].begin(); it != reqData["remove"].end(); ++it) {
            std::string key = *it;
            if (infoDB().count(key)) {
              infoDB()[key]->stars = 0;
              infoDB()[key]->updateDB();
            }
          }
        }
      }

      std::list<std::string> result;
      for (const auto& e : infoDB()) {
        if (e.second->stars > 3) {
          result.push_back(e.first);
        }
      }
      return HttpResponse(json(result));
    }
  }
  else if (parsedPath.size() >= 3 && parsedPath[0] == "media") {

  }

  return HttpResponse(404);
}
