#!/usr/bin/python3
import sys
import os
import argparse
import xdbi_py
import xtypes_py


backends = xdbi_py.get_available_backends()


def main():
    parser = argparse.ArgumentParser(description='Tries to fix errors in the database. This is an expert tool! Use with caution.')
    # xdbi specific args
    parser.add_argument('-b', '--db_backend', help="Database backend to be used", choices=backends, default=backends[0]) 
    parser.add_argument('-a', '--db_address', help="The url/local path to the db", required=True) 
    # tool specific args
    parser.add_argument('-s','--src_graph', help="The name of the source graph", default="src_graph")
    parser.add_argument('-d','--dst_graph', help="The name of the destination graph", default="dst_graph")
    parser.add_argument('--max_depth_for_load', help="Maximum depth for fast load before falling back to full load", default=3, type=int)
    parser.add_argument('--max_depth_for_store', help="Maximum depth for storing the Xtypes in destination graph", default=1, type=int)
    parser.add_argument('--always_load_full', help="Always use full loading (slow)", action="store_true")
    parser.add_argument('--use_update', help="Use update instead of add when storing Xtypes in destination graph", action="store_true")
    
    args = None
    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(1)

    if args.src_graph == args.dst_graph:
        print("Please choose a different destination graph")
        sys.exit(2)

    print(f"Setting up registry ...")
    registry = xtypes_py.ProjectRegistry()   

    print(f"Accessing database at {args.db_address}")
    src_dbi = xdbi_py.db_interface_from_config(registry, config={"type":args.db_backend, "address": args.db_address, "graph": args.src_graph}, read_only=True)
    dst_dbi = xdbi_py.db_interface_from_config(registry, config={"type":args.db_backend, "address": args.db_address, "graph": args.dst_graph}, read_only=False)

    print(f"Looking up URIs in source graph ...")
    uris = src_dbi.uris()
    print(f"Found {len(uris)} uris.")

    to_be_stored = set()
    to_be_removed = set()
    xtypes = {}

    print(f"Analyzing xtypes in source graph ...")
    print(f"Load xtypes and their immediate references (this may take a while) ...")
    for old_uri in uris:
        xtype = None
        loaded = False
        if not args.always_load_full:
            for depth in range(1,args.max_depth_for_load):
                try:
                    xtype = src_dbi.load(old_uri,"",depth)
                    new_uri = xtype.uri()
                    # We also have to check if the uris of the references can be constructed (used in second pass)
                    rels = xtype.get_relations()
                    for relname in rels:
                        if xtype.has_facts(relname):
                            facts = xtype.get_facts(relname)
                            for fact in facts:
                                fact.target.uri()
                    loaded = True
                except:
                    continue
        # Try full load if max depth was not sufficient
        if not loaded:
            try:
                xtype = src_dbi.load(old_uri)
                new_uri = xtype.uri()
                # We also have to check if the uris of the references can be constructed (used in second pass)
                rels = xtype.get_relations()
                for relname in rels:
                    if xtype.has_facts(relname):
                        facts = xtype.get_facts(relname)
                        for fact in facts:
                            fact.target.uri()
                loaded = True
            except:
                # Here we found a broken entry
                print(f"Marked broken entry {old_uri} to be removed (cannot be loaded correctly)")
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
                dst_dbi.update([xtypes[old_uri]], args.max_depth_for_store)
            else:
                dst_dbi.add([xtypes[old_uri]], args.max_depth_for_store)
            stored += 1
        except Exception as error:
            print(f"WARNING: Could not store {old_uri}: {error}!")

    print("SUMMARY")
    print(f"Found {len(uris)} xtypes")
    print(f"Wanted to store {len(to_be_stored)} xtypes")
    print(f"Stored {stored} xtypes")
    print(f"Dropped {len(to_be_removed)} bogus/deprecated xtypes")
    if stored < len(to_be_stored):
        print(f"Stored only {stored} out of {len(to_be_stored)}.")

    script_name = sys.argv[0]
    print(f"{script_name} executed successfully")


if __name__ == '__main__':
    main()


#todo if user do control c then script should stop running
