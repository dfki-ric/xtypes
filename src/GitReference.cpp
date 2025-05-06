#include "GitReference.hpp"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <regex>
// Including used XType classes
#include "ComponentModel.hpp"

using namespace xtypes;

// Constructor
xtypes::GitReference::GitReference(const std::string& classname) : _GitReference(classname)
{
    // NOTE: Properties and relations have been created in _GitReference constructor
    // You can add custom properties with
    // this->define_property("name", json type, { allowed_value1, allowed_value2, ...}, default_value [, true if you want to override a base class property ] );
    // Most importantly, if you use the registry to instantiate other XTypes not yet specified in the template
    // you have to register them here with
    // registry->register_class<YourInternallyUsedXType>();
}

std::string xtypes::GitReference::uri() const
{
    std::string url = std::string("drock://");
    // Make sure there are no invalid characters in the uri
    url += std::regex_replace(this->get_remote_url(), std::regex("[/:]"), "_");
    url += '/' + this->get_revision_name();
    return url;
}

// Static identifier
const std::string xtypes::GitReference::classname = "xtypes::GitReference";

// Method implementations
nl::json xtypes::GitReference::load(const std::string& local_dir)
{
    // REVIEW this:
    return this->checkout_repository(local_dir);
}

/// This function stores the local content to the remote_url
void xtypes::GitReference::store(const std::string& local_dir)
{
    assert(!this->get_read_only());
    // REVIEW this:
    this->update_repository({"*"});
}

// This function creates a new GIT repository
nl::json xtypes::GitReference::create_repository(const std::string& local_dir, const std::string& username, const std::string& password)
{
  // Check if local repo already created, if not create it, otherwise throw
  if (m_repo) // if m_repo is created, throw an exception
      throw std::runtime_error("GitReference:create_repository: repo already exists");
  std::string repository = get_remote_url();
  if (!repository.empty())
      throw std::runtime_error("GitReference::create_repository: found repository information " + repository + " Use checkout_repository!");

  const std::string uuid_string = std::to_string(this->uuid());
  if (get_revision_type() != "BRANCH")
    throw std::runtime_error("GitReference::create_repository: Can't create a " + repository + " on other than a branch!");
  const std::string branch = (get_revision_name() != "DEFAULT") ? get_revision_name() : m_repo->current_branch_name();

  const fs::path repo_dir = fs::path(local_dir) / uuid_string;
  if (fs::exists(repo_dir))
      throw std::runtime_error("GitReference::create_repository: " + repo_dir.string() + " directory already exists");

  // Creates a new repository
  m_repo = std::make_shared<Repository>(username, password);
  m_repo->create(repo_dir);
  // Add remote
  if (get_remote_url().empty())
      throw std::runtime_error("GitReference.create_repository: Cannot proceed without valid URL");
  const std::string url_https = this->convert_url_to_https(get_remote_url(), username, password);
  m_repo->add_remote("origin", url_https);
  //m_repo->fetch("origin");
  // Workaround: Master branch will be created by default
  // // Add some README.md and commit
  // const fs::path file_path = repo_dir / fs::path("README.md");
  // if (std::ofstream ofs{file_path})
  // {
  //     ofs << "# Model repository of component model " + name + " \n\nContains extended information for the Q-Rock component model " + name + " in domain " + domain;
  //     ofs.close();
  // }
  // else
  //     throw std::runtime_error("GitReference::create_repository: Failed to create file " + file_path.string());
  // m_repo->add("README.md");
  m_repo->commit("Initial commit");
  // Push initial commit to remote
  m_repo->push("origin", m_repo->current_branch_name());
  // Create a new branch and set head to it
  m_repo->create_branch(branch);
  m_repo->checkout(branch);

  // To be sure, that everything is up-to-date, call checkout_repository
  return this->checkout_repository(local_dir);
}

// This function checks out an existing GIT repository from remote
nl::json xtypes::GitReference::checkout_repository(const std::string& local_dir, const std::string& username, const std::string& password, const bool& use_https, const bool& only_clone)
{
  // Check local info first
  std::string revision_type = this->get_revision_type();
  std::transform(revision_type.begin(), revision_type.end(), revision_type.begin(), ::tolower);
  if (m_repo)
      return nl::json({{"url", this->get_remote_url()}, {revision_type, get_revision_name()}});
  // Get & check repository information
  std::string remote_url = this->get_remote_url();
  if (remote_url.empty())
      throw std::runtime_error("GitReference.checkout_repository: no remote_url information available");
  if (remote_url.find("git") == std::string::npos)
      throw std::runtime_error("GitReference.checkout_repository: no 'git' in " + remote_url);

  const std::string uuid = std::to_string(this->uuid());
  const fs::path repo_dir = fs::path(local_dir) / uuid;
  // Check if local_dir already contains a valid GIT
  if (fs::exists(repo_dir))
  {
      // In case the git exists, we just create and fetch info
      m_repo = std::make_shared<Repository>(username, password);
      m_repo->open(repo_dir);
      if (!only_clone)
      {
          m_repo->pull("origin");
      }
  }
  else
  {
      if (!use_https)
      {
          try
          {
              // Create directory to clone into it
              fs::create_directories(repo_dir);
              // Clone & open repository into the directory
              m_repo.reset();
              if (revision_type == "branch")
                m_repo = Repository::clone(remote_url, repo_dir, get_revision_name().c_str());
              else
                m_repo = Repository::clone(remote_url, repo_dir);
          }
          catch (const std::exception &ex)
          {
              // TODO: make this param non-const in generator
              const_cast<bool &>(use_https) = true;
          }
      }
      else
      {
        remote_url = this->convert_url_to_https(remote_url, username, password);
        // Otherwise, clone the git from remote
        try
        {
            // Create directory to clone into it
            fs::create_directories(repo_dir);
            // Clone & open repository into the directory
            m_repo.reset();
            m_repo = Repository::clone(remote_url, repo_dir);
        }
        catch (const std::exception &ex)
        {
            throw std::runtime_error("GitReference::checkout_repository: could not clone from " + remote_url + " Reason: " + ex.what());
        }
      }
  }
  const std::string revision = (get_revision_name() != "DEFAULT") ? get_revision_name() : m_repo->current_branch_name();
  // If the branch has not been created, do so. Otherwise switch to the branch first
  if (revision_type == "branch" && !m_repo->branch_exists(revision))
  {
      m_repo->create_branch(revision);
      m_repo->checkout(revision);
  }
  else
  {
      m_repo->checkout(revision);
  }
  // Pull and update repo info (as string)
  if (revision_type == "branch")
  {
      if (!m_repo->remote_exists("origin"))
          m_repo->add_remote("origin", get_remote_url());
      m_repo->pull("origin", revision);
  }
  return nl::json{{"url", remote_url}, {"local_path", repo_dir}, {revision_type, revision}};
}

