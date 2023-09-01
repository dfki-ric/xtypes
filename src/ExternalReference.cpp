#include "ExternalReference.hpp"
#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <regex>
// Including used XType classes
#include "ComponentModel.hpp"

using namespace xtypes;

// Constructor
xtypes::ExternalReference::ExternalReference(const std::string& classname) : _ExternalReference(classname)
{
    // NOTE: Properties and relations have been created in _ExternalReference constructor
    // You can add custom properties with
    // this->define_property("name", json type, { allowed_value1, allowed_value2, ...}, default_value [, true if you want to override a base class property ] );
    // Most importantly, if you use the registry to instantiate other XTypes not yet specified in the template
    // you have to register them here with
    // registry->register_class<YourInternallyUsedXType>();
}

// Static identifier
const std::string xtypes::ExternalReference::classname = "xtypes::ExternalReference";

std::string xtypes::ExternalReference::uri() const
{
    std::string url = std::string("drock://");
    // Make sure there are no invalid characters in the uri
    url += std::regex_replace(this->get_remote_url(), std::regex("[/:]"), "_");
    return url;
}

// Method implementations
// This function loads the remote content to local directory.
nl::json xtypes::ExternalReference::load(const std::string &local_dir)
{
    std::string filename = local_dir + "/" + std::to_string(this->uuid());

    // meta file contains the etag and last modified date to avoid refetching the file if it has not changed
    std::string metadata_filename = filename + ".meta";
    std::ifstream metadata_file(metadata_filename.c_str());
    nl::json metadata;
    if (metadata_file.good()) {
        metadata_file >> metadata;
    }
    metadata_file.close();

    // Check if the file has changed
    cpr::Response headResponse = cpr::Head(
        cpr::Url{this->get_remote_url()},
        cpr::Header{{"If-None-Match", metadata["etag"]}, {"If-Modified-Since", metadata["last_modified"]}}
    );
    if (headResponse.status_code == 304) {
        return nl::json({{"url", this->get_remote_url()}, {"local_path", filename}});
    }

    // fetch remote content
    cpr::Response response = cpr::Get(cpr::Url{this->get_remote_url()});
    if (response.status_code >= 400) {
        throw std::runtime_error("ExternalReference.load: Error fetching remote content");
    }
    std::ofstream downloaded_file(filename, std::ofstream::out);
    if (!downloaded_file.is_open()) {
        throw std::runtime_error("ExternalReference.load: Error opening file for writing");
    }
    downloaded_file << response.text;
    downloaded_file.close();

    // store metadata to allow caching
    std::ofstream metadata_file_w(metadata_filename, std::ofstream::out);
    if (!metadata_file_w.is_open()) {
        throw std::runtime_error("ExternalReference.load: Error opening metadata file for writing");
    }
    metadata["etag"] = response.header["etag"];
    metadata["last_modified"] = response.header["last-modified"];
    metadata_file_w << metadata.dump();
    metadata_file_w.close();

    return nl::json{{"url", this->get_remote_url()}, {"local_path", filename}};
}

// This function stores the local content to the remote_url
void xtypes::ExternalReference::store(const std::string& local_dir)
{
    std::string filename = local_dir + "/" + std::to_string(this->uuid());
    std::ifstream local_file(filename.c_str());
    if (!local_file.good()) {
        throw std::runtime_error("ExternalReference.store: Error opening file for reading");
    }
    local_file.close();
    cpr::Response response = cpr::Put(cpr::Url{this->get_remote_url()}, cpr::Multipart{{"file", cpr::File{filename}}});
    if (response.status_code >= 400) {
        throw std::runtime_error("ExternalReference.store: Error storing content to remote");
    }
}


// Overrides for setters of properties

// Overrides for relation setters
