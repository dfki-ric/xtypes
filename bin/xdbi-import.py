#!/usr/bin/python3
import os
import sys
import json
import argparse
import xdbi_py
import xtypes_py


backends = xdbi_py.get_available_backends()


def main():
    parser = argparse.ArgumentParser(description='Import Xtypes from json files into the X-Rock graph database')
    # xdbi specific args
    parser.add_argument('-b', '--db_backend', help="Database backend to be used", choices=backends, default=backends[0]) 
    parser.add_argument('-a', '--db_address', help="The url/local path to the db", required=True) 
    parser.add_argument('-g', '--db_graph', help="The name of the database graph to be used", required=True)
    # tool specific args 
    parser.add_argument('json_file', help="The JSON file which contains the (top-level) Xtype specification, format xxx.json")
    
    args = None
    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(1)

    successful = True
    print(f"Setting up registry ...")
    registry = xtypes_py.ProjectRegistry()   

    print(f"Accessing database at {args.db_address}")
    dbi = xdbi_py.db_interface_from_config(registry, config={"type":args.db_backend, "address": args.db_address, "graph": args.db_graph}, read_only=False)

    file_name = os.path.abspath(args.json_file)
    path, xtype_uuid = os.path.split(file_name)
    xtype_uuid = os.path.splitext(xtype_uuid)[0]
    print(f"Import path set to {path}")

    def load_by_uri(uri) -> xtypes_py.XType:
        uuid = xtypes_py.uri_to_uuid(uri)
        fname = os.path.join(path, f"{str(uuid)}.json")
        with open(fname, 'r') as import_file:
            print(f"Importing {fname}")
            spec = json.loads(import_file.read())
            xtype = xtypes_py.XType.import_from(spec, registry)
            return xtype
        return None
    registry.set_load_func(load_by_uri)

    with open(file_name, 'r') as import_file:
        toplvl_spec = json.loads(import_file.read())
    xtype_uri = toplvl_spec['uri']
    xtype = xtypes_py.XType.import_from(xtype_uri, registry)
    if not xtype:
        print(f"Could not import {xtype_uri}")
        sys.exit(2)

    print(f"Adding/updating {xtype_uri} to/in graph database")
    dbi.update([xtype])
    
    script_name = sys.argv[0]
    print(f"{script_name} executed successfully")

if __name__ == '__main__':
    main()