// This function updates an existing GIT repository
nl::json xtypes::GitReference::update_repository(const std::vector<std::string>& new_files, const std::string& custom_message)
{
  // Make sure, that we are on the correct branch
  const std::string branch = (get_revision_name() != "DEFAULT") ? get_revision_name() : m_repo->current_branch_name();
  if (get_revision_type() != "BRANCH" || !m_repo->branch_exists(branch))
      throw std::runtime_error("Repository is pointed to a revision "+branch+" which is not a branch and thus can't be updated!");

  m_repo->checkout(branch);

  // We now can have untracked files and modified files
  // First, add changes of modified files
  const std::vector<git_diff_delta> diffs = m_repo->diff();
  bool should_commit_and_push = false;
  for (std::size_t i = 0; i < diffs.size(); i++)
  {
      const auto &diff = diffs[i];
      const std::string new_file = diff.new_file.path;
      switch (diff.status)
      {
      // if the file is modifed, add it to index
      case git_delta_t::GIT_DELTA_MODIFIED:
      {
          m_repo->add(new_file);
          should_commit_and_push = true;
          break;
      }
      // if the file is untracked, check if its in the new_files list, then add it to index
      case git_delta_t::GIT_DELTA_UNTRACKED:
          if (!new_files.empty())
          {
            // handle * (all)
            if (std::find(new_files.begin(), new_files.end(), "*") != new_files.end())
            {
                m_repo->add(new_file);
                should_commit_and_push = true;
            }
            // handle files filter
            else if (std::find(new_files.begin(), new_files.end(), new_file) != new_files.end())
            {
                m_repo->add(new_file);
                should_commit_and_push = true;
            }
          }
          break;
      default:
          // ignore, we only look for modified & untracked files for now.
          break;
      }
  }

  // Check if we need to commit & push
  if (should_commit_and_push)
      // Commit local changes
      m_repo->commit("GitReference::update_repository() on " + this->get_name() + '\n' + custom_message);

  // Pull remote (get remote changes and merge with local changes)
  if(!m_repo->remote_exists("origin"))
    m_repo->add_remote("origin", get_remote_url());
  m_repo->fetch("origin");
  m_repo->pull("origin", branch);

  // Push merged local and remote changes if we need to
  if (should_commit_and_push)
      m_repo->push("origin", branch);

  return nl::json{{"url", get_remote_url()}, {"local_path", m_repo->get_repository_directory()}, {"branch", m_repo->current_branch_name()}};
}

// This method takes a GIT url and converts it to an HTTPS url
std::string xtypes::GitReference::convert_url_to_https(const std::string& url, const std::string& username, const std::string& password)
{
    std::string protocol, credentials, host, port, path;
    auto split = [](std::string s, std::string delimiter) -> std::vector<std::string>
    {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        if ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
        {
            token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(token);
        }

        res.push_back(s.substr(pos_start));
        return res;
    };
    std::cout<<url<<std::endl;

    if (url.find("git@") == 0) {
        protocol = "ssh";
        std::vector<std::string> tokens = split(url, ":");
        host = tokens[0];
        path = tokens[1];
    } else {
        assert(url.find("://") != std::string::npos);
        std::vector<std::string> tokens = split(url, "://");
        protocol = tokens[0];
        if (tokens[1].find("@") != std::string::npos) {
            tokens = split(tokens[1], "@");
            credentials = tokens[0];
        }
        tokens = split(tokens[1], "/");
        host = tokens[0];
        path = tokens[1];
        if (host.find(":") != std::string::npos) {
            tokens = split(host, ":");
            host = tokens[0];
            port = tokens[1];
        }
    }
    // now we are finished parsing so let's assemble
    std::string out = "https://";
    if (!username.empty() && !password.empty())
        out += username+":"+password+"@";
    else if (!credentials.empty())
        out += credentials+"@";
        
    out += host;
    if (!port.empty())
        out +=":"+port;

    return out+"/"+path;
}
