/**
 * Auto-generated with xtypes_generator types_generator 04/18/2023 11:48:39
 */

#pragma once
#include "_ExternalReference.hpp"


namespace xtypes {
    // Forward Declarations

    class ExternalReference : public _ExternalReference
    {
        public:
            /// Constructor
            ExternalReference(const std::string& classname = ExternalReference::classname);

            // Static indentifier
            /// Useful to lookup the derived classname at compile time
            static const std::string classname;

            /// Custom URI generator (overrides default implementation in _ExternalReference)
            std::string uri() const override;

            // Method Declarations
            /// This function loads the remote content to local directory.
            virtual nl::json load(const std::string& local_dir = ".");
            
            /// This function stores the local content to the remote_url
            virtual void store(const std::string& local_dir = ".");
            
            // Overrides for setters of properties
            // Overrides for relation setters
    };

    using ExternalReferencePtr = std::shared_ptr<ExternalReference>;
    using ExternalReferenceCPtr = const std::shared_ptr<ExternalReference> ;
    using ConstExternalReferencePtr = std::shared_ptr<const ExternalReference> ;
    using ConstExternalReferenceCPtr = const std::shared_ptr<const ExternalReference> ;
}
