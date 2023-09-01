#!/usr/bin/python3
import os
import sys
import json
import argparse
import xdbi_py
import xtypes_py


backends = xdbi_py.get_available_backends()


def main():
    parser = argparse.ArgumentParser(description='Export Xtypes from the X-Rock graph database into JSON file(s)')
    # xdbi specific args
    parser.add_argument('-b', '--db_backend', help="Database backend to be used", choices=backends, default=backends[0])
    parser.add_argument('-a', '--db_address', help="The url/local path to the db", required=True)
    parser.add_argument('-g', '--db_graph', help="The name of the database graph to be used", required=True)
    # tool specific args
    parser.add_argument('uri', help="The URI of the Xtype to export")
    parser.add_argument('-c','--classname',  help="The name of the Xtype-derived class [%(default)s]", default="xtypes::ComponentModel")
    parser.add_argument('-p','--export_path', help="Path to the root directory to create file(s) in", default=".")
    parser.add_argument('-m', '--max_depth', help="The maximum depth to which dependencies are resolved and exported", type=int, default=-1)
    
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

    p = os.path.abspath(args.export_path)
    print(f"Export path set to {p}")
    recursion_limit=args.max_depth
    print(f"Recursion depth limited to {recursion_limit}")
    xtype_uri = args.uri
    xtypes = dbi.find(classname = args.classname, properties={'uri':xtype_uri}, search_depth=recursion_limit)
    if len(xtypes) < 1:
        print(f"Xtype with URI {xtype_uri} not found")
        sys.exit(2)
    for xtype in xtypes:
        print(f"Exporting Xtype with URI {xtype_uri}")
        try:
            export_list = xtype.export_to(recursion_limit=recursion_limit, export_relation_properties = True)
        except:
            print(f"Failed. Try to increase recursion limit")
            sys.exit(3)
        if xtype_uri not in export_list:
            print(f"Failed to export {xtype_uri}. Not in {export_list}.")
            sys.exit(4)
        for export_uri, export_content in export_list.items():
            file_name = os.path.join(p, f"{export_content['uuid']}.json")
            print(f"File {file_name} exists. Perform JSON merge")
            if os.path.exists(file_name):
                # On file collision, we have to perform a merge
                print(f"File {file_name} exists. Perform JSON merge")
                with open(file_name, 'r') as in_file:
                    old_dict = json.loads(in_file.read())
                    print(f"Data from Old file: {old_dict}")
            with open(file_name, 'w') as out_file:
                json.dump(export_content, out_file)
            print(f"Exported {export_content['uri']} to {file_name}")

    if successful:
        script_name = sys.argv[0]
        print(f"{script_name} executed successfully")


if __name__ == '__main__':
    main()

