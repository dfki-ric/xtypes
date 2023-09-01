#pragma once
#include <string>
#include <iostream>
#include <git2.h>
#include <mutex>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif

/**
 * @brief Minimal libgit2 C++ wrapper (has been adapted by @hwiedemann)
 */

#define GIT_CHECK_ERROR(func) \
    if ((func) < 0)           \
    throw std::runtime_error(#func + std::string(": ") + giterr_last()->message + std::string("\n\tat: ") + __PRETTY_FUNCTION__ + std::string("\n\tat: ") + std::string(__FILE__) + ':' + std::to_string(__LINE__))

namespace xtypes
{
    /**
     * @brief A Git repository class
     */
    class Repository
    {
    public:
        /**
         * @brief Construct a new Repository
         *
         * @param username: Optional HTTPS login username
         * @param password: Optional HTTPS login password
         * @note: Only when SSH is not supported, https will be used.
         */
        Repository(const std::string &username = "", const std::string &password = "")
            : m_username(username),
              m_password(password),
              m_repository(nullptr)
        {
            git_libgit2_init();
        }

        ~Repository()
        {
            if (m_repository)
                git_repository_free(m_repository);
            git_libgit2_shutdown();
        }

        /**
         * @brief Returns true if this repo instance is created or opened
         *
         * @return true if repo is created/opened, otherwise false
         */
        operator bool()
        {
            return m_repository != nullptr;
        }

    public:
        /**
         * @brief Creates a new local repository (equivalent to: git init)
         *
         * @param local_dir: Local directory to create repository at
         */
        void create(const fs::path &local_dir)
        {
            // If m_repository ptr is not nullptr, repository was already created
            if (m_repository != nullptr)
                throw std::runtime_error("Repository::create: repo already exists");

            if (fs::exists(local_dir))
                throw std::runtime_error("Repository::create: " + local_dir.string() + " directory already exists");

            // Creates a new repository
            const int res = git_repository_init(&m_repository, local_dir.string().c_str(), false);

            // git_repository_init returns 0 on success.
            if (res != 0)
                throw std::runtime_error("Repository::create: Could not initialize repository");
        }

        /**
         * @brief Opens a existing local repository
         *
         * @param repo_dir: Path to existing repository directory
         */
        void open(const fs::path &repo_dir)
        {
            if (m_repository != nullptr) // close any previously opened repository.
                git_repository_free(m_repository);
            if (git_repository_open(&m_repository, repo_dir.string().c_str()) < 0)
                throw std::runtime_error("Repository::open: Failed to open existing repository " + repo_dir.string());
        }

        /**
         * @brief Get the repository working directory
         *
         * @return repository's working directory
         */
        fs::path get_repository_directory()
        {
            return fs::path(git_repository_workdir(m_repository));
        }

        /**
         * @brief Adds remote to the repository
         *
         * @param name: remote name example "origin"
         * @param url: remote url
         */
        void add_remote(const std::string &name, const std::string &url)
        {
            // Add remote
            if (url.empty())
                throw std::runtime_error("Repository::add_remote: Cannot proceed without valid URL");
            git_remote *remote;
            const int res = git_remote_create(&remote, m_repository, name.c_str(), url.c_str());
            if (!remote)
                throw std::runtime_error("Repository::add_remote: Could not add remote " + url + " to new repository");
            git_remote_free(remote);
            if (res == GIT_EEXISTS)
                throw std::runtime_error("Repository::add_remote: Remote " + url + " already added");
            else if (res != 0)
                throw std::runtime_error("Repository::add_remote: Could not add remote " + url);
        }

        /**
         * @brief Returns true if remote exists
         * 
         * @param remote_name: remote name to look for
         */
        bool remote_exists(const std::string& remote_name) {
            git_remote *remote = nullptr;
            int error = git_remote_lookup(&remote, m_repository, remote_name.c_str());
            if (error == GIT_OK) {
                git_remote_free(remote);
                return true;
            }
            return false;
        }

        /**
         * @brief Add file contents to the index
         *
         * @param filename: Path to file to add
         */
        void add(const fs::path &filename)
        {
            git_index *index = nullptr;
            // get index
            GIT_CHECK_ERROR(git_repository_index(&index, m_repository));
            // git add file
            GIT_CHECK_ERROR(git_index_add_bypath(index, filename.string().c_str())); // perform git add filename
            GIT_CHECK_ERROR(git_index_write(index));
            git_index_free(index);
        }

        /**
         * @brief Add all files contents to the index
         */
        void add_all()
        {
            git_index *index = nullptr;
            GIT_CHECK_ERROR(git_repository_index(&index, m_repository));
            GIT_CHECK_ERROR(git_index_add_all(index, nullptr, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr));
            /* Write the in-memory index to disk */
            GIT_CHECK_ERROR(git_index_write(index));
            git_index_free(index);
        }

        /**
         * @brief Record changes to the repository and commit with a message
         *
         * @param message: commit message
         */
        void commit(const std::string &message)
        {
            git_signature *sig{};
            git_index *index{};
            git_oid tree_id{}, commit_id{};
            git_tree *tree{};

            // Create a new action signature
            GIT_CHECK_ERROR(git_signature_default(&sig, m_repository));
            // Get the Index file for this repository.
            GIT_CHECK_ERROR(git_repository_index(&index, m_repository));
            // Write the index as a tree
            GIT_CHECK_ERROR(git_index_write_tree(&tree_id, index));
            // Lookup a tree object from the repository.
            GIT_CHECK_ERROR(git_tree_lookup(&tree, m_repository, &tree_id));
            const git_commit *last_commits[1] = {this->get_last_commit()};
            // Perform commit
            GIT_CHECK_ERROR(git_commit_create(&commit_id,
                                              m_repository,
                                              "HEAD",
                                              sig,
                                              sig,
                                              "UTF-8",
                                              message.c_str(),
                                              tree,
                                              1, last_commits));
            git_index_free(index);
            git_tree_free(tree);
            git_signature_free(sig);
        }

        /**
         * @brief Upload local repository content to a remote repository
         *
         * @param remote_name: remote name example "origin"
         * @param branch_name: branch name example "master"
         */
        void push(const std::string &remote_name = "origin", const std::string &branch_name = "master")
        {
            const std::string str = "+refs/heads/" + branch_name;
            const char *refspec[1] = {str.c_str()};
            const git_strarray refspecs = {
                const_cast<char **>(refspec),
                1,
            };
            git_remote *remote;
            GIT_CHECK_ERROR(git_remote_lookup(&remote, m_repository, remote_name.c_str()));
            git_push_options options;
            GIT_CHECK_ERROR(git_push_init_options(&options, GIT_PUSH_OPTIONS_VERSION));
            options.callbacks.credentials = acquire_credentials;
            options.callbacks.payload = this;
            GIT_CHECK_ERROR(git_remote_push(remote, &refspecs, &options));
            git_remote_free(remote);
        }

        /**
         * @brief Download objects and refs from another repository
         *
         * @param remote_name: remote name. "origin" by default.
         */
        void fetch(const std::string &remote_name = "origin")
        {
            git_remote *remote;

            GIT_CHECK_ERROR(git_remote_lookup(&remote, m_repository, remote_name.c_str()));
            git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
            opts.callbacks.credentials = acquire_credentials;
            opts.callbacks.payload = this;
            GIT_CHECK_ERROR(git_remote_fetch(remote, nullptr, &opts, "fetch"));
            git_remote_free(remote);
        }

        /**
         * @brief  Fetch from and integrate with another repository or a local branch
         *
         * @param branch_name: branch name. example "master", "main", "develop"..
         */
        void pull(const std::string &remote_name = "origin", const std::string &branch_name = "master")
        {
            git_remote *remote;
            GIT_CHECK_ERROR(git_remote_lookup(&remote, m_repository, remote_name.c_str()));
            git_fetch_options options = GIT_FETCH_OPTIONS_INIT;
            options.callbacks.credentials = acquire_credentials;
            options.callbacks.payload = this;
            GIT_CHECK_ERROR(git_remote_fetch(remote, nullptr, &options, "pull"));

            git_oid branchOidToMerge;
            GIT_CHECK_ERROR(git_repository_fetchhead_foreach(
                m_repository, [](const char *name, const char *url, const git_oid *oid, unsigned int is_merge, void *payload) -> int
                {
                                //if (is_merge)
                                   GIT_CHECK_ERROR(git_oid_cpy((git_oid *)payload, oid));
                                return 0; },
                &branchOidToMerge));

            git_annotated_commit *their_heads[1];
            GIT_CHECK_ERROR(git_annotated_commit_lookup(&their_heads[0], m_repository, &branchOidToMerge));

            git_merge_analysis_t merge_analysis;
            git_merge_preference_t merge_prefs;
            GIT_CHECK_ERROR(git_merge_analysis(&merge_analysis, &merge_prefs, m_repository, (const git_annotated_commit **)their_heads, 1));

            if (merge_analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE)
            {
                std::cout << "Repository::pull(): Already up to date.\n";
                git_annotated_commit_free(their_heads[0]);
                git_repository_state_cleanup(m_repository);
                git_remote_free(remote);
                return; // Repo is up to date, no merge is needed
            }
            else if (merge_analysis & GIT_MERGE_ANALYSIS_FASTFORWARD)
            {
                std::cout << "Repository::pull(): Fast-forwarding.\n";
                git_reference *reference;
                git_reference *new_reference;

                const std::string name = ("refs/heads/" + branch_name);
                if (git_reference_lookup(&reference, m_repository, name.c_str()) == 0)
                    GIT_CHECK_ERROR(git_reference_set_target(&new_reference, reference, &branchOidToMerge, "Pull: Fast-forward"));

                GIT_CHECK_ERROR(git_reset_from_annotated(m_repository, their_heads[0], GIT_RESET_HARD, nullptr));
                git_reference_free(reference);
            }
            git_annotated_commit_free(their_heads[0]);
            git_repository_state_cleanup(m_repository);
            git_remote_free(remote);
        }

        /**
         * @brief Clone a repository into a new directory
         *
         * @param url: SSH/HTTPS URL of the repository
         * @param path: Path to the directory to clone into (this function does not create a new directory)
         */
        static std::shared_ptr<Repository> clone(const std::string &url, const fs::path &path = fs::current_path(), const char* branch = nullptr)
        {
            std::shared_ptr<Repository> repo = std::make_shared<Repository>();

            git_repository *cloned_repository = nullptr;
            git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
            clone_opts.checkout_branch = branch;
            git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

            // Set up options
            checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
            clone_opts.checkout_opts = checkout_opts;
            clone_opts.fetch_opts.callbacks.credentials = acquire_credentials;
            clone_opts.fetch_opts.callbacks.payload = repo.get();
            // TODO: cloning private repo with HTTPS may fail

            // Do the clone
            GIT_CHECK_ERROR(git_clone(&cloned_repository, url.c_str(), path.string().c_str(), &clone_opts));

            // Open cloned repository
            repo->open(path);

            git_repository_free(cloned_repository);

            return repo;
        }

        /**
         * @brief Create a new branch
         *
         * @param branch_name: branch name. example: "master", "main"...
         */
        void create_branch(const std::string &branch_name)
        {
            git_reference *reference;
            GIT_CHECK_ERROR(git_branch_create(&reference, m_repository, branch_name.c_str(), get_last_commit(), 0));
            git_reference_free(reference);
        }

        /**
         * @brief Find a branch in this repository by name
         *
         * @param branch_name
         * @return true: if the branch exists
         * @return false: if the branch does not exist
         */
        bool branch_exists(const std::string &branch_name)
        {
            git_reference *reference;
            const int res = git_branch_lookup(&reference, m_repository, branch_name.c_str(), GIT_BRANCH_ALL);
            if (res == GIT_OK)
            {
                git_reference_free(reference);
                return true;
            }
            else if (res == GIT_ENOTFOUND)
            {
                git_reference_free(reference);
                return false;
            }
            else if (res == GIT_EINVALIDSPEC)
            {
                git_reference_free(reference);
                throw std::runtime_error("Repository::branch_exists(): branch " + branch_name + " is invalid");
            }
            return false;
        }

        /**
         * @brief checkout a revision (tag, branch, commit)
         *
         * @param revision_name: revision name. example: "master", "main"...
         */
        void checkout(const std::string &revision_name)
        {
            git_object *treeish = nullptr;
            git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
            opts.checkout_strategy = GIT_CHECKOUT_SAFE;
            int is_valid = git_revparse_single(&treeish, m_repository, revision_name.c_str());
            if (is_valid != 0)
                throw std::runtime_error("Repository::checkout(): '" + revision_name + "' does not exits");
            GIT_CHECK_ERROR(is_valid);
            GIT_CHECK_ERROR(git_checkout_tree(m_repository, treeish, &opts));
            const std::string str = "refs/heads/" + revision_name;
            GIT_CHECK_ERROR(git_repository_set_head(m_repository, str.c_str()));
            git_object_free(treeish);
        }

        /**
         * @brief Returns all branches names of this repository
         *
         * @return std::vector<std::string>
         */
        std::vector<std::string> refs()
        {
            std::vector<std::string> result;
            git_branch_iterator *it;
            if (!git_branch_iterator_new(&it, m_repository, GIT_BRANCH_ALL))
            {
                git_reference *ref;
                git_branch_t type;
                while (!git_branch_next(&ref, &type, it))
                {
                    result.push_back(git_reference_name(ref));
                    git_reference_free(ref);
                }
                git_branch_iterator_free(it);
            }
            return result;
        }

        /**
         * @brief Returns all the differences made since last commit
         * @param find_options: bitwise flags from enum git_diff_find_t to custimize the find
         * @return a vector of file differences `git_diff_delta`
         */
        std::vector<git_diff_delta> diff(const std::uint32_t find_options = git_diff_find_t::GIT_DIFF_FIND_RENAMES | git_diff_find_t::GIT_DIFF_FIND_COPIES | git_diff_find_t::GIT_DIFF_FIND_FOR_UNTRACKED)
        {
            std::vector<git_diff_delta> result;

            git_diff_options diff_opts = GIT_DIFF_OPTIONS_INIT;
            diff_opts.flags = GIT_CHECKOUT_NOTIFY_CONFLICT;
            diff_opts.flags |= GIT_DIFF_INCLUDE_UNTRACKED;
            diff_opts.flags |= GIT_DIFF_RECURSE_UNTRACKED_DIRS;

            git_diff *diff;
            GIT_CHECK_ERROR(git_diff_index_to_workdir(&diff, m_repository, nullptr, &diff_opts));

            git_diff_find_options diff_find_opt = GIT_DIFF_FIND_OPTIONS_INIT;
            diff_find_opt.flags = find_options;
            GIT_CHECK_ERROR(git_diff_find_similar(diff, &diff_find_opt));

            const std::size_t num_deltas = git_diff_num_deltas(diff);
            if (num_deltas != 0)
            {
                result.reserve(num_deltas);
                for (std::size_t i = 0; i < num_deltas; ++i)
                {
                    const git_diff_delta *delta = git_diff_get_delta(diff, i);
                    result.push_back(*delta);
                }
            }

            git_diff_free(diff);
            return result;
        }

        /**
         * @brief Returns the name of the current branch
         * @return the branch name
         * @ref https://libgit2.org/libgit2/#HEAD/group/branch/git_branch_name
         */
        std::string current_branch_name()
        {
            git_reference *head;
            GIT_CHECK_ERROR(git_repository_head(&head, this->m_repository));

            const char *name;
            GIT_CHECK_ERROR(git_branch_name(&name, head));
            std::string branch_name(name);
            if (branch_name.empty())
                throw std::runtime_error("Repository::current_branch_name()(): Failed to get current branch name");

            git_reference_free(head);

            return branch_name;
        }

    private:
        /**
         * @brief Authenticates to remote servers with SSH or HTTPS
         */
        static int acquire_credentials(git_cred **out, const char *url, const char *username_from_url, unsigned int allowed_types, void *payload)
        {
            if (allowed_types & GIT_CREDENTIAL_SSH_KEY)
            {
                std::cout << "Repository::acquire_credentials(): Authenticating with SSH\n";
                const int res = git_cred_ssh_key_from_agent(out, username_from_url);
                if (res < 0)
                    throw std::runtime_error("Repository::acquire_credentials(): Failed to authenticate with SSH: " + std::string(giterr_last()->message));
                return res;
            }
            else if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT)
            {
                std::cout << "Repository::acquire_credentials(): Athenticating with HTTPS\n";
                const Repository *__this = static_cast<Repository *>(payload);
                if (__this->m_username.empty())
                    throw std::runtime_error("Repository::acquire_credentials(): username is missing");
                if (__this->m_password.empty())
                    throw std::runtime_error("Repository::acquire_credentials(): password is missing");

                const int res = git_cred_userpass_plaintext_new(out, __this->m_username.c_str(), __this->m_password.c_str());
                if (res < 0)
                    throw std::runtime_error("Repository::acquire_credentials(): Failed to authenticate with HTTPS: " + std::string(giterr_last()->message));
                return res;
            }
            else
            {
                throw std::runtime_error("Repository::acquire_credentials(): Failed to authenticate: unsupported protocol");
            }
        }

        /**
         * @brief Get the last commit
         *
         * @return git_commit*
         */
        git_commit *get_last_commit()
        {
            git_commit *commit = nullptr;
            git_oid oid_parent_commit;
            if (git_reference_name_to_id(&oid_parent_commit, m_repository, "HEAD") == 0)
            {
                if (git_commit_lookup(&commit, m_repository, &oid_parent_commit) == 0)
                    return commit;
            }
            return nullptr;
        }

    private:
        std::string m_username;
        std::string m_password;

        git_repository *m_repository;
    };

}
