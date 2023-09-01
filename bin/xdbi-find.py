#!/usr/bin/python3
import sys
import os
import argparse
import xdbi_py
import xtypes_py


backends = xdbi_py.get_available_backends()


def main():
    parser = argparse.ArgumentParser(description='Find an Xtype of a certain class inside the graph database')
    # xdbi specific args
    parser.add_argument('-b', '--db_backend', help="Database backend to be used", choices=backends, default=backends[0]) 
    parser.add_argument('-a', '--db_address', help="The url/local path to the db", required=True) 
    parser.add_argument('-g', '--db_graph', help="The name of the database graph to be used", required=True) 
    parser.add_argument('-m', '--model_name', help="Name of the model to update in the database", required=True) 
    parser.add_argument('-v', '--model_version', help="The version to set by default [%(default)s]", default='v0.0.1') 
    parser.add_argument('-d', '--model_domain', help="The domain of the model [%(default)s]", choices=xtypes_py.ComponentModel().get_allowed_property_values("domain"), default='SOFTWARE') 
    # tool specific args
    parser.add_argument('-V','--verbose', dest="verbose", help="Print-out additional information about found xtypes", action="store_true")
    parser.add_argument('-c','--classname',  help="The name of the Xtype-derived class [%(default)s]", default="ComponentModel")
    parser.add_argument('-m', '--max_depth', help="The maximum Search depth ", type=int, default=-1)
    
    parser.set_defaults(verbose=False)
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

    if args.classname:
        print(f"Finding Xtype(s) of class {args.classname} in graph {args.db_graph}")
    else:
        print(f"Finding Xtype(s) in graph {args.db_graph}")
    properties = {}
    if args.model_name is not None:
        properties = {"name": args.model_name, "domain": args.model_domain, "version": args.model_version}
    xtypes = dbi.find(classname=args.classname, properties = properties, search_depth=max_depth)
    
    if not xtypes:
        print("Could not find any Xtype")
        sys.exit(2)
    for x in xtypes:
        if args.verbose:
            print(f"{x.uri()} {x.get_properties()}")
        else:
            print(x.uri())
                
    if successful:
        script_name = sys.argv[0]
        print(f"{script_name} executed successfully")


if __name__ == '__main__':
    main()

