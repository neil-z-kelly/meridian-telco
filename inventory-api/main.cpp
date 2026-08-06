#include <cstdio>
#include <cstdlib>
#include <string>
#include "httpd.h"
#include "../mediation/store.h"
#include "../mediation/capacity.h"
#include "../mediation/status.h"
#include "../mediation/circuit_counter.h"

/* inventory api. GET /sites , GET /circuits
   field names are the store's field names, downstream depends on them. */

static Store g_store;

static std::string site_json(Row &r) {
  char buf[1024];
  snprintf(buf, sizeof(buf),
           "{\"ASSET_ID\":%s,\"SITE_NM\":\"%s\",\"SITE_CD\":\"%s\",\"REGION_CD\":\"%s\","
           "\"LAT\":%s,\"LON\":%s,\"STATUS_CD\":%s,\"STATUS_LABEL\":\"%s\","
           "\"TOWER_REG\":\"%s\",\"TOTAL_CAP_MBPS\":%s,\"ALLOC_CAP_MBPS\":%s,"
           "\"AVAIL_CAP_MBPS\":%d}",
           r["ASSET_ID"].c_str(), json_escape(r["SITE_NM"]).c_str(), r["SITE_CD"].c_str(),
           r["REGION_CD"].c_str(), r["LAT"].c_str(), r["LON"].c_str(),
           r["STATUS_CD"].c_str(), status_label(to_int(r["STATUS_CD"])).c_str(),
           r["TOWER_REG"].c_str(), r["TOTAL_CAP_MBPS"].c_str(), r["ALLOC_CAP_MBPS"].c_str(),
           available_capacity(to_int(r["TOTAL_CAP_MBPS"]), to_int(r["ALLOC_CAP_MBPS"])));
  return buf;
}

static std::string handle_sites(const HttpRequest &req, int *status) {
  (void)status;
  std::string sql = "SELECT * FROM SITE";
  std::map<std::string, std::string> q = req.query;
  if (q.count("REGION_CD")) sql += " WHERE REGION_CD='" + q["REGION_CD"] + "'";
  else if (q.count("STATUS_CD")) sql += " WHERE STATUS_CD=" + q["STATUS_CD"];
  sql += " ORDER BY ASSET_ID";
  std::vector<Row> rows = g_store.query(sql);
  std::string out = "{\"count\":";
  char n[32];
  snprintf(n, sizeof(n), "%d", (int)rows.size());
  out += n;
  out += ",\"sites\":[";
  for (size_t i = 0; i < rows.size(); i++) {
    if (i) out += ",";
    out += site_json(rows[i]);
  }
  out += "]}";
  return out;
}

static std::string handle_circuits(const HttpRequest &req, int *status) {
  (void)status;
  std::map<std::string, std::string> q = req.query;
  std::string sql = "SELECT * FROM CIRCUIT";
  if (q.count("CIRCUIT_ID")) sql += " WHERE CIRCUIT_ID='" + q["CIRCUIT_ID"] + "'";
  sql += " ORDER BY CIRCUIT_ID";
  std::vector<Row> rows = g_store.query(sql);
  std::string out = "{\"count\":";
  char n[32];
  snprintf(n, sizeof(n), "%d", (int)rows.size());
  out += n;
  out += ",\"active_count\":";
  snprintf(n, sizeof(n), "%d", count_active_circuits(rows));
  out += n;
  out += ",\"circuits\":[";
  for (size_t i = 0; i < rows.size(); i++) {
    Row r = rows[i];
    char buf[1024];
    snprintf(buf, sizeof(buf),
             "{\"CIRCUIT_ID\":\"%s\",\"CIRCUIT_NM\":\"%s\",\"A_ASSET_ID\":%s,"
             "\"Z_ASSET_ID\":%s,\"CAP_MBPS\":%s,\"ALLOC_MBPS\":%s,\"ROLE_CD\":%s,"
             "\"STATUS_CD\":%s,\"AVAIL_MBPS\":%d}",
             r["CIRCUIT_ID"].c_str(), json_escape(r["CIRCUIT_NM"]).c_str(),
             r["A_ASSET_ID"].c_str(), r["Z_ASSET_ID"].c_str(), r["CAP_MBPS"].c_str(),
             r["ALLOC_MBPS"].c_str(), r["ROLE_CD"].c_str(), r["STATUS_CD"].c_str(),
             available_capacity(to_int(r["CAP_MBPS"]), to_int(r["ALLOC_MBPS"])));
    if (i) out += ",";
    out += buf;
  }
  out += "]}";
  return out;
}

int main(int argc, char **argv) {
  std::string dbpath = "meridian.db";
  int port = 8081;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--db" && i + 1 < argc) dbpath = argv[++i];
    else if (a == "--port" && i + 1 < argc) port = atoi(argv[++i]);
  }
  if (!g_store.open(dbpath)) return 1;
  std::vector<Route> routes;
  Route r1; r1.prefix = "/sites"; r1.fn = handle_sites; routes.push_back(r1);
  Route r2; r2.prefix = "/circuits"; r2.fn = handle_circuits; routes.push_back(r2);
  return serve(port, routes);
}
