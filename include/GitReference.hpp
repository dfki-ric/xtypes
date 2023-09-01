/**
 * Auto-generated with xtypes_generator types_generator 04/18/2023 11:48:39
 */

#pragma once
#include "_GitReference.hpp"
#include "git_wrapper.hpp"

namespace xtypes {
    // Forward Declarations

    /// taking repository ptr
    using RepositoryPtr = std::shared_ptr<Repository>;

    class GitReference : public _GitReference
    {

        private:
            RepositoryPtr m_repo;
        public:
            /// Constructor
            GitReference(const std::string& classname = GitReference::classname);

            // Static indentifier
            /// Useful to lookup the derived classname at compile time
            static const std::string classname;

            /// Custom URI generator (overrides default implementation in _ExternalReference)
            std::string uri() const override;

            // Method Declarations
            /// This function creates a new GIT repository
            virtual nl::json create_repository(const std::string& local_dir = ".", const std::string& username = "", const std::string& password = "");
            
            /// This function checks out an existing GIT repository from remote
            virtual nl::json checkout_repository(const std::string& local_dir = ".", const std::string& username = "", const std::string& password = "", const bool& use_https = true, const bool& only_clone = false);
            
            /// This function updates an existing GIT repository
            virtual nl::json update_repository(const std::vector<std::string>& new_files, const std::string& custom_message = "");
            
            /// This method takes a GIT url and converts it to an HTTPS url
            static std::string convert_url_to_https(const std::string& url, const std::string& username, const std::string& password);

            /// This function loads the remote content to local directory.
            virtual nl::json load(const std::string& local_dir = ".") override;
            
            /// This function stores the local content to the remote_url
            virtual void store(const std::string& local_dir = ".") override;
            
            // Overrides for setters of properties
            // Overrides for relation setters
    };

    using GitReferencePtr = std::shared_ptr<GitReference>;
    using GitReferenceCPtr = const std::shared_ptr<GitReference> ;
    using ConstGitReferencePtr = std::shared_ptr<const GitReference> ;
    using ConstGitReferenceCPtr = const std::shared_ptr<const GitReference> ;
}
