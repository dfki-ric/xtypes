#!/usr/bin/python3
import sys
import os
import argparse
import xdbi_py
import xtypes_py
import copy


def main():
    backends = xdbi_py.get_available_backends()

    config = {
            "type": "MultiDbClient",
            "config":
                {
                    "import_servers": [],
                    "main_server":
                        {
                            "type": None,
                            "address": None,
                            "graph": None
                        }
                }
            }

    parser = argparse.ArgumentParser(description='Tries to fix errors in the database. This is an expert tool! Use with caution.')
    # xdbi specific args
    parser.add_argument('-b', '--db_backend', help="Database backend to be used", choices=backends, default=backends[0])
    parser.add_argument('-a', '--db_address', help="The url/local path to the db", required=True)
    parser.add_argument('-i', '--import_server', help="Specify an import server for additional lookup (format: <backend>,<address>,<graph>)", action="append", type=str)
    # tool specific args
    parser.add_argument('-s','--src_graph', help="The name of the source graph", default="src_graph", required=True)
    parser.add_argument('-d','--dst_graph', help="The name of the destination graph", default="dst_graph", required=True)
    parser.add_argument('--max_depth_for_store', help="Maximum depth for storing the Xtypes in destination graph", default=1, type=int)
    parser.add_argument('--use_update', help="Use update instead of add when storing Xtypes in destination graph", action="store_true")

    args = None
    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(1)

    config["config"]["main_server"]["type"] = args.db_backend
    config["config"]["main_server"]["address"] = args.db_address
    config["config"]["main_server"]["graph"] = args.dst_graph
    import_servers = args.import_server
    if import_servers:
        for import_server in import_servers:
            b,a,g = import_server.split(",", 2)
            config["config"]["import_servers"].append( {"type": b.strip(), "address": a.strip(), "graph": g.strip() } )
    config["config"]["import_servers"].append( { "name": "main", "type": args.db_backend, "address": args.db_address, "graph": args.src_graph } )

    if args.src_graph == args.dst_graph:
        print("Please choose a different destination graph")
        sys.exit(2)

    print(f"Setting up registry ...")
    registry = xtypes_py.ProjectRegistry()   

    print(f"Accessing database at {args.db_address}")
    dbi = xdbi_py.db_interface_from_config(registry, config=config, read_only=False)

    print(f"Looking up URIs in source graph ...")
    src_registry = xtypes_py.ProjectRegistry()
    src_dbi = xdbi_py.db_interface_from_config(src_registry, config=config["config"]["import_servers"][-1], read_only=True)
    uris = src_dbi.uris()
    print(f"Found {len(uris)} uris in main database")

    to_be_stored = set()
    to_be_removed = set()
    xtypes = {}

    print(f"Analyzing xtypes in source graph ...")
    print(f"Load xtypes and their immediate references (this may take a while) ...")
    for old_uri in uris:
        xtype = None
        try:
            xtype = dbi.load(old_uri)
            new_uri = xtype.uri()
            # We also have to check if the uris of the references can be constructed (used in second pass)
            rels = xtype.get_relations()
            for relname in rels:
                if xtype.has_facts(relname):
                    facts = xtype.get_facts(relname)
                    for fact in facts:
                        fact.target.uri()
            loaded = True
        except Exception as error:
            # Here we found a broken entry
            print(f"Marked broken entry {old_uri} to be removed (cannot be loaded correctly): {error}")
            to_be_removed.add(old_uri)
            continue
        # Store resolved xtype
        xtypes[old_uri] = xtype
        to_be_stored.add(old_uri)

    print(f"Storing {len(to_be_stored)} xtypes in destination graph ...")
    stored = 0
    for old_uri in to_be_stored:
        try:
            if args.use_update:
                dbi.update([xtypes[old_uri]], args.max_depth_for_store)
            else:
                dbi.add([xtypes[old_uri]], args.max_depth_for_store)
            stored += 1
        except Exception as error:
            print(f"WARNING: Could not store {old_uri}: {error}!")

    print(f"Removing {len(to_be_removed)} xtypes in destination graph ...")
    removed = 0
    for old_uri in to_be_removed:
        try:
            dbi.remove(old_uri)
            removed += 1
        except Exception as error:
            print(f"WARNING: Could not remove {old_uri}: {error}!")

    print("SUMMARY")
    print(f"Found {len(uris)} xtypes")
    print(f"Wanted to store {len(to_be_stored)} xtypes")
    print(f"Wanted to remove {len(to_be_removed)} bogus/deprecated xtypes")
    if stored < len(to_be_stored):
        print(f"Stored only {stored} out of {len(to_be_stored)}.")
    if removed < len(to_be_removed):
        print(f"Removed only {removed} out of {len(to_be_removed)}.")

    script_name = sys.argv[0]
    print(f"{script_name} executed successfully")


if __name__ == '__main__':
    main()


#TODO if user do control c then script should stop running
